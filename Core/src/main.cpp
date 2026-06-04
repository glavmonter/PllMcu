#include <cstring>
#include "stm32f0xx_hal.h"
#include "stm32f0xx_ll_usart.h"
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
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
    RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
    {
        Error_Handler();
    }
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
    PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
        Error_Handler();
    }
}


/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init() {

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
    /* USER CODE BEGIN SPI1_Init 0 */

    /* USER CODE END SPI1_Init 0 */

    LL_SPI_InitTypeDef SPI_InitStruct = {0};

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Peripheral clock enable */
    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_SPI1);

    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
    /**SPI1 GPIO Configuration
     PA5   ------> SPI1_SCK
    PA6   ------> SPI1_MISO
    PA7   ------> SPI1_MOSI
    */
    GPIO_InitStruct.Pin = LL_GPIO_PIN_5;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_6;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_7;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USER CODE BEGIN SPI1_Init 1 */

    /* USER CODE END SPI1_Init 1 */
    /* SPI1 parameter configuration*/
    SPI_InitStruct.TransferDirection = LL_SPI_FULL_DUPLEX;
    SPI_InitStruct.Mode = LL_SPI_MODE_MASTER;
    SPI_InitStruct.DataWidth = LL_SPI_DATAWIDTH_4BIT;
    SPI_InitStruct.ClockPolarity = LL_SPI_POLARITY_LOW;
    SPI_InitStruct.ClockPhase = LL_SPI_PHASE_1EDGE;
    SPI_InitStruct.NSS = LL_SPI_NSS_SOFT;
    SPI_InitStruct.BaudRate = LL_SPI_BAUDRATEPRESCALER_DIV4;
    SPI_InitStruct.BitOrder = LL_SPI_MSB_FIRST;
    SPI_InitStruct.CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE;
    SPI_InitStruct.CRCPoly = 7;
    LL_SPI_Init(SPI1, &SPI_InitStruct);
    LL_SPI_SetStandard(SPI1, LL_SPI_PROTOCOL_MOTOROLA);
    LL_SPI_EnableNSSPulseMgt(SPI1);
    /* USER CODE BEGIN SPI1_Init 2 */

    /* USER CODE END SPI1_Init 2 */
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
