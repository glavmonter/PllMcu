SET(CMAKE_SYSTEM_NAME Generic)
SET(CMAKE_SYSTEM_VERSION 1)
cmake_minimum_required(VERSION 3.17)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/toolchain-arm-none-eabi.cmake")
list(APPEND CMAKE_MODULE_PATH cmake)
include(cmake/functions.cmake)
include(cmake/GetGitVersion.cmake)
# include(cmake/macro-add-python.cmake)


SET(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/STM32F051XX_FLASH.ld")

set(FPU_FLAGS "")

SET(COMMON_FLAGS
        "-mcpu=cortex-m0plus ${FPU_FLAGS} -mthumb -ffunction-sections -fdata-sections \
         -g -fno-common -Wpointer-arith -fmessage-length=0 -specs=nano.specs")

SET(CMAKE_C_FLAGS_INIT "${COMMON_FLAGS} -std=gnu11")
SET(CMAKE_CXX_FLAGS_INIT "${COMMON_FLAGS} -std=gnu++14 -fno-rtti -fno-exceptions -fno-threadsafe-statics")
SET(CMAKE_EXE_LINKER_FLAGS_INIT "-Wl,-gc-sections,--print-memory-usage")

project(PllMcu C CXX ASM)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(POSTFIX "d")

    # 0-NONE, 1-ERROR, 2-WARN, 3-INFO, 4-DEBUG, 5-VERBOSE
    add_definitions("-DCONFIG_LOG_MAXIMUM_LEVEL=5")
    add_definitions("-DDEBUG")
    add_definitions("-DUSE_ASSERT_INFO=2")
    add_definitions("-DUSE_FULL_ASSERT")
    set(LIBRARY_OPTIMIZATION_OPTIONS "-Os -g")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(POSTFIX "")
    # 0-NONE, 1-ERROR, 2-WARN, 3-INFO, 4-DEBUG, 5-VERBOSE
    add_definitions("-DCONFIG_LOG_MAXIMUM_LEVEL=0")
    add_definitions("-DUSE_FULL_ASSERT")
else()
    message(WARNING "Unknown build type")
    message(FATAL_ERROR "Set CMAKE_BUILD_TYPE to Debug or Release and reconfigure")
endif()

# Заглушки newlib
set(NEWLIB_STUBS_SRC
        "Drivers/portable/GCC_CM0/newlib/_cxx.cpp"
        "Drivers/portable/GCC_CM0/newlib/_exit.c"
        "Drivers/portable/GCC_CM0/newlib/_syscalls.c"
        "Drivers/portable/GCC_CM0/newlib/_sbrk.c"
        )

# Векторы прерываний и начального старта
set(VECTORS_EXCEPTION_HANDLERS_SRC
        "Core/src/system_stm32f0xx.c"
        "Drivers/portable/GCC_CM0/exception_handlers.c"
        "Drivers/portable/GCC_CM0/initialize_hardware.c"
        "Drivers/portable/GCC_CM0/reset_hardware.c"
        "Drivers/portable/GCC_CM0/startup.c"
        "Drivers/portable/GCC_CM0/vectors.c"
        )

set(COMMON_SRC ${VECTORS_EXCEPTION_HANDLERS_SRC} ${NEWLIB_STUBS_SRC})

set(HAL_LL_INC "Drivers/STM32F0xx_HAL_Driver/inc")
set(CMSIS_INC "Drivers/CMSIS/Include" "Drivers/CMSIS/Device/ST/STM32F0xx/Include")
set(FREERTOS "Middlewares/FreeRTOS")
set(MIDDLEWARES_INC "Middlewares")
set(RINGBUFFER_INC "Middlewares/RingBuffer")
set(PRINTF_INC "Middlewares/printf")
set(SEGGER_INC "Middlewares/SEGGER")

set(APP_INC "Core/inc")

include_directories(${CMSIS_INC})
include_directories(${HAL_LL_INC})
include_directories(${FREERTOS}/include)
include_directories(${FREERTOS}/portable/GCC/ARM_CM0)
include_directories(${APP_INC})
include_directories(${SEGGER_INC})
include_directories(${PRINTF_INC})
include_directories(${MIDDLEWARES_INC})
include_directories(${RINGBUFFER_INC})
include_directories(${MIDDLEWARES_INC})
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

set(FREERTOS_HEAP_SIZE_KB  1)

set(COMMON_DEFINITIONS -DSTM32F051x8 -DUSE_HAL_DRIVER -DUSE_FULL_LL_DRIVER)
add_definitions(${COMMON_DEFINITIONS})
add_definitions("-DPRINTF_INCLUDE_CONFIG_H")

add_subdirectory(Middlewares/FreeRTOS)
add_subdirectory(Middlewares)
add_subdirectory(Drivers)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DUSE_RTT -DUSE_FULL_ASSERT")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DUSE_RTT -DUSE_FULL_ASSERT")
set(CMAKE_ASM_FLAGS "-x assembler-with-cpp")

set(PROJECT_SRC
        "Core/src/common.cpp"
        "Core/src/main.cpp"
        "Core/src/MainTask.cpp"
        "Core/src/stm32f0xx_it.c"
        "Core/src/stm32f0xx_hal_msp.c"
    )


get_git_version(VERSION_MAJOR VERSION_MINOR VERSION_PATCH VERSION_SHORT VERSION_FULL)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Core/inc/version_app.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/include/version_app.h @ONLY)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Core/inc/FreeRTOSConfig.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/include/FreeRTOSConfig.h @ONLY)


set(PN "PllMcu${POSTFIX}")
add_executable(${PN}.elf ${PROJECT_SRC} ${COMMON_SRC})
set_options(${PN} ${LINKER_SCRIPT})
target_link_libraries(${PN}.elf PRIVATE spl rtt printf freertos)
