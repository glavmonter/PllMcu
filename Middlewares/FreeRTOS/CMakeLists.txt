
set(CMAKE_C_FLAGS_DEBUG ${LIBRARY_OPTIMIZATION_OPTIONS})
set(CMAKE_CXX_FLAGS_DEBUG ${LIBRARY_OPTIMIZATION_OPTIONS})

set(FREERTOS_SRCS
        "timers.c"
        "queue.c"
        "list.c"
        "tasks.c"
        "portable/GCC/ARM_CM0/port.c"
        "portable/MemMang/heap_1.c"
        )

add_library(freertos STATIC ${FREERTOS_SRCS})
