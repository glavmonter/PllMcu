SET(CMAKE_SYSTEM_NAME Generic)
SET(CMAKE_SYSTEM_VERSION 1)
cmake_minimum_required(VERSION 3.10)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/toolchain-arm-none-eabi.cmake")
list(APPEND CMAKE_MODULE_PATH cmake)
include(cmake/functions.cmake)

SET(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/STM32L052C6_FLASH.ld")

set(FPU_FLAGS "")

SET(COMMON_FLAGS
        "-mcpu=cortex-m0plus ${FPU_FLAGS} -mthumb -ffunction-sections -fdata-sections \
         -g -fno-common -fmessage-length=0 -specs=nano.specs")

SET(CMAKE_C_FLAGS_INIT "${COMMON_FLAGS} -std=gnu11")
SET(CMAKE_CXX_FLAGS_INIT "${COMMON_FLAGS} -std=gnu++14 -fno-rtti -fno-exceptions -fno-threadsafe-statics")
SET(CMAKE_EXE_LINKER_FLAGS_INIT "-Wl,-gc-sections,--print-memory-usage")

project(PllMcu C CXX ASM)


if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(POSTFIX "d")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(POSTFIX "")
else()
    message(WARNING "Unknown build type")
    message(FATAL_ERROR "Set CMAKE_BUILD_TYPE to Debug or Release and reconfigure")
endif()


# Исходные коды библиотек
set(HAL_LL_SRC
        "Drivers/STM32L0xx_HAL_Driver/src/stm32l0xx_hal.c"
        "Drivers/STM32L0xx_HAL_Driver/src/stm32l0xx_hal_flash.c"
        "Drivers/STM32L0xx_HAL_Driver/src/stm32l0xx_hal_flash_ex.c"
        "Drivers/STM32L0xx_HAL_Driver/src/stm32l0xx_ll_gpio.c"
        "Drivers/STM32L0xx_HAL_Driver/src/stm32l0xx_ll_spi.c"
        "Drivers/STM32L0xx_HAL_Driver/src/stm32l0xx_ll_utils.c"
        "Drivers/STM32L0xx_HAL_Driver/src/stm32l0xx_ll_adc.c"
    )

set(SEGGER_SRC
        "Core/src/SEGGER_RTT.c"
        "Core/src/SEGGER_RTT_printf.c"
    )

set(FREERTOS_SRC
        "Middlewares/Third_Party/FreeRTOS/list.c"
        "Middlewares/Third_Party/FreeRTOS/queue.c"
        "Middlewares/Third_Party/FreeRTOS/tasks.c"
        "Middlewares/Third_Party/FreeRTOS/timers.c"
        "Middlewares/Third_Party/FreeRTOS/portable/GCC/ARM_CM0/port.c"
    )

set(COMMON_SRC ${HAL_LL_SRC} ${SEGGER_SRC} ${FREERTOS_SRC})

set(HAL_LL_INC "Drivers/STM32L0xx_HAL_Driver/inc")
set(CMSIS_INC "Drivers/CMSIS/Include" "Drivers/CMSIS/Device/ST/STM32L0xx/Include")
set(FREERTORS_INC "Middlewares/Third_Party/FreeRTOS/include" "Middlewares/Third_Party/FreeRTOS/portable/GCC/ARM_CM0")
set(APP_INC "Core/inc")

include_directories(${CMSIS_INC})
include_directories(${HAL_LL_INC})
include_directories(${FREERTORS_INC})
include_directories(${APP_INC})

set(COMMON_DEFINITIONS "-DSTM32L052xx -DUSE_HAL_DRIVER -DUSE_FULL_LL_DRIVER -DETL_NO_STL")

add_definitions(${COMMON_DEFINITIONS})

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DUSE_RTT -DUSE_FULL_ASSERT")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DUSE_RTT -DUSE_FULL_ASSERT")
set(CMAKE_ASM_FLAGS "-x assembler-with-cpp")

set(PROJECT_SRC
        "Core/src/common.cpp"
        "Core/src/main.cpp"
        "Core/src/MainTask.cpp"
        "Core/src/stm32l0xx_it.c"
        "Core/src/syscalls.c"
        "Core/src/system_stm32l0xx.c"
        "startup/startup_stm32l052xx.s"
    )

set(PN "PllMcu${POSTFIX}")
add_executable(${PN}.elf ${PROJECT_SRC} ${COMMON_SRC})
set_options(${PN} ${LINKER_SCRIPT})
