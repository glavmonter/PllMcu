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
#include "etl/histogram.h"
#include "hardware.h"
#include "common.h"
#include "config.h"
#include <semphr.h>

#define MODNAME "[main] "

#if FREQ_MODE == FREQ_MODE_TWO_TIMER
#define MASTER_TIMER    TIM21
#define SLAVE_TIM       TIM2
#elif FREQ_MODE == FREQ_MODE_DMA
#define DMA_TIMER       TIM2
#elif FREQ_MODE == FREQ_MODE_ONE_TIMER

#define LOOP_PERIOD     401     // 40 ms
#define LOOP_TIMER      TIM2
#define METER_TIMER     TIM22
#define METER_ALT_TIMER TIM21

#endif


enum Notify {
    Interrupt = (1 << 0),
    Timeout = (1 << 1)
#if FREQ_MODE == FREQ_MODE_DMA
    , StartDMA = (1 << 2)
#endif
#if FREQ_MODE == FREQ_MODE_TWO_TIMER
    , SlaveTimOverflow = (1 << 2)
#endif
};


#if FREQ_MODE == FREQ_MODE_TWO_TIMER
static etl::callback_timer<3> timer_controller;
static etl::timer::id::type timer_loop_timeout;
#elif FREQ_MODE == FREQ_MODE_DMA
etl::callback_timer<4> timer_controller;
#elif FREQ_MODE == FREQ_MODE_ONE_TIMER
etl::callback_timer<3> timer_controller;
#else
#error "FREQ_MODE must be FREQ_MODE_DMA or FREQ_MODE_TIMERS"
#endif


static MainTask mt;
TaskHandle_t MainTaskHandle = nullptr;

namespace rtos_static {
    namespace maintask {
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

namespace st = rtos_static::maintask;

static void SpiTransmit(uint8_t data);


void MainTask::StartReceived() {
    reset_timer = timer_controller.register_timer([]{ NVIC_SystemReset(); }, 500, false);
    timer_loop_timeout = timer_controller.register_timer([] { xTaskNotify(MainTaskHandle, Notify::Timeout, eSetBits); }, 20, false); // TODO China xfer


    xQueueLedState = xQueueCreateStatic(1, sizeof(uint32_t), st::ucQueueLedStateStorage, &st::xQueueLedStateStatic);
    getLC().xLedState = xQueueLedState;

    xQueueMultiplier = xQueueCreateStatic(st::xQueueMultiplierSize, sizeof(lc::Multiplier), st::ucQueueMultiplierStorage, &st::xQueueMultiplierStatic);
    getLC().xQueueMultiplier = xQueueMultiplier;

    InitLoopTimer();
    state_loop = LOOP_STOP;
}

#if FREQ_MODE == FREQ_MODE_DMA
constexpr size_t dma_mem_size = 16;
uint16_t dma_mem[dma_mem_size] = {0};
using Histogram = etl::sparse_histogram<int32_t, uint8_t, dma_mem_size>;
Histogram dma_hist;
#endif

#if FREQ_MODE == FREQ_MODE_TWO_TIMER
#define COMPARATOR_CYCLES_MAXIMUM       256

// TODO China xfer
// comparator_cycles для китайских трансформаторов примерно 64
const auto comparator_cycles = COMPARATOR_CYCLES_MAXIMUM;           //  Сколько импульсов компаратора будет посчитано
const auto comparator_cycles_measured = 1;   //  Сколько импульсов компаратора будет участвовать в измерениях

uint16_t cycles_measure[3] = {UINT16_MAX, UINT16_MAX, UINT16_MAX};
#endif


/**
 * Таймер TIM21 считаем внешние импульсы с компаратора. CH2 используется как внешний тактирующий.
 * TIM21_ARR - количество импульсов, которые посчитает таймер и вызовет прерывание Update Event, означающий конец трейна.
 * Канал CH1 работает на выход синхронизации TRGO. Режим PWM2, считает количество импульсов в конце трейна,
 * то есть после переходного процесса запуска генератора.
 *
 * TIM2 - в режиме подчиненного Gated. Считает, пока TGRI в логической 1.
 */
void MainTask::InitLoopTimer() {
    // TIM2 Slave in Gated mode, Clock 32 MHz
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
    // CC2 channel is configured as input, IC2 is mapped on TI2
    // IC2F - Filter,
    MASTER_TIMER->CCMR1 = (0b01 << TIM_CCMR1_CC2S_Pos) | (0b0000 << TIM_CCMR1_IC2F_Pos);
    MASTER_TIMER->CCER = (0b0 << TIM_CCER_CC2NP_Pos) | (0b0 << TIM_CCER_CC2P_Pos);

    // CH1 - выход, PWM mode 1
    MASTER_TIMER->CCMR1 |= (0b00 << TIM_CCMR1_CC1S_Pos) | (0b111 << TIM_CCMR1_OC1M_Pos);
    // TODO CH1 включаем на выход с нужной полярностью, для тестов
    MASTER_TIMER->CCER |= (1 << TIM_CCER_CC1E_Pos) | (0 << TIM_CCER_CC1P_Pos);  // CC1P - полярность по которой встаёт и опускается TRGO: 0 - фронт, 1 - срез

    MASTER_TIMER->SMCR = (0b111 << TIM_SMCR_SMS_Pos); // External Clock mode 1
    MASTER_TIMER->SMCR |= (0b110 << TIM_SMCR_TS_Pos); // TI2FP2 as trigger

    MASTER_TIMER->ARR = (comparator_cycles - 1);
    MASTER_TIMER->DIER = TIM_DIER_UIE;
    MASTER_TIMER->SR = 0;
    MASTER_TIMER->CR2 = (0b100 << TIM_CR2_MMS_Pos);

    auto ccr = MASTER_TIMER->ARR - comparator_cycles_measured + 1;
    MASTER_TIMER->CCR1 = ccr;

    MASTER_TIMER->CR1 = TIM_CR1_CEN;
    SLAVE_TIM->CR1 |= TIM_CR1_CEN;

    NVIC_SetPriority(TIM21_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 7, 0));
    NVIC_EnableIRQ(TIM21_IRQn);
}


#if FREQ_MODE == FREQ_MODE_DMA

#endif

uint32_t MainTask::PrepareMeterTimer(lc::LoopName loop) {
lc::Measure measure;
    measure.loop = loop;
#if FREQ_MODE == FREQ_MODE_TWO_TIMER
    measure.value = SLAVE_TIM->CNT;
    measure.multiplier = cycles_measure[(int)loop];

#elif FREQ_MODE == FREQ_MODE_DMA

    dma_hist.clear();
    int32_t base = dma_mem[0];
    dma_mem[0] = 0;
    for (unsigned int i = 1; i < sizeof(dma_mem)/sizeof(dma_mem[0]); ++i) {
        dma_hist.add(dma_mem[i] - base);
        base = dma_mem[i];
        dma_mem[i] = 0;
    }

    int key = 0;
    int key_value_max = 0;
    for (const auto &kv: dma_hist) {
        if (kv.second >= key_value_max) {
            key_value_max = kv.second;
            key = kv.first;
        }
    }

    if (key >= 0) {
        measure.value = key;
    } else {
        measure.value = 0;
    }
#elif FREQ_MODE == FREQ_MODE_ONE_TIMER
    measure.value = METER_ALT_TIMER->CNT;
    METER_ALT_TIMER->CNT = 0;
#endif

    if (xTaskGetTickCount() > 2000) {
        xQueueSend(lc.xQueueLoopMeasure, &measure, 1);
    }

    return measure.value;
}


bool MainTask::StartLoopA() {
#if FREQ_MODE == FREQ_MODE_ONE_TIMER
    PrepareMeterTimer(lc::LoopName::LOOP_B);
    LOOP_TIMER->CCR1 = 1;
    LOOP_TIMER->CCR2 = UINT16_MAX;
    LOOP_TIMER->CCR3 = UINT16_MAX;
    LOOP_TIMER->CR1 |= TIM_CR1_CEN;
#else
    PrepareMeterTimer(lc::LoopName::LOOP_C);
    if (cycles_measure[0] > 0 and cycles_measure[0] < comparator_cycles) {
        MASTER_TIMER->CCR1 = (comparator_cycles - 1) - cycles_measure[0] + 1;
    }
    if (switch_all & (0x1 << 2))
        LL_GPIO_ResetOutputPin(TIM2_PORT, TIM2_CH1_PIN);
#endif
    return cycles_measure[0] > 1;
}


bool MainTask::StartLoopB() {
#if FREQ_MODE == FREQ_MODE_ONE_TIMER
    PrepareMeterTimer(lc::LoopName::LOOP_C);
    LOOP_TIMER->CCR1 = UINT16_MAX;
    LOOP_TIMER->CCR2 = 1;
    LOOP_TIMER->CCR3 = UINT16_MAX;
    LOOP_TIMER->CR1 |= TIM_CR1_CEN;
#else
    PrepareMeterTimer(lc::LoopName::LOOP_A);
    if (cycles_measure[1] > 0 and cycles_measure[1] < comparator_cycles) {
        MASTER_TIMER->CCR1 = (comparator_cycles - 1) - cycles_measure[1] + 1;
    }
    if (switch_all & (0x1 << 1))
        LL_GPIO_ResetOutputPin(TIM2_PORT, TIM2_CH2_PIN);
#endif
    return cycles_measure[1] > 1;
}


bool MainTask::StartLoopC() {
#if FREQ_MODE == FREQ_MODE_ONE_TIMER
    PrepareMeterTimer(lc::LoopName::LOOP_A);
    LOOP_TIMER->CCR1 = UINT16_MAX;
    LOOP_TIMER->CCR2 = UINT16_MAX;
    LOOP_TIMER->CCR3 = 1;
    LOOP_TIMER->CR1 |= TIM_CR1_CEN;
#else
    PrepareMeterTimer(lc::LoopName::LOOP_B);
    if (cycles_measure[2] > 0 and cycles_measure[2] < comparator_cycles) {
        MASTER_TIMER->CCR1 = (comparator_cycles - 1) - cycles_measure[2] + 1;
    }
    if (switch_all & (0x1 << 0))
        LL_GPIO_ResetOutputPin(TIM2_PORT, TIM2_CH3_PIN);
#endif
    return cycles_measure[2] > 1;
}


void MainTask::CheckMultiplierMessage() {
    lc::Multiplier mul;
    if (xQueueReceive(xQueueMultiplier, &mul, 0) == pdPASS) {
        SEGGER_RTT_printf(0, MODNAME"%c) mul: %u\n", mul.loop + 'A', mul.multiplier);
        cycles_measure[mul.loop] = mul.multiplier;
    }
}

void MainTask::BarTimeoutCallback() {
    ProcessBarNext();
}

void MainTask::ProcessBarNext() {
    switch (state_led) {
    case LED_IDLE:
        break;

    case LED_BAR0:
        ShowLedBar(BAR_B);
        state_led = LED_BAR1;
        break;

    case LED_BAR1:
        ShowLedBar(BAR_C);
        state_led = LED_BAR2;
        break;

    case LED_BAR2:
        ScanSwitches();
        ShowLedBar(BAR_A);
        state_led = LED_BAR0;
        break;
    }
}

void MainTask::InitBars() {
static etl::function_mv<MainTask, &MainTask::BarTimeoutCallback> bar_to(*this);
    timer_bars = timer_controller.register_timer(bar_to, 5, true);
    timer_controller.start(timer_bars);

    // 0 - не светит, 1 - светит
    leds[BAR_A].set(0b001);
    leds[BAR_B].set(0b011);
    leds[BAR_C].set(0b111);
    SpiTransmit(0xFF);
    SpiTransmit(0xFF);
    delay_us(10);

    ReadSwitch(switch0, switch1);
    xQueueSend(lc.xQueueSettings, &switch_all, portMAX_DELAY);
    state_led = LED_BAR0;
}


void MainTask::ShowLedBar(BarNumber bar) {
    DisableBars();

    switch (bar) {
    case BAR_A:
        BAR_PORT->BRR = BAR2_o_PIN;
        break;
    case BAR_B:
        BAR_PORT->BRR = BAR1_o_PIN;
        break;
    case BAR_C:
        BAR_PORT->BRR = BAR0_o_PIN;
        break;
    default:
        assert_param(0);
    }

    auto l = leds[(int)bar];
    SpiTransmit(l.value());
}


void MainTask::ScanSwitches() {
    bool changed = ReadSwitch(switch0, switch1);
    if (changed) {
        timer_controller.start(reset_timer);
    }
}



bool MainTask::ReadSwitch(uint8_t &sw0, uint8_t &sw1) {
static uint8_t sw0_last = 0;
static uint8_t sw1_last = 0;

uint8_t moved_one = 0x01;
uint32_t l_sw0 = 0, l_sw1 = 0;

    DisableBars();
    for (int i = 0; i < 8; i++) {
        SpiTransmit(~moved_one);
        delay_us(2);

        auto pins_b = SW_PORT->IDR;
        l_sw0 |= (pins_b & SW0_i_PIN) ? 0 : 1;
        l_sw1 |= (pins_b & SW1_i_PIN) ? 0 : 1;

        moved_one <<= 1;
        l_sw0 <<= 1;
        l_sw1 <<= 1;
    }

    // l_sw сдвинуты на 1 лишний бит
    sw0 = (l_sw0 >> 1) & 0xFF;
    sw1 = (l_sw1 >> 1) & 0xFF;
    sw0 = NormalizeSwitch(sw0);
    sw1 = NormalizeSwitch(sw1);
    switch_all = (sw0 << 8) | sw1;

bool changed = (sw0_last ^ sw0) || (sw1_last ^ sw1);
    sw0_last = sw0;
    sw1_last = sw1;
    return changed;
}


uint8_t MainTask::NormalizeSwitch(uint8_t sw) {
_u8 out{}, in{};
    in.dw = sw;
    out.b7 = in.b3; //b.set(7, sw & (1 << 3));
    out.b6 = in.b2; //b.set(6, sw & (1 << 2));
    out.b5 = in.b1; //b.set(5, sw & (1 << 1));
    out.b4 = in.b0; //b.set(4, sw & (1 << 0));
    out.b3 = in.b4; //b.set(3, sw & (1 << 4));
    out.b2 = in.b5; //b.set(2, sw & (1 << 5));
    out.b1 = in.b6; //b.set(1, sw & (1 << 6));
    out.b0 = in.b7; //b.set(0, sw & (1 << 7));
    return out.dw;
}

void MainTask::CheckLedState() {
    uint32_t st;
    if (xQueueReceive(xQueueLedState, &st, 0) == pdTRUE) {
        leds[BAR_A].set_all((st & 0x0000FF) >> 0);
        leds[BAR_B].set_all((st & 0x00FF00) >> 8);
        leds[BAR_C].set_all((st & 0xFF0000) >> 16);
    }
}


void MainTask::ResetTimers() {
    MASTER_TIMER->SR = 0;
    MASTER_TIMER->CNT = 0;
    SLAVE_TIM->CNT = 0;
    SLAVE_TIM->SR = 0;
}


uint16_t timer_multiplier[3] = {1, 1, 1};
bool loop_calibrated[3] = {false, false, false};

#define LOOP_TIMEOUT_MS         10
#define LOOP_DUMMY_CICLES       100


void MainTask_(void *pvParameters) {
    auto &lc = getLC();
    mt.StartReceived();
    mt.InitBars();
    timer_controller.enable(true);

    TickType_t last_tick = xTaskGetTickCount();

    uint16_t slave_cnt;
    lc::Measure measure;

    StLoop state = StLoop::LOOP_STOP;
    uint32_t ulNotifyValue = 0;
    for (;;) {
        auto event = xTaskNotifyWait(0, UINT32_MAX, &ulNotifyValue, 1);

        switch (state) {
        case StLoop::LOOP_STOP: {
            MainTask::InitLoopTimer();

            MainTask::ResetTimers();
            MASTER_TIMER->ARR = timer_multiplier[0] + LOOP_DUMMY_CICLES;
            MASTER_TIMER->CCR1 = MASTER_TIMER->ARR - timer_multiplier[0] + 1;
            state = StLoop::LOOP_A_EN;
            timer_controller.set_period(timer_loop_timeout, LOOP_TIMEOUT_MS);
            timer_controller.start(timer_loop_timeout);
            LL_GPIO_ResetOutputPin(LOOP_ABC_EN_PORT, LOOP_A_EN_PIN);
            break;
        }

        case StLoop::LOOP_A_EN:
            if (event == pdTRUE) {
                measure.loop = lc::LoopName::LOOP_A;
                measure.multiplier = 1;

                if (ulNotifyValue & Notify::SlaveTimOverflow) {
                }
                if (ulNotifyValue & Notify::Interrupt) {
                    timer_controller.stop(timer_loop_timeout);
                    slave_cnt = SLAVE_TIM->CNT;
                    measure.value = slave_cnt;
                    xQueueSend(lc.xQueueLoopMeasure, &measure, 1);
                }
                if (ulNotifyValue & Notify::Timeout) {
                    LL_GPIO_SetOutputPin(LOOP_ABC_EN_PORT, LOOP_A_EN_PIN | LOOP_B_EN_PIN | LOOP_C_EN_PIN);
                    measure.value = 0;
                    xQueueSend(lc.xQueueLoopMeasure, &measure, 1);
                }

                MainTask::ResetTimers();
                MASTER_TIMER->ARR = timer_multiplier[1] + LOOP_DUMMY_CICLES;
                MASTER_TIMER->CCR1 = MASTER_TIMER->ARR - timer_multiplier[1] + 1;
                state = StLoop::LOOP_B_EN;
                timer_controller.set_period(timer_loop_timeout, LOOP_TIMEOUT_MS);
                timer_controller.start(timer_loop_timeout);
                LL_GPIO_ResetOutputPin(LOOP_ABC_EN_PORT, LOOP_B_EN_PIN);
            }
            break;

        case StLoop::LOOP_B_EN:
            if (event == pdTRUE) {
                measure.loop = lc::LoopName::LOOP_B;
                measure.multiplier = 1;
                if (ulNotifyValue & Notify::SlaveTimOverflow) {
                }
                if (ulNotifyValue & Notify::Interrupt) {
                    timer_controller.stop(timer_loop_timeout);
                    slave_cnt = SLAVE_TIM->CNT;
                    measure.value = slave_cnt;
                    xQueueSend(lc.xQueueLoopMeasure, &measure, 1);
                }
                if (ulNotifyValue & Notify::Timeout) {
                    LL_GPIO_SetOutputPin(LOOP_ABC_EN_PORT, LOOP_A_EN_PIN | LOOP_B_EN_PIN | LOOP_C_EN_PIN);
                    measure.value = 0;
                    xQueueSend(lc.xQueueLoopMeasure, &measure, 1);
                }

                MainTask::ResetTimers();
                MASTER_TIMER->ARR = timer_multiplier[2] + LOOP_DUMMY_CICLES;
                MASTER_TIMER->CCR1 = MASTER_TIMER->ARR - timer_multiplier[2] + 1;
                state = StLoop::LOOP_C_EN;
                timer_controller.set_period(timer_loop_timeout, LOOP_TIMEOUT_MS);
                timer_controller.start(timer_loop_timeout);
                LL_GPIO_ResetOutputPin(LOOP_ABC_EN_PORT, LOOP_C_EN_PIN);
            }
            break;

        case StLoop::LOOP_C_EN:
            if (event == pdTRUE) {
                measure.loop = lc::LoopName::LOOP_C;
                measure.multiplier = 1;
                if (ulNotifyValue & Notify::SlaveTimOverflow) {
                }
                if (ulNotifyValue & Notify::Interrupt) {
                    timer_controller.stop(timer_loop_timeout);
                    slave_cnt = SLAVE_TIM->CNT;
                    measure.value = slave_cnt;
                    xQueueSend(lc.xQueueLoopMeasure, &measure, 1);
                }
                if (ulNotifyValue & Notify::Timeout) {
                    LL_GPIO_SetOutputPin(LOOP_ABC_EN_PORT, LOOP_A_EN_PIN | LOOP_B_EN_PIN | LOOP_C_EN_PIN);
                    measure.value = 0;
                    xQueueSend(lc.xQueueLoopMeasure, &measure, 1);
                }

                MainTask::ResetTimers();
                MASTER_TIMER->ARR = timer_multiplier[0] + LOOP_DUMMY_CICLES;
                MASTER_TIMER->CCR1 = MASTER_TIMER->ARR - timer_multiplier[0] + 1;
                state = StLoop::LOOP_A_EN;
                timer_controller.start(timer_loop_timeout);
                LL_GPIO_ResetOutputPin(LOOP_ABC_EN_PORT, LOOP_A_EN_PIN);
            }
            break;
        }

        mt.CheckLedState();

        lc::Multiplier mul;
        if (xQueueReceive(mt.xQueueMultiplier, &mul, 0) == pdPASS) {
            SEGGER_RTT_printf(0, MODNAME"%c) mul: %u\n", mul.loop + 'A', mul.multiplier);
            timer_multiplier[mul.loop] = mul.multiplier;
            loop_calibrated[mul.loop] = true;
        }

        TickType_t current_tick = xTaskGetTickCount();
        timer_controller.tick(current_tick - last_tick);
        last_tick = current_tick;
    }
}



void StartMainTask() {
    getLC().InitObjects();
    MainTaskHandle = xTaskCreateStatic(MainTask_, "Main", st::TaskStackSize, nullptr, st::TaskPriority, st::ucTaskStack, &st::xTCBTask);
}


static void SpiTransmit(uint8_t data) {
    // Ждем пока флаг BSY в 1, SPI занят
    while (SPI1->SR & SPI_SR_BSY) {}
    SPI1_CS_PORT->BRR = SPI1_CS_PIN;
    SPI1->DR = data;
    while (!(SPI1->SR & SPI_SR_TXE)) {}
    // Ждем пока не опустится флаг BSY
    while (SPI1->SR & SPI_SR_BSY) {}
    SPI1_CS_PORT->BSRR = SPI1_CS_PIN;
}



#if FREQ_MODE == FREQ_MODE_TWO_TIMER
extern "C" void TIM21_IRQHandler() {
BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (MASTER_TIMER->SR & TIM_SR_UIF) {
        LL_GPIO_SetOutputPin(TIM2_PORT, TIM2_CH1_PIN | TIM2_CH2_PIN | TIM2_CH3_PIN); // заканчиваем трейн и отключаем генератор
        xTaskNotifyFromISR(MainTaskHandle, Notify::Interrupt, eSetBits, &xHigherPriorityTaskWoken);
    }
    MASTER_TIMER->SR = 0;
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

extern "C" void TIM2_IRQHandler() {
BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (SLAVE_TIM->SR & TIM_SR_UIF) {
        xTaskNotifyFromISR(MainTaskHandle, Notify::SlaveTimOverflow, eSetBits, &xHigherPriorityTaskWoken);
    }
    SLAVE_TIM->SR = 0;
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
#endif
