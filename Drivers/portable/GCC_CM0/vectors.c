#include "exception_handlers.h"

void Default_Handler (void) __attribute__((weak));

/* STM32F051 Specific Interrupts */
void WWDG_IRQHandler                    (void) __attribute__ ((weak, alias("Default_Handler")));
void PVD_IRQHandler                     (void) __attribute__ ((weak, alias("Default_Handler")));
void RTC_IRQHandler                     (void) __attribute__ ((weak, alias("Default_Handler")));
void FLASH_IRQHandler                   (void) __attribute__ ((weak, alias("Default_Handler")));
void RCC_CRS_IRQHandler                 (void) __attribute__ ((weak, alias("Default_Handler")));
void EXTI0_1_IRQHandler                 (void) __attribute__ ((weak, alias("Default_Handler")));
void EXTI2_3_IRQHandler                 (void) __attribute__ ((weak, alias("Default_Handler")));
void EXTI4_15_IRQHandler                (void) __attribute__ ((weak, alias("Default_Handler")));
void TSC_IRQHandler                     (void) __attribute__ ((weak, alias("Default_Handler")));
void DMA1_Channel1_IRQHandler           (void) __attribute__ ((weak, alias("Default_Handler")));
void DMA1_Channel2_3_IRQHandler         (void) __attribute__ ((weak, alias("Default_Handler")));
void DMA1_Channel4_5_IRQHandler         (void) __attribute__ ((weak, alias("Default_Handler")));
void ADC1_COMP_IRQHandler               (void) __attribute__ ((weak, alias("Default_Handler")));
void TIM1_BRK_UP_TRG_COM_IRQHandler     (void) __attribute__ ((weak, alias("Default_Handler")));
void TIM1_CC_IRQHandler                 (void) __attribute__ ((weak, alias("Default_Handler")));
void TIM2_IRQHandler                    (void) __attribute__ ((weak, alias("Default_Handler")));
void TIM3_IRQHandler                    (void) __attribute__ ((weak, alias("Default_Handler")));
void TIM6_DAC_IRQHandler                (void) __attribute__ ((weak, alias("Default_Handler")));
void TIM14_IRQHandler                   (void) __attribute__ ((weak, alias("Default_Handler")));
void TIM15_IRQHandler                   (void) __attribute__ ((weak, alias("Default_Handler")));
void TIM16_IRQHandler                   (void) __attribute__ ((weak, alias("Default_Handler")));
void TIM17_IRQHandler                   (void) __attribute__ ((weak, alias("Default_Handler")));
void I2C1_IRQHandler                    (void) __attribute__ ((weak, alias("Default_Handler")));
void I2C2_IRQHandler                    (void) __attribute__ ((weak, alias("Default_Handler")));
void SPI1_IRQHandler                    (void) __attribute__ ((weak, alias("Default_Handler")));
void SPI2_IRQHandler                    (void) __attribute__ ((weak, alias("Default_Handler")));
void USART1_IRQHandler                  (void) __attribute__ ((weak, alias("Default_Handler")));
void USART2_IRQHandler                  (void) __attribute__ ((weak, alias("Default_Handler")));
void CEC_CAN_IRQHandler                 (void) __attribute__ ((weak, alias("Default_Handler")));

extern unsigned int __stack;

typedef void (*const pHandler)(void);

// The vector table.
// This relies on the linker script to place at correct location in memory.

pHandler __isr_vectors[] __attribute__ ((section(".isr_vector"),used)) =  {
        (pHandler) &__stack,                    // The initial stack pointer
             Reset_Handler,
     NMI_Handler,
     HardFault_Handler,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     SVC_Handler,
     0,
     0,
     PendSV_Handler,
     SysTick_Handler,
     WWDG_IRQHandler,                   /* Window WatchDog              */
     PVD_IRQHandler,                    /* PVD through EXTI Line detect */
     RTC_IRQHandler,                    /* RTC through the EXTI line    */
     FLASH_IRQHandler,                  /* FLASH                        */
     RCC_CRS_IRQHandler,                /* RCC and CRS                  */
     EXTI0_1_IRQHandler,                /* EXTI Line 0 and 1            */
     EXTI2_3_IRQHandler,                /* EXTI Line 2 and 3            */
     EXTI4_15_IRQHandler,               /* EXTI Line 4 to 15            */
     TSC_IRQHandler,                    /* TSC                          */
     DMA1_Channel1_IRQHandler,          /* DMA1 Channel 1               */
     DMA1_Channel2_3_IRQHandler,        /* DMA1 Channel 2 and Channel 3 */
     DMA1_Channel4_5_IRQHandler,        /* DMA1 Channel 4 and Channel 5 */
     ADC1_COMP_IRQHandler,              /* ADC1, COMP1 and COMP2         */
     TIM1_BRK_UP_TRG_COM_IRQHandler,    /* TIM1 Break, Update, Trigger and Commutation */
     TIM1_CC_IRQHandler,                /* TIM1 Capture Compare         */
     TIM2_IRQHandler,                   /* TIM2                         */
     TIM3_IRQHandler,                   /* TIM3                         */
     TIM6_DAC_IRQHandler,               /* TIM6 and DAC                 */
     0,                                 /* Reserved                     */
     TIM14_IRQHandler,                  /* TIM14                        */
     TIM15_IRQHandler,                  /* TIM15                        */
     TIM16_IRQHandler,                   /* TIM16                        */
     TIM17_IRQHandler,                  /* TIM17                        */
     I2C1_IRQHandler,                   /* I2C1                         */
     I2C2_IRQHandler,                   /* I2C2                         */
     SPI1_IRQHandler,                   /* SPI1                         */
     SPI2_IRQHandler,                   /* SPI2                         */
     USART1_IRQHandler,                 /* USART1                       */
     USART2_IRQHandler,                 /* USART2                       */
     0,                                 /* Reserved                     */
     CEC_CAN_IRQHandler,                /* CEC and CAN                  */
     0,                                 /* Reserved                     */
};

// Processor ends up here if an unexpected interrupt occurs or a specific
// handler is not present in the application code.
__attribute__ ((section(".after_vectors")))
void Default_Handler (void)
{
	while (1) ;
}
