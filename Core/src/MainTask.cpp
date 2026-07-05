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
#include <cstdlib>
#include <cstring>
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
    "\r\necho <text>:\r\n Echoes the provided text back to the console.\r\n",
    prvEchoCmd,
    1
};

static void WriteRegister(uint8_t reg, uint32_t value) {
    (void) reg;
    const uint8_t bytes[3] = {
        static_cast<uint8_t>((value >> 16) & 0xFF),
        static_cast<uint8_t>((value >> 8) & 0xFF),
        static_cast<uint8_t>(value & 0xFF)
    };

    while (LL_SPI_IsActiveFlag_BSY(SPI1)) {}
    LMX_CS_PORT->BRR = LMX_CS_PIN;
    for (uint8_t byte : bytes) {
        LL_SPI_TransmitData8(SPI1, byte);
        while (!(LL_SPI_IsActiveFlag_TXE(SPI1))) {}
        while (!(LL_SPI_IsActiveFlag_RXNE(SPI1))) {}
        LL_SPI_ReceiveData8(SPI1);
    }
    while (LL_SPI_IsActiveFlag_BSY(SPI1)) {}
    LMX_CS_PORT->BSRR = LMX_CS_PIN;
}

static BaseType_t prvWriteRegisterCmd(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString) {
    BaseType_t xRegLen, xValueLen;
    const char *pcRegParam = FreeRTOS_CLIGetParameter(pcCommandString, 1, &xRegLen);
    const char *pcValueParam = FreeRTOS_CLIGetParameter(pcCommandString, 2, &xValueLen);

    char regBuf[16];
    size_t regLen = static_cast<size_t>(xRegLen) < sizeof(regBuf) - 1 ? static_cast<size_t>(xRegLen) : sizeof(regBuf) - 1;
    memcpy(regBuf, pcRegParam, regLen);
    regBuf[regLen] = '\0';

    char valueBuf[16];
    size_t valueLen = static_cast<size_t>(xValueLen) < sizeof(valueBuf) - 1 ? static_cast<size_t>(xValueLen) : sizeof(valueBuf) - 1;
    memcpy(valueBuf, pcValueParam, valueLen);
    valueBuf[valueLen] = '\0';

    const char *pcRegNumber = regBuf;
    if (*pcRegNumber == 'R' || *pcRegNumber == 'r') {
        ++pcRegNumber;
    }

    char *pcEnd = nullptr;
    unsigned long regNumber = strtoul(pcRegNumber, &pcEnd, 10);
    if (pcEnd == pcRegNumber) {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Invalid register '%s'\r\n", regBuf);
        return pdFALSE;
    }

    unsigned long value = strtoul(valueBuf, &pcEnd, 0);
    if (pcEnd == valueBuf) {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Invalid value '%s'\r\n", valueBuf);
        return pdFALSE;
    }

    RTT_LOGI(TAG, "Writing 0x%06lX to register R%lu", value & 0xFFFFFFUL, regNumber);

    WriteRegister(static_cast<uint8_t>(regNumber), static_cast<uint32_t>(value));

    snprintf(pcWriteBuffer, xWriteBufferLen, "R%lu = 0x%06lX\r\n", regNumber, value & 0xFFFFFFUL);
    return pdFALSE;
}

static const CLI_Command_Definition_t xWriteRegisterCommand = {
    "wr",
    "\r\nwr <register> <value>:\r\n Writes value to a PLL register, e.g. wr R43 0x2B0000 or wr 43 0x2B0000\r\n",
    prvWriteRegisterCmd,
    2
};

static uint32_t ReadRegister(uint8_t reg) {
    uint8_t rx[3] = {0};
    const uint8_t tx[3] = {
        static_cast<uint8_t>(0x80 | (reg & 0x7F)),
        0x00,
        0x00
    };

    while (LL_SPI_IsActiveFlag_BSY(SPI1)) {}
    LMX_CS_PORT->BRR = LMX_CS_PIN;
    for (uint8_t i = 0; i < 3; ++i) {
        LL_SPI_TransmitData8(SPI1, tx[i]);
        while (!(LL_SPI_IsActiveFlag_TXE(SPI1))) {}
        while (!(LL_SPI_IsActiveFlag_RXNE(SPI1))) {}
        rx[i] = LL_SPI_ReceiveData8(SPI1);
    }
    while (LL_SPI_IsActiveFlag_BSY(SPI1)) {}
    LMX_CS_PORT->BSRR = LMX_CS_PIN;

    return (static_cast<uint32_t>(rx[1]) << 8) | rx[2];
}

static BaseType_t prvReadRegisterCmd(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString) {
    BaseType_t xRegLen;
    const char *pcRegParam = FreeRTOS_CLIGetParameter(pcCommandString, 1, &xRegLen);

    char regBuf[16];
    size_t regLen = static_cast<size_t>(xRegLen) < sizeof(regBuf) - 1 ? static_cast<size_t>(xRegLen) : sizeof(regBuf) - 1;
    memcpy(regBuf, pcRegParam, regLen);
    regBuf[regLen] = '\0';

    const char *pcRegNumber = regBuf;
    if (*pcRegNumber == 'R' || *pcRegNumber == 'r') {
        ++pcRegNumber;
    }

    char *pcEnd = nullptr;
    unsigned long regNumber = strtoul(pcRegNumber, &pcEnd, 10);
    if (pcEnd == pcRegNumber) {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Invalid register '%s'\r\n", regBuf);
        return pdFALSE;
    }

    uint32_t value = ReadRegister(static_cast<uint8_t>(regNumber));

    RTT_LOGI(TAG, "Read 0x%04lX from register R%lu", static_cast<unsigned long>(value), regNumber);

    snprintf(pcWriteBuffer, xWriteBufferLen, "R%lu = 0x%04lX\r\n", regNumber, static_cast<unsigned long>(value));
    return pdFALSE;
}

static const CLI_Command_Definition_t xReadRegisterCommand = {
    "rr",
    "\r\nrr <register>:\r\n Reads a PLL register over SPI, e.g. rr R43 or rr 43\r\n",
    prvReadRegisterCmd,
    1
};

static BaseType_t prvEnableCmd(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString) {
    (void) pcCommandString;
    LL_GPIO_SetOutputPin(LMX_ENABLE_PORT, LMX_ENABLE_PIN);
    snprintf(pcWriteBuffer, xWriteBufferLen, "LMX enabled\r\n");
    return pdFALSE;
}

static const CLI_Command_Definition_t xEnableCommand = {
    "enable",
    "\r\nenable:\r\n Enables the LMX2592 (LMX_ENABLE_PIN high).\r\n",
    prvEnableCmd,
    0
};

static BaseType_t prvDisableCmd(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString) {
    (void) pcCommandString;
    LL_GPIO_ResetOutputPin(LMX_ENABLE_PORT, LMX_ENABLE_PIN);
    snprintf(pcWriteBuffer, xWriteBufferLen, "LMX disabled\r\n");
    return pdFALSE;
}

static const CLI_Command_Definition_t xDisableCommand = {
    "disable",
    "\r\ndisable:\r\n Disables the LMX2592 (LMX_ENABLE_PIN low, power down).\r\n",
    prvDisableCmd,
    0
};

static BaseType_t prvResetCmd(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString) {
    (void) pcCommandString;
    if (!LL_GPIO_IsOutputPinSet(LMX_ENABLE_PORT, LMX_ENABLE_PIN)) {
        snprintf(pcWriteBuffer, xWriteBufferLen, "LMX is already disabled. Enable it first.\r\n");
        return pdFALSE;
    }

    LL_GPIO_ResetOutputPin(LMX_ENABLE_PORT, LMX_ENABLE_PIN);
    vTaskDelay(pdMS_TO_TICKS(10));
    LL_GPIO_SetOutputPin(LMX_ENABLE_PORT, LMX_ENABLE_PIN);
    snprintf(pcWriteBuffer, xWriteBufferLen, "LMX reset\r\n");
    return pdFALSE;
}

static const CLI_Command_Definition_t xResetCommand = {
    "reset",
    "\r\nreset:\r\n Resets the LMX2592 by power-cycling LMX_ENABLE_PIN.\r\n",
    prvResetCmd,
    0
};

static BaseType_t prvLockCmd(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString) {
    (void) pcCommandString;
    uint32_t locked = LL_GPIO_IsInputPinSet(LMX_LOCK_PORT, LMX_LOCK_PIN);
    snprintf(pcWriteBuffer, xWriteBufferLen, "LockDetect: %s\r\n", locked ? "LOCKED" : "UNLOCKED");
    return pdFALSE;
}

static const CLI_Command_Definition_t xLockCommand = {
    "lock",
    "\r\nlock:\r\n Reads the LMX2592 LockDetect pin state.\r\n",
    prvLockCmd,
    0
};

void MainTask(void *pvParameters) {
UNUSED(pvParameters);
    RTT_LOGI(TAG, "MainTask started");
    BaseType_t xMore;

    int32_t inputIndex = 0;
    xCommandLineCharsQueue = xQueueCreateStatic(sizeof(ucCommandLineCharsQueueBuffer), sizeof(uint8_t), ucCommandLineCharsQueueBuffer, &xStaticCommandLineCharsQueue);
    FreeRTOS_CLIRegisterCommand(&xEchoCommand);
    FreeRTOS_CLIRegisterCommand(&xWriteRegisterCommand);
    FreeRTOS_CLIRegisterCommand(&xReadRegisterCommand);
    FreeRTOS_CLIRegisterCommand(&xEnableCommand);
    FreeRTOS_CLIRegisterCommand(&xDisableCommand);
    FreeRTOS_CLIRegisterCommand(&xResetCommand);
    FreeRTOS_CLIRegisterCommand(&xLockCommand);

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

extern "C" void USART1_IRQHandler(void) {
BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (LL_USART_IsActiveFlag_RXNE(SERIAL_UART)) {
        uint8_t received = LL_USART_ReceiveData8(SERIAL_UART);
        xQueueSendFromISR(xCommandLineCharsQueue, &received, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
