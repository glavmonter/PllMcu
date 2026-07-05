set(CMAKE_C_FLAGS_DEBUG ${LIBRARY_OPTIMIZATION_OPTIONS})
set(CMAKE_CXX_FLAGS_DEBUG ${LIBRARY_OPTIMIZATION_OPTIONS})

set(SPL_SRC
        "STM32F0xx_HAL_Driver/src/stm32f0xx_hal.c"
        "STM32F0xx_HAL_Driver/src/stm32f0xx_hal_cortex.c"
        "STM32F0xx_HAL_Driver/src/stm32f0xx_hal_dma.c"
        "STM32F0xx_HAL_Driver/src/stm32f0xx_hal_exti.c"
        "STM32F0xx_HAL_Driver/src/stm32f0xx_hal_flash_ex.c"
        "STM32F0xx_HAL_Driver/src/stm32f0xx_hal_flash.c"
        "STM32F0xx_HAL_Driver/src/stm32f0xx_hal_gpio.c"
        "STM32F0xx_HAL_Driver/src/stm32f0xx_hal_i2c_ex.c"
        "STM32F0xx_HAL_Driver/src/stm32f0xx_hal_i2c.c"
        "STM32F0xx_HAL_Driver/src/stm32f0xx_hal_pwr_ex.c"
        "STM32F0xx_HAL_Driver/src/stm32f0xx_hal_pwr.c"
        "STM32F0xx_HAL_Driver/src/stm32f0xx_hal_rcc.c"
        "STM32F0xx_HAL_Driver/src/stm32f0xx_hal_rcc_ex.c"
        "STM32F0xx_HAL_Driver/src/stm32f0xx_hal.c"

        "STM32F0xx_HAL_Driver/src/stm32f0xx_ll_dma.c"
        "STM32F0xx_HAL_Driver/src/stm32f0xx_ll_exti.c"
        "STM32F0xx_HAL_Driver/src/stm32f0xx_ll_gpio.c"
        "STM32F0xx_HAL_Driver/src/stm32f0xx_ll_rcc.c"
        "STM32F0xx_HAL_Driver/src/stm32f0xx_ll_spi.c"
        "STM32F0xx_HAL_Driver/src/stm32f0xx_ll_usart.c"
        "STM32F0xx_HAL_Driver/src/stm32f0xx_ll_utils.c"
)

add_library(spl STATIC ${SPL_SRC})
