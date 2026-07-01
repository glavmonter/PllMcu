#ifndef INC_MAINTASK_H_
#define INC_MAINTASK_H_


#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include "SEGGER_RTT.h"
#include "etl/callback_timer.h"
#include "etl/function.h"
#include "etl/bitset.h"
#include "hardware.h"

void StartMainTask();
void UART_Printf(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
void UART_Transmit(const char *data, size_t length);

#endif /* INC_MAINTASK_H_ */
