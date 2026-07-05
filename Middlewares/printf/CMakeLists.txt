# Код printf: https://github.com/mpaland/printf
set(CMAKE_C_FLAGS_DEBUG ${LIBRARY_OPTIMIZATION_OPTIONS})
set(CMAKE_CXX_FLAGS_DEBUG ${LIBRARY_OPTIMIZATION_OPTIONS})

set(SOURCES printf.c)

add_library(printf-${CORE_POSTFIX} STATIC ${SOURCES})
