#ifndef __MDR_LOG_INTERNAL_H__
#define __MDR_LOG_INTERNAL_H__

//these functions do not check level versus MDT_LOCAL_LEVEL, this should be done in stm_log.h
void rtt_log_buffer_hex_internal(const char *tag, const void *buffer, uint16_t buff_len, rtt_log_level_t level);
void rtt_log_buffer_char_internal(const char *tag, const void *buffer, uint16_t buff_len, rtt_log_level_t level);
void rtt_log_buffer_hexdump_internal(const char *tag, const void *buffer, uint16_t buff_len, rtt_log_level_t log_level);

#endif // __MDR_LOG_INTERNAL_H__
