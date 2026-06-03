#ifndef INC_MAINTASK_H_
#define INC_MAINTASK_H_


#include <FreeRTOS.h>
#include <task.h>
#include "SEGGER_RTT.h"
#include "etl/callback_timer.h"
#include "etl/function.h"
#include "etl/bitset.h"
#include "LoopController.h"
#include "Leds.h"
#include "hardware.h"


/*
 * Конечный автомат простой:
 * 3 аппаратных таймера перебираются по кругу. Прерывание каждые 20 мс.
 * 3 светодиодных линейки перебиратся по кругу, но с большей частотой
 */

typedef enum {
    LOOP_STOP,
    LOOP_A_EN,
    LOOP_B_EN,
    LOOP_C_EN
} StLoop;

class MainTask {

    typedef enum {
        LED_IDLE,
        LED_BAR0,
        LED_BAR1,
        LED_BAR2
    } StLed;

public:
    MainTask() = default;
    ~MainTask() = default;

    void BarTimeoutCallback();
    void CheckLedState();

    QueueHandle_t xQueueLedState = nullptr;
    QueueHandle_t xQueueMultiplier = nullptr;

    void CheckMultiplierMessage();

    void StartReceived();
    void InitBars();
    void ProcessLoopInterrupt();
    void ProcessBarNext();
    uint32_t PrepareMeterTimer(lc::LoopName loop);

    StLoop GetState() { return state_loop; }

    static void InitLoopTimer();
    static void ResetTimers();

private:
    StLoop state_loop = LOOP_STOP;
    StLed  state_led  = LED_IDLE;


    bool StartLoopA();
    bool StartLoopB();
    bool StartLoopC();


private:
    etl::timer::id::type timer_bars;
    etl::timer::id::type reset_timer;


    static void inline __attribute__((always_inline)) DisableBars() { BAR_PORT->BSRR = BAR0_o_PIN | BAR1_o_PIN | BAR2_o_PIN; }
    void ShowLedBar(BarNumber bar);
    void ScanSwitches();
    bool ReadSwitch(uint8_t &sw0, uint8_t &sw1);
    static uint8_t NormalizeSwitch(uint8_t sw);

    uint8_t switch0 = 0;
    uint8_t switch1 = 0;
    uint16_t switch_all = 0;

    Leds leds[3];

private:
    lc::LoopController &lc = getLC();
};

void StartMainTask();



#endif /* INC_MAINTASK_H_ */
