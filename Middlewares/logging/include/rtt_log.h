#ifndef __RTT_LOG_H__
#define __RTT_LOG_H__

#include <stdint.h>
#include <stdarg.h>
#include "app_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Log level
 *
 */
typedef enum {
    RTT_LOG_NONE,       /*!< No log output */
    RTT_LOG_ERROR,      /*!< Critical errors, software module can not recover on its own */
    RTT_LOG_WARN,       /*!< Error conditions from which recovery measures have been taken */
    RTT_LOG_INFO,       /*!< Information messages which describe normal flow of events */
    RTT_LOG_DEBUG,      /*!< Extra information which is not necessary for normal use (values, pointers, sizes, etc). */
    RTT_LOG_VERBOSE     /*!< Bigger chunks of debugging information, or frequent messages which can potentially flood the output. */
} rtt_log_level_t;

typedef int (*vprintf_like_t)(const char *, va_list);

/**
 * @brief Default log level
 *
 * This is used by the definition of RTT_EARLY_LOGx macros. It is not
 * recommended to set this directly, call rtt_log_level_set("*", level)
 * instead.
 */
extern rtt_log_level_t rtt_log_default_level;

/**
 * @brief Set log level for given tag
 *
 * If logging for given component has already been enabled, changes previous setting.
 *
 * @note Note that this function can not raise log level above the level set using
 * CONFIG_LOG_MAXIMUM_LEVEL setting in menuconfig.
 * To raise log level above the default one for a given file, define
 * LOG_LOCAL_LEVEL to one of the RTT_LOG_* values, before including
 * rtt_log.h in this file.
 *
 * @param tag Tag of the log entries to enable. Must be a non-NULL zero terminated string.
 *            Value "*" resets log level for all tags to the given value.
 *
 * @param level  Selects log level to enable. Only logs at this and lower verbosity
 * levels will be shown.
 */
void rtt_log_level_set(const char* tag, rtt_log_level_t level);

/**
 * @brief Get log level for a given tag, can be used to avoid expensive log statements
 *
 * @param tag Tag of the log to query current level. Must be a non-NULL zero terminated
 *            string.
 *
 * @return The current log level for the given tag
 */
rtt_log_level_t rtt_log_level_get(const char* tag);

/**
 * @brief Set function used to output log entries
 *
 * By default, log output goes to UART0. This function can be used to redirect log
 * output to some other destination, such as file or network. Returns the original
 * log handler, which may be necessary to return output to the previous destination.
 *
 * @note Please note that function callback here must be re-entrant as it can be
 * invoked in parallel from multiple thread context.
 *
 * @param func new Function used for output. Must have same signature as vprintf.
 *
 * @return func old Function used for output.
 */
vprintf_like_t rtt_log_set_vprintf(vprintf_like_t func);

/**
 * @brief Function which returns timestamp to be used in log output
 *
 * This function is used in expansion of RTT_LOGx macros.
 * In the 2nd stage bootloader, and at early application startup stage
 * this function uses CPU cycle counter as time source. Later when
 * FreeRTOS scheduler start running, it switches to FreeRTOS tick count.
 *
 * For now, we ignore millisecond counter overflow.
 *
 * @return timestamp, in milliseconds
 */
uint32_t rtt_log_timestamp(void);

/**
 * @brief Function which returns system timestamp to be used in log output
 *
 * This function is used in expansion of RTT_LOGx macros to print
 * the system time as "HH:MM:SS.sss". The system time is initialized to
 * 0 on startup, this can be set to the correct time with an SNTP sync,
 * or manually with standard POSIX time functions.
 *
 * Currently, this will not get used in logging from binary blobs
 * (i.e. Wi-Fi & Bluetooth libraries), these will still print the RTOS tick time.
 *
 * @return timestamp, in "HH:MM:SS.sss"
 */
char* rtt_log_system_timestamp(void);

/**
 * @brief Function which returns timestamp to be used in log output
 *
 * This function uses HW cycle counter and does not depend on OS,
 * so it can be safely used after application crash.
 *
 * @return timestamp, in milliseconds
 */
uint32_t rtt_log_early_timestamp(void);

/**
 * @brief Write message into the log
 *
 * This function is not intended to be used directly. Instead, use one of
 * RTT_LOGE, RTT_LOGW, RTT_LOGI, RTT_LOGD, RTT_LOGV macros.
 *
 * This function or these macros should not be used from an interrupt.
 */
void rtt_log_write(rtt_log_level_t level, const char* tag, const char* format, ...) __attribute__ ((format (printf, 3, 4)));

/**
 * @brief Write message into the log, va_list variant
 * @see rtt_log_write()
 *
 * This function is provided to ease integration toward other logging framework,
 * so that rtt_log can be used as a log sink.
 */
void rtt_log_writev(rtt_log_level_t level, const char* tag, const char* format, va_list args);

/** @cond */

#include "rtt_log_internal.h"

#ifndef LOG_LOCAL_LEVEL
#ifndef BOOTLOADER_BUILD
#define LOG_LOCAL_LEVEL  CONFIG_LOG_MAXIMUM_LEVEL
#else
#define LOG_LOCAL_LEVEL  CONFIG_BOOTLOADER_LOG_LEVEL
#endif
#endif

/** @endcond */

/**
 * @brief Log a buffer of hex bytes at specified level, separated into 16 bytes each line.
 *
 * @param  tag      description tag
 * @param  buffer   Pointer to the buffer array
 * @param  buff_len length of buffer in bytes
 * @param  level    level of the log
 *
 */
#define RTT_LOG_BUFFER_HEX_LEVEL( tag, buffer, buff_len, level ) \
    do {\
        if ( LOG_LOCAL_LEVEL >= (level) ) { \
            rtt_log_buffer_hex_internal( tag, buffer, buff_len, level ); \
        } \
    } while(0)

/**
 * @brief Log a buffer of characters at specified level, separated into 16 bytes each line. Buffer should contain only printable characters.
 *
 * @param  tag      description tag
 * @param  buffer   Pointer to the buffer array
 * @param  buff_len length of buffer in bytes
 * @param  level    level of the log
 *
 */
#define RTT_LOG_BUFFER_CHAR_LEVEL( tag, buffer, buff_len, level ) \
    do {\
        if ( LOG_LOCAL_LEVEL >= (level) ) { \
            rtt_log_buffer_char_internal( tag, buffer, buff_len, level ); \
        } \
    } while(0)

/**
 * @brief Dump a buffer to the log at specified level.
 *
 * The dump log shows just like the one below:
 *
 *      I (0) MAIN: 0x20000c3c   4b 31 39 38 36 56 45 39  32 20 69 73 20 67 72 65  |K1986VE92 is gre|
 *      I (1) MAIN: 0x20000c3c   61 74 20 6d 69 63 72 6f  63 6f 6e 74 6f 6c 6c 65  |at microcontolle|
 *      I (2) MAIN: 0x20000c3c   72 20 66 72 6f 6d 20 4d  69 6c 61 6e 64 72 2c 20  |r from Milandr, |
 *      I (2) MAIN: 0x20000c3c   5a 65 6c 65 6e 6f 67 72  61 64 00                 |Zelenograd.|
 *
 * It is highly recommended to use terminals with over 102 text width.
 *
 * @param tag description tag
 * @param buffer Pointer to the buffer array
 * @param buff_len length of buffer in bytes
 * @param level level of the log
 */
#define RTT_LOG_BUFFER_HEXDUMP( tag, buffer, buff_len, level ) \
    do { \
        if ( LOG_LOCAL_LEVEL >= (level) ) { \
            rtt_log_buffer_hexdump_internal( tag, buffer, buff_len, level); \
        } \
    } while(0)

/**
 * @brief Log a buffer of hex bytes at Info level
 *
 * @param  tag      description tag
 * @param  buffer   Pointer to the buffer array
 * @param  buff_len length of buffer in bytes
 *
 * @see ``rtt_log_buffer_hex_level``
 *
 */
#define RTT_LOG_BUFFER_HEX(tag, buffer, buff_len) \
    do { \
        if (LOG_LOCAL_LEVEL >= RTT_LOG_INFO) { \
            RTT_LOG_BUFFER_HEX_LEVEL( tag, buffer, buff_len, RTT_LOG_INFO ); \
        }\
    } while(0)

/**
 * @brief Log a buffer of characters at Info level. Buffer should contain only printable characters.
 *
 * @param  tag      description tag
 * @param  buffer   Pointer to the buffer array
 * @param  buff_len length of buffer in bytes
 *
 * @see ``rtt_log_buffer_char_level``
 *
 */
#define RTT_LOG_BUFFER_CHAR(tag, buffer, buff_len) \
    do { \
        if (LOG_LOCAL_LEVEL >= RTT_LOG_INFO) { \
            RTT_LOG_BUFFER_CHAR_LEVEL( tag, buffer, buff_len, RTT_LOG_INFO ); \
        }\
    } while(0)

/** @cond */

//to be back compatible
#define rtt_log_buffer_hex      RTT_LOG_BUFFER_HEX
#define rtt_log_buffer_char     RTT_LOG_BUFFER_CHAR


#if CONFIG_LOG_COLORS
#define LOG_COLOR_BLACK   "30"
#define LOG_COLOR_RED     "31"
#define LOG_COLOR_GREEN   "32"
#define LOG_COLOR_BROWN   "33"
#define LOG_COLOR_BLUE    "34"
#define LOG_COLOR_PURPLE  "35"
#define LOG_COLOR_CYAN    "36"
#define LOG_COLOR(COLOR)  "\033[0;" COLOR "m"
#define LOG_BOLD(COLOR)   "\033[1;" COLOR "m"
#define LOG_RESET_COLOR   "\033[0m"
#define LOG_COLOR_E       LOG_COLOR(LOG_COLOR_RED)
#define LOG_COLOR_W       LOG_COLOR(LOG_COLOR_BROWN)
#define LOG_COLOR_I       LOG_COLOR(LOG_COLOR_GREEN)
#define LOG_COLOR_D
#define LOG_COLOR_V
#else //CONFIG_LOG_COLORS
#define LOG_COLOR_E
#define LOG_COLOR_W
#define LOG_COLOR_I
#define LOG_COLOR_D
#define LOG_COLOR_V
#define LOG_RESET_COLOR
#endif //CONFIG_LOG_COLORS

#define LOG_FORMAT(letter, format)  LOG_COLOR_ ## letter #letter " (%8lu) %s: " format LOG_RESET_COLOR "\n"
#define LOG_SYSTEM_TIME_FORMAT(letter, format)  LOG_COLOR_ ## letter #letter " (%s) %s: " format LOG_RESET_COLOR "\n"

/** @endcond */

/// macro to output logs in startup code, before heap allocator and syscalls have been initialized.
/// Log at ``RTT_LOG_ERROR`` level. @see ``printf``,``RTT_LOGE``
#define portGET_ARGUMENT_COUNT_INNER(zero, one, count, ...) count

/**
 * In the future, we want to switch to C++20. We also want to become compatible with clang.
 * Hence, we provide two versions of the following macros which are using variadic arguments.
 * The first one is using the GNU extension \#\#__VA_ARGS__. The second one is using the C++20 feature __VA_OPT__(,).
 * This allows users to compile their code with standard C++20 enabled instead of the GNU extension.
 * Below C++20, we haven't found any good alternative to using \#\#__VA_ARGS__.
 */
#if defined(__cplusplus) && (__cplusplus >  201703L)
#define RTT_EARLY_LOGE( tag, format, ... ) RTT_LOG_EARLY_IMPL(tag, format, RTT_LOG_ERROR,   E __VA_OPT__(,) __VA_ARGS__)
/// macro to output logs in startup code at ``RTT_LOG_WARN`` level.  @see ``RTT_EARLY_LOGE``,``RTT_LOGE``, ``printf``
#define RTT_EARLY_LOGW( tag, format, ... ) RTT_LOG_EARLY_IMPL(tag, format, RTT_LOG_WARN,    W __VA_OPT__(,) __VA_ARGS__)
/// macro to output logs in startup code at ``RTT_LOG_INFO`` level.  @see ``RTT_EARLY_LOGE``,``RTT_LOGE``, ``printf``
#define RTT_EARLY_LOGI( tag, format, ... ) RTT_LOG_EARLY_IMPL(tag, format, RTT_LOG_INFO,    I __VA_OPT__(,) __VA_ARGS__)
/// macro to output logs in startup code at ``RTT_LOG_DEBUG`` level.  @see ``RTT_EARLY_LOGE``,``RTT_LOGE``, ``printf``
#define RTT_EARLY_LOGD( tag, format, ... ) RTT_LOG_EARLY_IMPL(tag, format, RTT_LOG_DEBUG,   D __VA_OPT__(,) __VA_ARGS__)
/// macro to output logs in startup code at ``RTT_LOG_VERBOSE`` level.  @see ``RTT_EARLY_LOGE``,``RTT_LOGE``, ``printf``
#define RTT_EARLY_LOGV( tag, format, ... ) RTT_LOG_EARLY_IMPL(tag, format, RTT_LOG_VERBOSE, V __VA_OPT__(,) __VA_ARGS__)
#else // !(defined(__cplusplus) && (__cplusplus >  201703L))
#define RTT_EARLY_LOGE( tag, format, ... ) RTT_LOG_EARLY_IMPL(tag, format, RTT_LOG_ERROR,   E, ##__VA_ARGS__)
/// macro to output logs in startup code at ``RTT_LOG_WARN`` level.  @see ``RTT_EARLY_LOGE``,``RTT_LOGE``, ``printf``
#define RTT_EARLY_LOGW( tag, format, ... ) RTT_LOG_EARLY_IMPL(tag, format, RTT_LOG_WARN,    W, ##__VA_ARGS__)
/// macro to output logs in startup code at ``RTT_LOG_INFO`` level.  @see ``RTT_EARLY_LOGE``,``RTT_LOGE``, ``printf``
#define RTT_EARLY_LOGI( tag, format, ... ) RTT_LOG_EARLY_IMPL(tag, format, RTT_LOG_INFO,    I, ##__VA_ARGS__)
/// macro to output logs in startup code at ``RTT_LOG_DEBUG`` level.  @see ``RTT_EARLY_LOGE``,``RTT_LOGE``, ``printf``
#define RTT_EARLY_LOGD( tag, format, ... ) RTT_LOG_EARLY_IMPL(tag, format, RTT_LOG_DEBUG,   D, ##__VA_ARGS__)
/// macro to output logs in startup code at ``RTT_LOG_VERBOSE`` level.  @see ``RTT_EARLY_LOGE``,``RTT_LOGE``, ``printf``
#define RTT_EARLY_LOGV( tag, format, ... ) RTT_LOG_EARLY_IMPL(tag, format, RTT_LOG_VERBOSE, V, ##__VA_ARGS__)
#endif // !(defined(__cplusplus) && (__cplusplus >  201703L))

#ifdef BOOTLOADER_BUILD
#define _RTT_LOG_EARLY_ENABLED(log_level) (LOG_LOCAL_LEVEL >= (log_level))
#else
/* For early log, there is no log tag filtering. So we want to log only if both the LOG_LOCAL_LEVEL and the
   currently configured min log level are higher than the log level */
#define _RTT_LOG_EARLY_ENABLED(log_level) (LOG_LOCAL_LEVEL >= (log_level) && rtt_log_default_level >= (log_level))
#endif

#define RTT_LOG_EARLY_IMPL(tag, format, log_level, log_tag_letter, ...) do {                             \
        if (_RTT_LOG_EARLY_ENABLED(log_level)) {                                                         \
            rtt_rom_printf(LOG_FORMAT(log_tag_letter, format), rtt_log_timestamp(), tag, ##__VA_ARGS__); \
        }} while(0)

#ifndef BOOTLOADER_BUILD
#if defined(__cplusplus) && (__cplusplus >  201703L)
#define RTT_LOGE( tag, format, ... ) RTT_LOG_LEVEL_LOCAL(RTT_LOG_ERROR,   tag, format __VA_OPT__(,) __VA_ARGS__)
#define RTT_LOGW( tag, format, ... ) RTT_LOG_LEVEL_LOCAL(RTT_LOG_WARN,    tag, format __VA_OPT__(,) __VA_ARGS__)
#define RTT_LOGI( tag, format, ... ) RTT_LOG_LEVEL_LOCAL(RTT_LOG_INFO,    tag, format __VA_OPT__(,) __VA_ARGS__)
#define RTT_LOGD( tag, format, ... ) RTT_LOG_LEVEL_LOCAL(RTT_LOG_DEBUG,   tag, format __VA_OPT__(,) __VA_ARGS__)
#define RTT_LOGV( tag, format, ... ) RTT_LOG_LEVEL_LOCAL(RTT_LOG_VERBOSE, tag, format __VA_OPT__(,) __VA_ARGS__)
#else // !(defined(__cplusplus) && (__cplusplus >  201703L))
#define RTT_LOGE( tag, format, ... ) RTT_LOG_LEVEL_LOCAL(RTT_LOG_ERROR,   tag, format, ##__VA_ARGS__)
#define RTT_LOGW( tag, format, ... ) RTT_LOG_LEVEL_LOCAL(RTT_LOG_WARN,    tag, format, ##__VA_ARGS__)
#define RTT_LOGI( tag, format, ... ) RTT_LOG_LEVEL_LOCAL(RTT_LOG_INFO,    tag, format, ##__VA_ARGS__)
#define RTT_LOGD( tag, format, ... ) RTT_LOG_LEVEL_LOCAL(RTT_LOG_DEBUG,   tag, format, ##__VA_ARGS__)
#define RTT_LOGV( tag, format, ... ) RTT_LOG_LEVEL_LOCAL(RTT_LOG_VERBOSE, tag, format, ##__VA_ARGS__)
#endif // !(defined(__cplusplus) && (__cplusplus >  201703L))
#else

/**
 * Macro to output logs at RTT_LOG_ERROR level.
 *
 * @note This macro cannot be used when interrupts are disabled or inside an ISR.
 *
 * @param tag tag of the log, which can be used to change the log level by ``rtt_log_level_set`` at runtime.
 *
 * @see ``printf``
 */
#if defined(__cplusplus) && (__cplusplus >  201703L)
#define RTT_LOGE( tag, format, ... )  RTT_EARLY_LOGE(tag, format __VA_OPT__(,) __VA_ARGS__)
/// macro to output logs at ``RTT_LOG_WARN`` level.  @see ``RTT_LOGE``
#define RTT_LOGW( tag, format, ... )  RTT_EARLY_LOGW(tag, format __VA_OPT__(,) __VA_ARGS__)
/// macro to output logs at ``RTT_LOG_INFO`` level.  @see ``RTT_LOGE``
#define RTT_LOGI( tag, format, ... )  RTT_EARLY_LOGI(tag, format __VA_OPT__(,) __VA_ARGS__)
/// macro to output logs at ``RTT_LOG_DEBUG`` level.  @see ``RTT_LOGE``
#define RTT_LOGD( tag, format, ... )  RTT_EARLY_LOGD(tag, format __VA_OPT__(,) __VA_ARGS__)
/// macro to output logs at ``RTT_LOG_VERBOSE`` level.  @see ``RTT_LOGE``
#define RTT_LOGV( tag, format, ... )  RTT_EARLY_LOGV(tag, format __VA_OPT__(,) __VA_ARGS__)
#else // !(defined(__cplusplus) && (__cplusplus >  201703L))
#define RTT_LOGE( tag, format, ... )  RTT_EARLY_LOGE(tag, format, ##__VA_ARGS__)
/// macro to output logs at ``RTT_LOG_WARN`` level.  @see ``RTT_LOGE``
#define RTT_LOGW( tag, format, ... )  RTT_EARLY_LOGW(tag, format, ##__VA_ARGS__)
/// macro to output logs at ``RTT_LOG_INFO`` level.  @see ``RTT_LOGE``
#define RTT_LOGI( tag, format, ... )  RTT_EARLY_LOGI(tag, format, ##__VA_ARGS__)
/// macro to output logs at ``RTT_LOG_DEBUG`` level.  @see ``RTT_LOGE``
#define RTT_LOGD( tag, format, ... )  RTT_EARLY_LOGD(tag, format, ##__VA_ARGS__)
/// macro to output logs at ``RTT_LOG_VERBOSE`` level.  @see ``RTT_LOGE``
#define RTT_LOGV( tag, format, ... )  RTT_EARLY_LOGV(tag, format, ##__VA_ARGS__)
#endif // !(defined(__cplusplus) && (__cplusplus >  201703L))
#endif  // BOOTLOADER_BUILD

/** runtime macro to output logs at a specified level.
 *
 * @param tag tag of the log, which can be used to change the log level by ``rtt_log_level_set`` at runtime.
 * @param level level of the output log.
 * @param format format of the output log. See ``printf``
 * @param ... variables to be replaced into the log. See ``printf``
 *
 * @see ``printf``
 */
#if defined(__cplusplus) && (__cplusplus >  201703L)
#if CONFIG_LOG_TIMESTAMP_SOURCE_RTOS
#define RTT_LOG_LEVEL(level, tag, format, ...) do {                     \
        if (level==RTT_LOG_ERROR )          { rtt_log_write(RTT_LOG_ERROR,      tag, LOG_FORMAT(E, format), rtt_log_timestamp(), tag __VA_OPT__(,) __VA_ARGS__); } \
        else if (level==RTT_LOG_WARN )      { rtt_log_write(RTT_LOG_WARN,       tag, LOG_FORMAT(W, format), rtt_log_timestamp(), tag __VA_OPT__(,) __VA_ARGS__); } \
        else if (level==RTT_LOG_DEBUG )     { rtt_log_write(RTT_LOG_DEBUG,      tag, LOG_FORMAT(D, format), rtt_log_timestamp(), tag __VA_OPT__(,) __VA_ARGS__); } \
        else if (level==RTT_LOG_VERBOSE )   { rtt_log_write(RTT_LOG_VERBOSE,    tag, LOG_FORMAT(V, format), rtt_log_timestamp(), tag __VA_OPT__(,) __VA_ARGS__); } \
        else                                { rtt_log_write(RTT_LOG_INFO,       tag, LOG_FORMAT(I, format), rtt_log_timestamp(), tag __VA_OPT__(,) __VA_ARGS__); } \
    } while(0)
#elif CONFIG_LOG_TIMESTAMP_SOURCE_SYSTEM
#define RTT_LOG_LEVEL(level, tag, format, ...) do {                     \
        if (level==RTT_LOG_ERROR )          { rtt_log_write(RTT_LOG_ERROR,      tag, LOG_SYSTEM_TIME_FORMAT(E, format), rtt_log_system_timestamp(), tag __VA_OPT__(,) __VA_ARGS__); } \
        else if (level==RTT_LOG_WARN )      { rtt_log_write(RTT_LOG_WARN,       tag, LOG_SYSTEM_TIME_FORMAT(W, format), rtt_log_system_timestamp(), tag __VA_OPT__(,) __VA_ARGS__); } \
        else if (level==RTT_LOG_DEBUG )     { rtt_log_write(RTT_LOG_DEBUG,      tag, LOG_SYSTEM_TIME_FORMAT(D, format), rtt_log_system_timestamp(), tag __VA_OPT__(,) __VA_ARGS__); } \
        else if (level==RTT_LOG_VERBOSE )   { rtt_log_write(RTT_LOG_VERBOSE,    tag, LOG_SYSTEM_TIME_FORMAT(V, format), rtt_log_system_timestamp(), tag __VA_OPT__(,) __VA_ARGS__); } \
        else                                { rtt_log_write(RTT_LOG_INFO,       tag, LOG_SYSTEM_TIME_FORMAT(I, format), rtt_log_system_timestamp(), tag __VA_OPT__(,) __VA_ARGS__); } \
    } while(0)
#endif //CONFIG_LOG_TIMESTAMP_SOURCE_xxx
#else // !(defined(__cplusplus) && (__cplusplus >  201703L))
#if CONFIG_LOG_TIMESTAMP_SOURCE_RTOS
#define RTT_LOG_LEVEL(level, tag, format, ...) do {                     \
        if (level==RTT_LOG_ERROR )          { rtt_log_write(RTT_LOG_ERROR,      tag, LOG_FORMAT(E, format), rtt_log_timestamp(), tag, ##__VA_ARGS__); } \
        else if (level==RTT_LOG_WARN )      { rtt_log_write(RTT_LOG_WARN,       tag, LOG_FORMAT(W, format), rtt_log_timestamp(), tag, ##__VA_ARGS__); } \
        else if (level==RTT_LOG_DEBUG )     { rtt_log_write(RTT_LOG_DEBUG,      tag, LOG_FORMAT(D, format), rtt_log_timestamp(), tag, ##__VA_ARGS__); } \
        else if (level==RTT_LOG_VERBOSE )   { rtt_log_write(RTT_LOG_VERBOSE,    tag, LOG_FORMAT(V, format), rtt_log_timestamp(), tag, ##__VA_ARGS__); } \
        else                                { rtt_log_write(RTT_LOG_INFO,       tag, LOG_FORMAT(I, format), rtt_log_timestamp(), tag, ##__VA_ARGS__); } \
    } while(0)
#elif CONFIG_LOG_TIMESTAMP_SOURCE_SYSTEM
#error "Not implemented"
#define RTT_LOG_LEVEL(level, tag, format, ...) do {                     \
        if (level==RTT_LOG_ERROR )          { rtt_log_write(RTT_LOG_ERROR,      tag, LOG_SYSTEM_TIME_FORMAT(E, format), rtt_log_system_timestamp(), tag, ##__VA_ARGS__); } \
        else if (level==RTT_LOG_WARN )      { rtt_log_write(RTT_LOG_WARN,       tag, LOG_SYSTEM_TIME_FORMAT(W, format), rtt_log_system_timestamp(), tag, ##__VA_ARGS__); } \
        else if (level==RTT_LOG_DEBUG )     { rtt_log_write(RTT_LOG_DEBUG,      tag, LOG_SYSTEM_TIME_FORMAT(D, format), rtt_log_system_timestamp(), tag, ##__VA_ARGS__); } \
        else if (level==RTT_LOG_VERBOSE )   { rtt_log_write(RTT_LOG_VERBOSE,    tag, LOG_SYSTEM_TIME_FORMAT(V, format), rtt_log_system_timestamp(), tag, ##__VA_ARGS__); } \
        else                                { rtt_log_write(RTT_LOG_INFO,       tag, LOG_SYSTEM_TIME_FORMAT(I, format), rtt_log_system_timestamp(), tag, ##__VA_ARGS__); } \
    } while(0)
#endif //CONFIG_LOG_TIMESTAMP_SOURCE_xxx
#endif // !(defined(__cplusplus) && (__cplusplus >  201703L))

/** runtime macro to output logs at a specified level. Also check the level with ``LOG_LOCAL_LEVEL``.
 *
 * @see ``printf``, ``RTT_LOG_LEVEL``
 */
#define RTT_LOG_LEVEL_LOCAL(level, tag, format, ...) do {               \
        if ( LOG_LOCAL_LEVEL >= level ) RTT_LOG_LEVEL(level, tag, format, ##__VA_ARGS__); \
    } while(0)


#ifdef __cplusplus
}
#endif


#endif /* __RTT_LOG_H__ */
