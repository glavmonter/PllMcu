#include "stm32f0xx_hal.h"
#include "stm32f0xx_ll_gpio.h"
#include "stm32f0xx_ll_spi.h"
#include "stm32f0xx_ll_bus.h"
#include "stm32f0xx_ll_usart.h"
#include "MainTask.h"
#include "SEGGER_RTT.h"
#include "etl/callback_timer.h"
#include "etl/function.h"
#include "etl/histogram.h"
#include "hardware.h"
#include "common.h"
#include "config.h"
#include "printf.h"
#include <cstdarg>
#include <semphr.h>
#include "FreeRTOS_CLI.h"

#include "log_levels.h"
#undef LOG_LOCAL_LEVEL
#define LOG_LOCAL_LEVEL LOG_TAG_MAIN_LEVEL
#include <rtt_log.h>
static const char *TAG = "   MAIN";

TaskHandle_t MainTaskHandle = nullptr;
QueueHandle_t xCommandLineCharsQueue = nullptr;
static StaticQueue_t xStaticCommandLineCharsQueue;
uint8_t ucCommandLineCharsQueueBuffer[128];

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
static char cInputBuffer[configCOMMAND_INT_MAX_OUTPUT_SIZE];
static char cOutputBuffer[configCOMMAND_INT_MAX_OUTPUT_SIZE];


static BaseType_t prvEchoCmd(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString) {
    const char *pcParameter;
    BaseType_t xParameterStringLength;

    pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, 1, &xParameterStringLength);

    if (xParameterStringLength > 0) {
        strncpy(pcWriteBuffer, pcParameter, xParameterStringLength);
        pcWriteBuffer[xParameterStringLength] = '\0';
    } else {
        strncpy(pcWriteBuffer, "No parameter provided", xWriteBufferLen);
        pcWriteBuffer[xWriteBufferLen - 1] = '\0';
    }

    return pdFALSE;
}

static const CLI_Command_Definition_t xEchoCommand = {
    "echo",
    "\r\necho <text>:\r\n Echoes the provided text back to the console.\r\n\r\n",
    prvEchoCmd,
    1
};

void MainTask(void *pvParameters) {
    TickType_t last_tick = xTaskGetTickCount();
    RTT_LOGI(TAG, "MainTask started");
    BaseType_t xMore;

    int32_t inputIndex = 0;
    xCommandLineCharsQueue = xQueueCreateStatic(sizeof(ucCommandLineCharsQueueBuffer), sizeof(uint8_t), ucCommandLineCharsQueueBuffer, &xStaticCommandLineCharsQueue);
    FreeRTOS_CLIRegisterCommand(&xEchoCommand);

    UART_Printf("\r\nPLL controller starting\n");

    LL_USART_EnableIT_RXNE(SERIAL_UART);
    for (;;) {
        uint8_t received_char;
        if (xQueueReceive(xCommandLineCharsQueue, &received_char, 100) == pdTRUE) {
            UART_Transmit((const char *)&received_char, 1);

            if (received_char == '\r' || received_char == '\n') {
                UART_Printf("\r\n");
                cInputBuffer[inputIndex] = '\0';
                if (inputIndex > 0) {
                    do {
                        xMore = FreeRTOS_CLIProcessCommand(cInputBuffer, cOutputBuffer, sizeof(cOutputBuffer));
                        UART_Transmit(cOutputBuffer, strlen(cOutputBuffer));
                    } while (xMore != pdFALSE);
                }
                inputIndex = 0;
                UART_Printf("> ");
            } else if (received_char == '\b' || received_char == 0x7F) {
                if (inputIndex > 0) {
                    inputIndex--;
                    UART_Printf("\b \b");
                }
            } else if (inputIndex < static_cast<int32_t>(sizeof(cInputBuffer) - 1)) {
                cInputBuffer[inputIndex++] = received_char;
            }
        }
    }
}

void StartMainTask() {
    MainTaskHandle = xTaskCreateStatic(MainTask, "Main", st::TaskStackSize, nullptr, st::TaskPriority, st::ucTaskStack, &st::xTCBTask);
}


void UART_Transmit(const char *data, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        while (!LL_USART_IsActiveFlag_TXE(SERIAL_UART)) {}
        LL_USART_TransmitData8(SERIAL_UART, static_cast<uint8_t>(data[i]));
    }
}

void UART_Printf(const char *format, ...) {
    char buffer[128];

    va_list va;
    va_start(va, format);
    int length = vsnprintf(buffer, sizeof(buffer), format, va);
    va_end(va);

    if (length <= 0) {
        return;
    }
    if (length >= static_cast<int>(sizeof(buffer))) {
        length = sizeof(buffer) - 1;
    }

    for (int i = 0; i < length; ++i) {
        while (!LL_USART_IsActiveFlag_TXE(SERIAL_UART)) {}
        LL_USART_TransmitData8(SERIAL_UART, static_cast<uint8_t>(buffer[i]));
    }
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


extern "C" void USART1_IRQHandler(void) {
BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (LL_USART_IsActiveFlag_RXNE(SERIAL_UART)) {
        uint8_t received = LL_USART_ReceiveData8(SERIAL_UART);
        xQueueSendFromISR(xCommandLineCharsQueue, &received, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
