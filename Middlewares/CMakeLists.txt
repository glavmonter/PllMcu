set(CMAKE_C_FLAGS_DEBUG ${LIBRARY_OPTIMIZATION_OPTIONS})
set(CMAKE_CXX_FLAGS_DEBUG ${LIBRARY_OPTIMIZATION_OPTIONS})

add_library(printf STATIC printf/printf.c)

set(LOGGING_SRC 
    SEGGER/SEGGER_RTT.c 
    SEGGER/SEGGER_RTT_printf.c 
    logging/log_freertos.cpp 
    logging/log.cpp 
    logging/log_buffers.cpp)
add_library(rtt_logging STATIC ${LOGGING_SRC})


set(FREERTOS_CLI_SRC
    FreeRTOS-Plus-CLI/FreeRTOS_CLI.c)
add_library(freertos_cli STATIC ${FREERTOS_CLI_SRC})
