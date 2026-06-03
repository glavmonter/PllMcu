#include <cstdio>
#include "stm32l0xx_hal.h"
#include "SEGGER_RTT.h"
#include "LoopController.h"
#include "stm32l0xx_ll_gpio.h"
#include "stm32l0xx_ll_usart.h"
#include "hardware.h"
#include "common.h"
#include "etl/function.h"
#include "etl/absolute.h"
#include "etl/utility.h"
#include "adc.h"
#include "utils.h"

#define SELFTEST    1
#define MODNAME "[Loop] "

namespace lc {

RB TransmitBuffer;
char PrintfBuffer[RING_BUFFER_SIZE];


LoopController ctrl;
Loop loopa(ctrl);
Loop loopb(ctrl);
Loop loopc(ctrl);
sml::sm<Loop> sm_loop_a{loopa};
sml::sm<Loop> sm_loop_b{loopb};
sml::sm<Loop> sm_loop_c{loopc};


static etl::callback_timer<4> timer_controller;
static etl_ext::function_mpval<LoopController, BarNumber, &LoopController::RedLedCallback> callback_a(ctrl, BAR_A);
static etl_ext::function_mpval<LoopController, BarNumber, &LoopController::RedLedCallback> callback_b(ctrl, BAR_B);
static etl_ext::function_mpval<LoopController, BarNumber, &LoopController::RedLedCallback> callback_c(ctrl, BAR_C);
static etl::function_imv<LoopController, ctrl, &LoopController::TemperatureCallback> temperature_callback;


void LoopController::task() {

    led_timers[BAR_A] = timer_controller.register_timer(callback_a, 100, etl::timer::mode::REPEATING);
    led_timers[BAR_B] = timer_controller.register_timer(callback_b, 100, etl::timer::mode::REPEATING);
    led_timers[BAR_C] = timer_controller.register_timer(callback_c, 100, etl::timer::mode::REPEATING);
    temperature_timer = timer_controller.register_timer(temperature_callback, 1000*60, etl::timer::mode::REPEATING);
    timer_controller.enable(true);


    Configure_ADC();
    Activate_ADC();


    loopa.InitObjects(LOOP_A, BAR_A);
    loopb.InitObjects(LOOP_B, BAR_B);
    loopc.InitObjects(LOOP_C, BAR_C);
    StartReceived();

    uint16_t uSettings = 0;
    while (xQueueReceive(xQueueSettings, &uSettings, portMAX_DELAY) != pdTRUE) {}

    UartPrint(TransmitBuffer, "\n\nStarting...\n");
    SetFreqencyRange(uSettings);

    loopa.SetSensitivity((uSettings & SETTINGS_SENS_A_Msk) >> SETTINGS_SENS_A_Pos);
    loopb.SetSensitivity((uSettings & SETTINGS_SENS_B_Msk) >> SETTINGS_SENS_B_Pos);
    loopc.SetSensitivity((uSettings & SETTINGS_SENS_C_Msk) >> SETTINGS_SENS_C_Pos);

    loopa.SetSelection((uSettings & SETTINGS_FREQ_A_Msk) >> SETTINGS_FREQ_A_Pos);
    loopb.SetSelection((uSettings & SETTINGS_FREQ_B_Msk) >> SETTINGS_FREQ_B_Pos);
    loopc.SetSelection((uSettings & SETTINGS_FREQ_C_Msk) >> SETTINGS_FREQ_C_Pos);

    sm_loop_a.process_event(StartEv{});
    sm_loop_b.process_event(StartEv{});
    sm_loop_c.process_event(StartEv{});

    Measure measure = {};
    TickType_t last_tick = xTaskGetTickCount();
    timer_controller.start(temperature_timer, true);

    for (;;) {
        if (xQueueReceive(xQueueLoopMeasure, &measure, 5) == pdTRUE) {
            switch (measure.loop) {
            case LOOP_A:
                sm_loop_a.process_event(MeasureEv{LOOP_A, measure.value, measure.multiplier});
                break;
            case LOOP_B:
                sm_loop_b.process_event(MeasureEv{LOOP_B, measure.value, measure.multiplier});
                break;
            case LOOP_C:
                sm_loop_c.process_event(MeasureEv{LOOP_C, measure.value, measure.multiplier});
                break;
            default:
                assert_param(0);
            }
        }

        TickType_t current_tick = xTaskGetTickCount();
        timer_controller.tick(current_tick - last_tick);
        last_tick = current_tick;
    }
}


void LoopController::SetFreqencyRange(uint16_t settings) {
    // SEL2
    if (settings & SETTINGS_FREQ_A_Msk) {
        SEL2_A_o_PORT->BSRR = SEL2_A_o_PIN;
    } else {
        SEL2_A_o_PORT->BRR = SEL2_A_o_PIN;
    }

    if (settings & SETTINGS_FREQ_B_Msk) {
        SEL1_B_o_PORT->BSRR = SEL1_B_o_PIN;
    } else {
        SEL1_B_o_PORT->BRR = SEL1_B_o_PIN;
    }

    if (settings & SETTINGS_FREQ_C_Msk) {
        SEL0_C_o_PORT->BSRR = SEL0_C_o_PIN;
    } else {
        SEL0_C_o_PORT->BRR = SEL0_C_o_PIN;
    }
}

#define TEST_TIMEOUT    200
void LoopController::StartReceived() {
    SetGreen(BAR_A, 0);
    SetGreen(BAR_B, 0);
    SetGreen(BAR_C, 0);
    SetRedState(BAR_A, true);
    SetRedState(BAR_B, true);
    SetRedState(BAR_C, true);
}



void LoopController::InitObjects() {
static StaticQueue_t xQueueSettingsStatic, xQueueLoopMeasureStatic;
static uint8_t ucQueueSettingsStorage[sizeof(uint16_t) * 1], ucQueueLoopMeasureStorage[sizeof(MeasureEv) * 1];

constexpr size_t LoopTaskStackSize = configMINIMAL_STACK_SIZE * 3;  // TODO При 3 возникает переполнение стека!!!!
static StackType_t xLoopTaskStack[LoopTaskStackSize];
static StaticTask_t xTCBLoopTask;

    xQueueSettings    = xQueueCreateStatic(1, sizeof(uint16_t), ucQueueSettingsStorage, &xQueueSettingsStatic);
    xQueueLoopMeasure = xQueueCreateStatic(1, sizeof(MeasureEv), ucQueueLoopMeasureStorage, &xQueueLoopMeasureStatic);
    xTaskCreateStatic(task_lc, "Loop", LoopTaskStackSize, this, tskIDLE_PRIORITY, xLoopTaskStack, &xTCBLoopTask);
}


void LoopController::UartPrint(RB &rb, const char *str) {
    size_t len = strnlen(str, RING_BUFFER_SIZE);
    for (size_t i = 0; i < len; i++) {
        rb.Write(str[i]);
    }

    if (rb.isTransmitting)
        return;

    char val = 0;
    if (rb.Read(val)) {
        rb.isTransmitting = 1;
        LL_USART_TransmitData8(MODBUS_UART, val);
        LL_USART_EnableIT_TXE(MODBUS_UART);
    }
}


/*=================================================================================================================*/
void LoopController::TemperatureCallback() {
__IO uint16_t uhADCxConvertedData = VAR_CONVERTED_DATA_INIT_VALUE; /* ADC group regular conversion data */
__IO int16_t hADCxConvertedData_Temperature_DegreeCelsius = 0;  /* Value of temperature calculated from ADC conversion data (unit: degree Celcius) */

    ConversionStartPoll_ADC_GrpRegular();

    uhADCxConvertedData = LL_ADC_REG_ReadConversionData12(ADC1);
    hADCxConvertedData_Temperature_DegreeCelsius = __LL_ADC_CALC_TEMPERATURE(VDDA_APPLI, uhADCxConvertedData, LL_ADC_RESOLUTION_12B);
    sprintf(PrintfBuffer, "Temperature: %d C\n", hADCxConvertedData_Temperature_DegreeCelsius);
    UartPrint(TransmitBuffer, PrintfBuffer);
}


void LoopController::RedLedCallback(BarNumber bar) {
    red_state[bar] = !red_state[bar];
    SendLedUpdate();
}

void LoopController::SetRedBlink(BarNumber bar, uint32_t period) {
    auto timer_id = led_timers[bar];
    timer_controller.set_period(timer_id, period);
    timer_controller.start(timer_id, true);
}

void LoopController::SetRedState(BarNumber bar, bool state) {
    timer_controller.stop(led_timers[bar]);
    red_state[bar] = state;
    SendLedUpdate();
}

void LoopController::SetGreen(BarNumber bar, uint8_t state) {
    green_state[bar] = state & 0x7F;
    SendLedUpdate();
}

void LoopController::SetGreenLevel(BarNumber bar, uint8_t level) {
    green_state[bar] = utils::GetGreenLevel(level);
    SendLedUpdate();
}

void LoopController::SendLedUpdate() {
    if (!xLedState)
        return;

    uint32_t led_all = 0;
    uint8_t led_bar = (red_state[BAR_A] << 7) + green_state[BAR_A];
    led_all = led_bar;

    led_bar = (red_state[BAR_B] << 7) + green_state[BAR_B];
    led_all |= led_bar << 8;

    led_bar = (red_state[BAR_C] << 7) + green_state[BAR_C];
    led_all |= led_bar << 16;

    xQueueSend(xLedState, &led_all, 10);
}


/*=================================================================================================================*/


void Loop::StartReceived() const {
    SEGGER_RTT_printf(0, MODNAME"%c) Start Received\n", _name);
}


bool Loop::MeasureEvent(const MeasureEv &e) {

char b[FLOAT_BUFFER_SIZE] = {};
char *s;

    TickType_t current_tick = xTaskGetTickCount();

int needle{};
auto raw = needleFilter.append(e.value, needle);
    if (needle >= 0) {
        SEGGER_RTT_printf(0, "%c)[%u] Needle:v%u:n%d->r%u [%u %u %u]\n", _name, current_tick,
                e.value, needle, raw, needleFilter.memory_dump[0], needleFilter.memory_dump[1], needleFilter.memory_dump[2]);
    }

    iMeasureCounter++;
    if ((m_bStarted == false) && (current_tick > 15000))
        m_bStarted = true;

    sample_filter::IIR::output_type val_lpf = filter_sample.sample(raw);
    if (etl::is_floating_point<sample_filter::IIR::output_type>::value) {
        s = _float_to_char(val_lpf, b);
    } else {
        sprintf(b, "%d", (int)val_lpf);
        s = b;
    }

    if (m_bStarted) {
        if (!out_state) {
            if (val_lpf < Calibrations.ThresholdLow) {
                // Set relay ON
                out_state = true;
                relays[_loop].port->BSRR = relays[_loop].pin;
                // Останавливаем таймеры калибровки, так как заехали на петлю
                recal_timer_pending = false;
                long_recal_timer_pending = false;
            }
        } else {
            if (val_lpf > Calibrations.ThresholdLowHyst) {
                // Set relay OFF
                out_state = false;
                relays[_loop].port->BSRR = (relays[_loop].pin << 16);
                SEGGER_RTT_printf(0, "%c)[%u] loop free\n", _name, current_tick);

                // Запускаем таймеры калибровки после съезда с петли
                long_recal_timer_pending = true;
                long_recal_timer_time = current_tick + LongCalibrationTime;
                recal_timer_pending = true;
                recal_timer_time = current_tick + iShortCalibrationTime;
            }
        }
    }

    if (!out_state) {
        if (utils::in_range<int32_t>(Calibrations.lta_low, val_lpf, Calibrations.lta_high)) {
            filter_lta_cma.add(filter_lta.sample(raw));
            lta_value = filter_lta_cma.value();
        }
    }

    uint8_t gled = utils::GetGreenLevel(utils::GetLedBar<int32_t>(val_lpf, Calibrations.CalibrationValue)) | (out_state << 6);
    lc.SetGreen(_bar, gled);

    sprintf(PrintfBuffer, "%c)%u:%d:%s:%d%s\n", _name, current_tick, e.value, s, (int)lta_value, out_state ? "+" : "-");
    lc.UartPrint(TransmitBuffer, PrintfBuffer);

    CheckTimeout(e);
    return false;
}


void Loop::CheckTimeout(const MeasureEv &e) {
    TickType_t t = xTaskGetTickCount();

    if ((t > recal_timer_time) and recal_timer_pending) {
        recal_timer_pending = false;
        auto cal = filter_lta.value();
        Calibrations.CalcCalibration(cal, fSensitivity);
        sprintf(PrintfBuffer, "%c)recal:%d:%d:%d\n", _name, (int)Calibrations.CalibrationValue, (int)Calibrations.ThresholdLow, (int)Calibrations.ThresholdLowHyst);
        lc.UartPrint(TransmitBuffer, PrintfBuffer);
        SEGGER_RTT_printf(0, "%c)[%u] recal:%d\n", _name, t, (int)cal);
    }

    if ((t > long_recal_timer_time) and long_recal_timer_pending) {
        SEGGER_RTT_printf(0, "%c)[%u] long_recal pend", _name, t);
        long_recal_timer_time = t + LongCalibrationTime;
        auto cal = filter_lta.value();
        auto error = etl::absolute(cal - Calibrations.CalibrationValue);
        if (error > 4) {
            Calibrations.CalcCalibration(cal, fSensitivity);
            sprintf(PrintfBuffer, "%c)lrecal:%d:%d:%d\n", _name, (int)Calibrations.CalibrationValue, (int)Calibrations.ThresholdLow, (int)Calibrations.ThresholdLowHyst);
            lc.UartPrint(TransmitBuffer, PrintfBuffer);
            SEGGER_RTT_printf(0, " %d", (int)cal);
        }
        SEGGER_RTT_printf(0, "\n");
    }
}


void Loop::OnCalibratingEnter() {
    SEGGER_RTT_printf(0, MODNAME"%c) Entering calibrate\n", _name);
    iCalibrationEndTime = xTaskGetTickCount() + CalibrationTime;
    iGuardCounter = 0;
    Calibrations.CalibrationValue = 0;
    adata.clear();
    sprintf(PrintfBuffer, "%c) OnCalibratingEnter\n", _name);
    lc.UartPrint(TransmitBuffer, PrintfBuffer);
}


bool Loop::MeasureEventError(const MeasureEv &e) {
    if (e.value == 0 or e.value == 1 or e.value < 900) {
        relays[_loop].port->BRR = relays[_loop].pin;

        lc.SetGreen(_bar, 0x00);
        lc.SetRedState(_bar, true);
        return true;
    }
    return false;
}


__attribute__((unused)) LoopError Loop::GetLoopError(float value) {
    lc.SetRedState(_bar, false);
    return LE_NO_ERROR;
}

LoopError Loop::GetLoopError(int32_t value) {
    lc.SetRedState(_bar, false);
    return LE_NO_ERROR;
}


bool Loop::CalibrationError() {
    auto err = GetLoopError(Calibrations.CalibrationValue);
    return (err == LE_SHORT) || (err == LE_OPEN);
}

bool Loop::CalibrationValid() {
    return GetLoopError(Calibrations.CalibrationValue) == LE_NO_ERROR;
}

bool Loop::CalibrationFatalError() {
    return GetLoopError(Calibrations.CalibrationValue) == LE_FATAL;
}

void Loop::ErrorEntry() {
    // TODO Заменить 83 на нормальное значение дефайна
    iErrorTimeout = 83; // ~ 10 секунд в состоянии ошибки
}

bool Loop::ErrorGuard() {
    if (iErrorTimeout == 0)
        return true;

    iErrorTimeout--;
    return false;
}

bool Loop::CalibratingGuard(const MeasureEv &e) {
    TickType_t tick = xTaskGetTickCount();

    auto cal = filter_sample.sample(e.value);
    sprintf(PrintfBuffer, "%c)[%u] Cal M: %d, F: %d\n", _name, tick, e.value, (int)cal);
    lc.UartPrint(TransmitBuffer, PrintfBuffer);

    if (tick > iCalibrationEndTime)
        return true;

    iGuardCounter++;
    if (iGuardCounter % 2 == 0) {
        lc.SetGreen(_bar, 0xFF);
    } else {
        lc.SetGreen(_bar, 0x00);
    }
    return false;
}


void Loop::CalibratingDone() {
    lc.SetGreen(_bar, 0);
    sprintf(PrintfBuffer, "%c) Calibration Done\n", _name);
    lc.UartPrint(TransmitBuffer, PrintfBuffer);
    vTaskDelay(5);

    auto cal = filter_sample.value();

    sprintf(PrintfBuffer, "%c) Sensitivity: %d\n", _name, iSensitivity);
    SEGGER_RTT_printf(0, MODNAME"%s", PrintfBuffer);
    lc.UartPrint(TransmitBuffer, PrintfBuffer);
    vTaskDelay(5);

    sprintf(PrintfBuffer, "%c) Sel: %d\n", _name, bSelection);
    SEGGER_RTT_printf(0, MODNAME"%s", PrintfBuffer);
    lc.UartPrint(TransmitBuffer, PrintfBuffer);
    vTaskDelay(5);

    Calibrations.CalcCalibration(cal, fSensitivity);
    char *s;
    char b[FLOAT_BUFFER_SIZE] = {};
    s = Calibrations.PrintCalibration(b);
    sprintf(PrintfBuffer, "%c) Cal value: %s\n", _name, s);
    SEGGER_RTT_printf(0, MODNAME"%s", PrintfBuffer);
    lc.UartPrint(TransmitBuffer, PrintfBuffer);
    vTaskDelay(5);

    memset(b, 0, sizeof(b));
    s = Calibrations.PrintThresholdLow(b);
    sprintf(PrintfBuffer, "%c) Threshold Low: %s\n", _name, s);
    SEGGER_RTT_printf(0, MODNAME"%s", PrintfBuffer);
    lc.UartPrint(TransmitBuffer, PrintfBuffer);
    vTaskDelay(5);

    memset(b, 0, sizeof(b));
    s = Calibrations.PrintThresholdLowHyst(b);
    sprintf(PrintfBuffer, "%c) Threshold Low Hyst: %s\n", _name, s);
    SEGGER_RTT_printf(0, MODNAME"%s", PrintfBuffer);
    lc.UartPrint(TransmitBuffer, PrintfBuffer);
    vTaskDelay(5);

    sprintf(PrintfBuffer, "%c)recal:%d:%d:%d\n", _name, (int)Calibrations.CalibrationValue, (int)Calibrations.ThresholdLow, (int)Calibrations.ThresholdLowHyst);
    lc.UartPrint(TransmitBuffer, PrintfBuffer);
    // Включаем таймер длительной калибровки
    long_recal_timer_pending = true;
    long_recal_timer_time = xTaskGetTickCount() + LongCalibrationTime;
}


bool Loop::PreCalibratingGuard(const MeasureEv &e) {
    if (iGuardCounter > 0) {
        adata.push_back(e.value);
    }
    if (iGuardCounter == PRECALIBRATION_COUNT) {
        return true;
    }

    iGuardCounter++;
    if (iGuardCounter % 2 == 0) {
        lc.SetGreen(_bar, 0xFF);
    } else {
        lc.SetGreen(_bar, 0x00);
    }
    return false;
}


void Loop::PreCalDone() {
    sprintf(PrintfBuffer, "%c) PreCalibration Done\n", _name);
    lc.UartPrint(TransmitBuffer, PrintfBuffer);

    // Простой фильтр, выкидываем самый старший и самый младший элементы, как самые маловероятные
    asort(adata);
    adata[0] = 0;
    adata[adata.size() - 1] = 0;

    uint32_t cal = 0;
    for (size_t i = 1; i < adata.size() - 1; i++) {
        auto el = adata[i];
        cal += el;
        sprintf(PrintfBuffer, "%c) FreqPre: %d\n", _name, el);
        lc.UartPrint(TransmitBuffer, PrintfBuffer);
        vTaskDelay(1);
    }

    cal = cal / (PRECALIBRATION_COUNT - 2);
    sprintf(PrintfBuffer, "%c) PreCal: %u\n", _name, (unsigned int)cal);
    lc.UartPrint(TransmitBuffer, PrintfBuffer);

    Multiplier mul;
    mul.multiplier = 50000 / cal + 1;
    if (mul.multiplier > 400) {
        mul.multiplier = 400;
    }
    sprintf(PrintfBuffer, "%c) Mul: %u\n", _name, mul.multiplier);
    lc.UartPrint(TransmitBuffer, PrintfBuffer);
    tim_multiplier = mul.multiplier;

    mul.loop = _loop;
    xQueueSend(lc.xQueueMultiplier, &mul, 10);
}

bool Loop::PreCalCheck() {
    if (tim_multiplier == 1) {
        lc.SetRedState(_bar, true);
        lc.SetGreenLevel(_bar, 0);
        return false;
    }
    return true;
}

}


lc::LoopController &getLC() {
    return lc::ctrl;
}


extern "C" void USART1_IRQHandler() {
    using namespace lc;
    if (LL_USART_IsEnabledIT_TXE(MODBUS_UART) and LL_USART_IsActiveFlag_TXE(MODBUS_UART)) {
        if (TransmitBuffer.Count() == 1) {
            LL_USART_DisableIT_TXE(MODBUS_UART);
            LL_USART_EnableIT_TC(MODBUS_UART);
        }

        char val = 0;
        TransmitBuffer.Read(val);
        LL_USART_TransmitData8(MODBUS_UART, val);
    }

    if (LL_USART_IsEnabledIT_TC(MODBUS_UART) and LL_USART_IsActiveFlag_TC(MODBUS_UART)) {
        LL_USART_ClearFlag_TC(MODBUS_UART);
        LL_USART_DisableIT_TC(MODBUS_UART);
        TransmitBuffer.isTransmitting = 0;
    }

    if (LL_USART_IsEnabledIT_ERROR(MODBUS_UART) and LL_USART_IsActiveFlag_NE(MODBUS_UART)) {
        __NOP();
    }
}
