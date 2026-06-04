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

class MainTask {

public:
    MainTask() = default;
    ~MainTask() = default;
};

void StartMainTask();


#endif /* INC_MAINTASK_H_ */
