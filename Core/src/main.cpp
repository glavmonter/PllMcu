#include <cstring>
#include "stm32l0xx_hal.h"
#include "stm32l0xx_ll_usart.h"
#include "SEGGER_RTT.h"

#include "main.h"
#include "boost/sml.hpp"
#include "FreeRTOS.h"
#include "task.h"
#include "MainTask.h"
#include "etl/vector.h"
#include "config.h"
#include "hardware.h"
#include "templates.h"
#include "common.h"


/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config();
static void MX_GPIO_Init();
static void MX_SPI_Init();
static void MX_USART_Init();


int main(int argc, char *argv[]) {
UNUSED(argv);

    SystemClock_Config();
    SEGGER_RTT_ConfigUpBuffer(0, NULL, NULL, 0, SEGGER_RTT_MODE_NO_BLOCK_SKIP);
    SEGGER_RTT_printf(0, "\n\n======================== Inductive Loop Controller ===========================\n");

    HAL_Init();
    MX_GPIO_Init();
    MX_SPI_Init();
    MX_USART_Init();

    StartMainTask();
    vTaskStartScheduler();
    for (;;) {
        __NOP();
    }
    return 0;
}


extern "C" HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority) {
    return HAL_OK;
}

void SystemClock_Config() {
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_1);
    while (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_1) {}

    LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
    LL_RCC_HSE_Enable();
    while (LL_RCC_HSE_IsReady() != 1) {}

    LL_RCC_LSI_Enable();
    while (LL_RCC_LSI_IsReady() != 1) {}

#if HW_PLATFORM == PLATFORM_CHINA
    LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLL_MUL_8, LL_RCC_PLL_DIV_3);
#elif HW_PLATFORM == PLATFORM_EPCOS
    LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLL_MUL_4, LL_RCC_PLL_DIV_2);
#endif

    LL_RCC_PLL_Enable();
    while (LL_RCC_PLL_IsReady() != 1) {}

    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);

    while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL) {}

    LL_SetSystemCoreClock(32000000);
#ifdef MODBUS
    LL_RCC_SetUSARTClockSource(LL_RCC_USART1_CLKSOURCE_PCLK2);
#endif
    LL_RCC_SetUSARTClockSource(LL_RCC_USART1_CLKSOURCE_PCLK2);
}


/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init() {
LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);

    LL_GPIO_SetOutputPin(SPI1_CS_PORT, SPI1_CS_PIN);

    GPIO_InitStruct.Pin = SPI1_CS_PIN;
    LL_GPIO_Init(SPI1_CS_PORT, &GPIO_InitStruct);

    // SPI1 MOSI and SCK
    GPIO_InitStruct.Pin = SPI1_MOSI_PIN | SPI1_SCK_PIN;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = SPI1_GPIO_AF;
    LL_GPIO_Init(SPI1_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;

    GPIO_InitStruct.Pin = MODBUS_TX_PIN;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
    GPIO_InitStruct.Alternate = MODBUS_TX_GPIO_AF;
    LL_GPIO_Init(MODBUS_TX_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = MODBUS_RX_PIN;
    GPIO_InitStruct.Alternate = MODBUS_RX_GPIO_AF;
    LL_GPIO_Init(MODBUS_RX_PORT, &GPIO_InitStruct);
}

static void MX_USART_Init() {
    __HAL_RCC_USART1_CLK_ENABLE();

    LL_USART_SetTransferDirection(MODBUS_UART, LL_USART_DIRECTION_TX_RX);
    LL_USART_ConfigCharacter(MODBUS_UART, LL_USART_DATAWIDTH_8B, LL_USART_PARITY_NONE, LL_USART_STOPBITS_1);
    LL_USART_SetHWFlowCtrl(MODBUS_UART, LL_USART_HWCONTROL_NONE);
    LL_USART_SetOverSampling(MODBUS_UART, LL_USART_OVERSAMPLING_16);
    LL_USART_SetBaudRate(MODBUS_UART, SystemCoreClock, LL_USART_OVERSAMPLING_16, 256000);

    LL_USART_EnableDEMode(MODBUS_UART);
    LL_USART_SetDESignalPolarity(MODBUS_UART, LL_USART_DE_POLARITY_HIGH);
    LL_USART_SetDEAssertionTime(MODBUS_UART, 8);
    LL_USART_SetDEDeassertionTime(MODBUS_UART, 8);
    LL_USART_ConfigAsyncMode(MODBUS_UART);
    LL_USART_Enable(MODBUS_UART);

    NVIC_SetPriority(USART1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 6, 0));
    NVIC_EnableIRQ(USART1_IRQn);
}


static void MX_SPI_Init() {
LL_SPI_InitTypeDef SPI_InitStruct = {0};

    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SPI1);

    SPI_InitStruct.TransferDirection = LL_SPI_FULL_DUPLEX;
    SPI_InitStruct.Mode = LL_SPI_MODE_MASTER;
    SPI_InitStruct.DataWidth = LL_SPI_DATAWIDTH_8BIT;
    SPI_InitStruct.ClockPolarity = LL_SPI_POLARITY_LOW;
    SPI_InitStruct.ClockPhase = LL_SPI_PHASE_1EDGE;
    SPI_InitStruct.NSS = LL_SPI_NSS_SOFT;
    SPI_InitStruct.BaudRate = LL_SPI_BAUDRATEPRESCALER_DIV4;
    SPI_InitStruct.BitOrder = LL_SPI_MSB_FIRST;
    SPI_InitStruct.CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE;
    SPI_InitStruct.CRCPoly = 7;
    LL_SPI_Init(SPI1, &SPI_InitStruct);
    LL_SPI_SetStandard(SPI1, LL_SPI_PROTOCOL_MOTOROLA);
    LL_SPI_Enable(SPI1);

}

void Error_Handler() {
    __disable_irq();
    while (true) { }
}



#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line) {
    __disable_irq();
    for (;;) {
        __NOP();
    }
}
#endif /* USE_FULL_ASSERT */


/* configSUPPORT_STATIC_ALLOCATION is set to 1, so the application must provide an
implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
used by the Idle task. */
extern "C" void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize )
{
/* If the buffers to be provided to the Idle task are declared inside this
function then they must be declared static - otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
    state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/*-----------------------------------------------------------*/

void vApplicationTickHook() {
    HAL_IncTick();
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    SEGGER_RTT_printf(0, "[ERR ] Stack Overflow at %s\n", pcTaskName);
    for (;;) {}
}

void vApplicationMallocFailedHook() {
    SEGGER_RTT_printf(0, "[ERR ] Malloc failed\n");
    for (;;) {}
}
