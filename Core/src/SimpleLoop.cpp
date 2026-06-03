#include "stm32l0xx_hal.h"
#include "stm32l0xx_ll_gpio.h"
#include "stm32l0xx_ll_spi.h"
#include "stm32l0xx_ll_dma.h"
#include "stm32l0xx_ll_bus.h"
#include "stm32l0xx_ll_tim.h"
#include "MainTaskAlt.h"
#include "SEGGER_RTT.h"
#include "etl/callback_timer.h"
#include "etl/function.h"
#include "etl/bitset.h"
#include "etl/histogram.h"
#include "hardware.h"
#include "common.h"
#include "config.h"
#include <semphr.h>

#define MODNAME "[sloo] "


#define MASTER_TIMER    TIM21
#define SLAVE_TIM       TIM2


enum Notify {
    Interrupt = (1 << 0),
    Timeout = (1 << 1),
    SlaveTimOverflow = (1 << 2)
};


static TaskHandle_t SimpleLoopTaskHandle = nullptr;
static QueueHandle_t xQueueMultiplier = nullptr;
static QueueHandle_t xQueueLedState = nullptr;


static etl::callback_timer<3> timer_controller;
static etl::timer::id::type timer_loop_timeout;


namespace rtos_static {
    namespace simpleloop {
        constexpr size_t TaskStackSize = configMINIMAL_STACK_SIZE * 3;
        constexpr BaseType_t TaskPriority = configMAX_PRIORITIES - 1;

        static StackType_t ucTaskStack[TaskStackSize];
        static StaticTask_t xTCBTask;

        static uint8_t ucQueueLedStateStorage[1 * sizeof(uint32_t)];
        static StaticQueue_t xQueueLedStateStatic;

        static constexpr size_t xQueueMultiplierSize = 1;
        static uint8_t ucQueueMultiplierStorage[xQueueMultiplierSize * sizeof(lc::Multiplier)];
        static StaticQueue_t xQueueMultiplierStatic;
    }
}


namespace st = rtos_static::simpleloop;


static void InitTimers() {
    // TIM2 Slave in Gated mode, Clock 32 MHz
    // Таймер считает до ARR, переполняется в 0 и генерирует UIE (update interrupt)
    // PSC обновляется только при событии UIE
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_TIM2_FORCE_RESET();
    __HAL_RCC_TIM2_RELEASE_RESET();
    SLAVE_TIM->SR = 0;
    SLAVE_TIM->PSC = 0; // 32 MHz счет
    SLAVE_TIM->ARR = UINT16_MAX;
    SLAVE_TIM->SMCR = (0b000 << TIM_SMCR_TS_Pos) | (0b101 << TIM_SMCR_SMS_Pos); // TIM21 - Master, Gated mode
    SLAVE_TIM->EGR = 1 << TIM_EGR_UG_Pos; // Обновим все настройки из буферизованных регистров (TIM->PSC)
    SLAVE_TIM->SR = 0;
    SLAVE_TIM->DIER = TIM_DIER_UIE; // Прерывание по переполнению
    NVIC_SetPriority(TIM2_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 6, 0));
    NVIC_EnableIRQ(TIM2_IRQn);

    __HAL_RCC_TIM21_CLK_ENABLE();
    // TIM21, Мастер таймер, который считает импульсы с генератора
    // Вход CC2 как вход, IC2 подключен к TI1 (CC2 channel is configured as input, IC2 is mapped on TI2)
    MASTER_TIMER->CCMR1 = (0b01 << TIM_CCMR1_CC2S_Pos) | (0b0000 << TIM_CCMR1_IC2F_Pos);
    // OC1 Active high
    MASTER_TIMER->CCER = (0b0 << TIM_CCER_CC2NP_Pos) | (0b0 << TIM_CCER_CC2P_Pos);

    // CH1 - выход, PWM mode 2, выход 0 пока TIM_CNT < TIM_CCR1.
    MASTER_TIMER->CCMR1 |= (0b00 << TIM_CCMR1_CC1S_Pos) | (0b111 << TIM_CCMR1_OC1M_Pos);
    //
    MASTER_TIMER->CCER |= (1 << TIM_CCER_CC1E_Pos) | (0 << TIM_CCER_CC1P_Pos);

    MASTER_TIMER->SMCR = (0b111 << TIM_SMCR_SMS_Pos);
    MASTER_TIMER->SMCR |= (0b110 << TIM_SMCR_TS_Pos);

    MASTER_TIMER->ARR = 255;
    MASTER_TIMER->DIER = TIM_DIER_UIE;
    MASTER_TIMER->SR = 0;
    MASTER_TIMER->CR2 = (0b100 << TIM_CR2_MMS_Pos);

    uint16_t ccr = MASTER_TIMER->ARR - 1 + 1;
    MASTER_TIMER->CCR1 = ccr;

    MASTER_TIMER->CR1 = TIM_CR1_CEN;
    SLAVE_TIM->CR1 |= TIM_CR1_CEN;

    NVIC_SetPriority(TIM21_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 7, 0));
    NVIC_EnableIRQ(TIM21_IRQn);
}

static void ResetTimers() {
    MASTER_TIMER->SR = 0;
    MASTER_TIMER->CNT = 0;
    SLAVE_TIM->CNT = 0;
    SLAVE_TIM->SR = 0;
}


enum StLoop {
    LOOP_STOP,
    LOOP_A_EN,
    LOOP_B_EN,
    LOOP_C_EN
};


uint16_t timer_multiplier[3] = {1, 1, 1};


#define LOOP_TIMEOUT_MS         5

void SimpleTask(void *pvParameters) {
    vTaskDelay(20);
    auto &lc = getLC();

    uint16_t switch_all = 0;
    xQueueSend(lc.xQueueSettings, &switch_all, portMAX_DELAY);

    LL_GPIO_SetOutputPin(LOOP_ABC_EN_PORT, LOOP_A_EN_PIN | LOOP_B_EN_PIN | LOOP_C_EN_PIN);
    timer_loop_timeout = timer_controller.register_timer([]{ xTaskNotify(SimpleLoopTaskHandle, Notify::Timeout, eSetBits); }, 10, false);
    timer_controller.enable(true);


    StLoop state = StLoop::LOOP_STOP;
    uint32_t lastTick = xTaskGetTickCount();
    uint16_t slave_cnt;
    lc::Measure measure;

    uint32_t ulNotifyValue = 0;
    for (;;) {
        auto event = xTaskNotifyWait(0, UINT32_MAX, &ulNotifyValue, 1);

        switch (state) {
        case StLoop::LOOP_STOP: {
            InitTimers();

            ResetTimers();
            MASTER_TIMER->ARR = timer_multiplier[0] + 10;
            MASTER_TIMER->CCR1 = MASTER_TIMER->ARR - timer_multiplier[0] + 1;
            state = StLoop::LOOP_A_EN;
            timer_controller.set_period(timer_loop_timeout, LOOP_TIMEOUT_MS);
            timer_controller.start(timer_loop_timeout);
            LL_GPIO_ResetOutputPin(LOOP_ABC_EN_PORT, LOOP_A_EN_PIN);
            break;
        }

        case StLoop::LOOP_A_EN:
            if (event == pdTRUE) {
                if (ulNotifyValue & Notify::SlaveTimOverflow) {
                    SEGGER_RTT_printf(0, MODNAME "Loop A OVF\n");
                }
                if (ulNotifyValue & Notify::Interrupt) {
                    timer_controller.stop(timer_loop_timeout);
                    slave_cnt = SLAVE_TIM->CNT;
                    measure.loop = lc::LoopName::LOOP_A;
                    measure.value = slave_cnt;
                    measure.multiplier = 1;
                    if (xTaskGetTickCount() > 2000)
                        xQueueSend(lc.xQueueLoopMeasure, &measure, 1);
                }
                if (ulNotifyValue & Notify::Timeout) {
                    LL_GPIO_SetOutputPin(LOOP_ABC_EN_PORT, LOOP_A_EN_PIN | LOOP_B_EN_PIN | LOOP_C_EN_PIN);
                    SEGGER_RTT_printf(0, MODNAME "Loop A TO\n");
                }

                ResetTimers();
                MASTER_TIMER->ARR = timer_multiplier[1] + 10;
                MASTER_TIMER->CCR1 = MASTER_TIMER->ARR - timer_multiplier[1] + 1;
                state = StLoop::LOOP_B_EN;
                timer_controller.set_period(timer_loop_timeout, LOOP_TIMEOUT_MS);
                timer_controller.start(timer_loop_timeout);
                LL_GPIO_ResetOutputPin(LOOP_ABC_EN_PORT, LOOP_B_EN_PIN);
            }
            break;

        case StLoop::LOOP_B_EN:
            if (event == pdTRUE) {
                if (ulNotifyValue & Notify::SlaveTimOverflow) {
                    SEGGER_RTT_printf(0, MODNAME "Loop B OVF\n");
                }
                if (ulNotifyValue & Notify::Interrupt) {
                    timer_controller.stop(timer_loop_timeout);
                    slave_cnt = SLAVE_TIM->CNT;
                    measure.loop = lc::LoopName::LOOP_B;
                    measure.value = slave_cnt;
                    measure.multiplier = 1;
                    if (xTaskGetTickCount() > 2000)
                        xQueueSend(lc.xQueueLoopMeasure, &measure, 1);
                }
                if (ulNotifyValue & Notify::Timeout) {
                    LL_GPIO_SetOutputPin(LOOP_ABC_EN_PORT, LOOP_A_EN_PIN | LOOP_B_EN_PIN | LOOP_C_EN_PIN);
                    SEGGER_RTT_printf(0, MODNAME "Loop B TO\n");
                }

                ResetTimers();
                MASTER_TIMER->ARR = timer_multiplier[2] + 10;
                MASTER_TIMER->CCR1 = MASTER_TIMER->ARR - timer_multiplier[2] + 1;
                state = StLoop::LOOP_C_EN;
                timer_controller.set_period(timer_loop_timeout, LOOP_TIMEOUT_MS);
                timer_controller.start(timer_loop_timeout);
                LL_GPIO_ResetOutputPin(LOOP_ABC_EN_PORT, LOOP_C_EN_PIN);
            }
            break;

        case LOOP_C_EN:
            if (event == pdTRUE) {
                if (ulNotifyValue & Notify::SlaveTimOverflow) {
                    SEGGER_RTT_printf(0, MODNAME "Loop C OVF\n");
                }
                if (ulNotifyValue & Notify::Interrupt) {
                    timer_controller.stop(timer_loop_timeout);
                    slave_cnt = SLAVE_TIM->CNT;
                    measure.loop = lc::LoopName::LOOP_C;
                    measure.value = slave_cnt;
                    measure.multiplier = 1;
                    if (xTaskGetTickCount() > 2000)
                        xQueueSend(lc.xQueueLoopMeasure, &measure, 1);
                }
                if (ulNotifyValue & Notify::Timeout) {
                    LL_GPIO_SetOutputPin(LOOP_ABC_EN_PORT, LOOP_A_EN_PIN | LOOP_B_EN_PIN | LOOP_C_EN_PIN);
                    SEGGER_RTT_printf(0, MODNAME "Loop C TO\n");
                }

                ResetTimers();
                MASTER_TIMER->ARR = timer_multiplier[0] + 10;
                MASTER_TIMER->CCR1 = MASTER_TIMER->ARR - timer_multiplier[0] + 1;
                state = StLoop::LOOP_A_EN;
                timer_controller.set_period(timer_loop_timeout, LOOP_TIMEOUT_MS);
                timer_controller.start(timer_loop_timeout);
                LL_GPIO_ResetOutputPin(LOOP_ABC_EN_PORT, LOOP_A_EN_PIN);
            }
            break;
        }


        lc::Multiplier mul;
        if (xQueueReceive(xQueueMultiplier, &mul, 0) == pdPASS) {
            SEGGER_RTT_printf(0, MODNAME"%c) mul: %u\n", mul.loop + 'A', mul.multiplier);
            timer_multiplier[mul.loop] = mul.multiplier;
        }

        TickType_t currentTick = xTaskGetTickCount();
        timer_controller.tick(currentTick - lastTick);
        lastTick = currentTick;


    } // for(;;)
}


extern "C" void TIM21_IRQHandler() {
BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (MASTER_TIMER->SR & TIM_SR_UIF) {
        LL_GPIO_SetOutputPin(LOOP_ABC_EN_PORT, LOOP_A_EN_PIN | LOOP_B_EN_PIN | LOOP_C_EN_PIN); // заканчиваем трейн и отключаем генератор
        xTaskNotifyFromISR(SimpleLoopTaskHandle, Notify::Interrupt, eSetBits, &xHigherPriorityTaskWoken);
    }
    MASTER_TIMER->SR = 0;
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


extern "C" void TIM2_IRQHandler() {
BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (SLAVE_TIM->SR & TIM_SR_UIF) {
        xTaskNotifyFromISR(SimpleLoopTaskHandle, Notify::SlaveTimOverflow, eSetBits, &xHigherPriorityTaskWoken);
    }
    SLAVE_TIM->SR = 0;
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


void StartSimpleLoop() {
    auto &lc = getLC();
    lc.InitObjects();

    SimpleLoopTaskHandle = xTaskCreateStatic(SimpleTask, "Stask", st::TaskStackSize, nullptr, st::TaskPriority, st::ucTaskStack, &st::xTCBTask);
    xQueueMultiplier = xQueueCreateStatic(st::xQueueMultiplierSize, sizeof(lc::Multiplier), st::ucQueueMultiplierStorage, &st::xQueueMultiplierStatic);
    xQueueLedState = xQueueCreateStatic(1, sizeof(uint32_t), st::ucQueueLedStateStorage, &st::xQueueLedStateStatic);
    lc.xLedState = xQueueLedState;
    lc.xQueueMultiplier = xQueueMultiplier;
}

