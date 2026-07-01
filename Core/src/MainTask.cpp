#include "stm32f0xx_hal.h"
#include "stm32f0xx_ll_gpio.h"
#include "stm32f0xx_ll_spi.h"
#include "stm32f0xx_ll_dma.h"
#include "stm32f0xx_ll_bus.h"
#include "MainTask.h"
#include "SEGGER_RTT.h"
#include "etl/callback_timer.h"
#include "etl/function.h"
#include "etl/histogram.h"
#include "hardware.h"
#include "common.h"
#include "config.h"
#include <semphr.h>

#include "log_levels.h"
#undef LOG_LOCAL_LEVEL
#define LOG_LOCAL_LEVEL LOG_TAG_MAIN_LEVEL
#include <rtt_log.h>
static const char *TAG = "   MAIN";


static MainTask mt;
TaskHandle_t MainTaskHandle = nullptr;

namespace rtos_static {
    namespace maintask {
        constexpr size_t TaskStackSize = configMINIMAL_STACK_SIZE * 3;
        constexpr BaseType_t TaskPriority = configMAX_PRIORITIES - 1;

        static StackType_t ucTaskStack[TaskStackSize];
        static StaticTask_t xTCBTask;
    }
}

namespace st = rtos_static::maintask;

static void SpiTransmit(uint8_t data);


void MainTask_(void *pvParameters) {
    TickType_t last_tick = xTaskGetTickCount();
    RTT_LOGI(TAG, "MainTask started");

    uint32_t ulNotifyValue = 0;

    int counter = 0;
    for (;;) {
        RTT_LOGD(TAG, "MainTask loop %d", counter++);
        vTaskDelay(1000);
    }
}

void StartMainTask() {
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
