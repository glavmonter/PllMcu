#ifndef INC_ADC_H_
#define INC_ADC_H_

#include "stm32l0xx.h"
#include "stm32l0xx_ll_gpio.h"
#include "stm32l0xx_ll_adc.h"
#include "hardware.h"


#define USE_TIMEOUT     1

/* Init variable out of expected ADC conversion data range */
#define VAR_CONVERTED_DATA_INIT_VALUE    (__LL_ADC_DIGITAL_SCALE(LL_ADC_RESOLUTION_12B) + 1)
#define VDDA_APPLI                       ((uint32_t)3300)


void Configure_ADC();
void Activate_ADC();
void ConversionStartPoll_ADC_GrpRegular();



#endif /* INC_ADC_H_ */
