#ifndef INC_HARDWARE_H_
#define INC_HARDWARE_H_

#include "config.h"

// LMX Lock detect input
#define LMX_LOCK_PIN       LL_GPIO_PIN_6
#define LMX_LOCK_PORT      GPIOA

// LMX Enable pin. Low - Power down, High - Active
#define LMX_ENABLE_PIN     LL_GPIO_PIN_4
#define LMX_ENABLE_PORT    GPIOA

// LMX SPI Chip select
#define LMX_CS_PIN          LL_GPIO_PIN_3
#define LMX_CS_PORT         GPIOA


#define SPI1_SCK_PORT       GPIOA
#define SPI1_SCK_PIN        LL_GPIO_PIN_5

#define SPI1_MOSI_PORT      GPIOA
#define SPI1_MOSI_PIN       LL_GPIO_PIN_7

#define SPI1_MISO_PORT      GPIOA
#define SPI1_MISO_PIN       LL_GPIO_PIN_6

#define SPI1_GPIO_AF        LL_GPIO_AF_0

// USART1_TX - PB6
#define SERIAL_TX_PIN       LL_GPIO_PIN_6
#define SERIAL_TX_PORT      GPIOB
#define SERIAL_TX_GPIO_AF   LL_GPIO_AF_0

// USART1_RX - PB7
#define SERIAL_RX_PIN       LL_GPIO_PIN_7
#define SERIAL_RX_PORT      GPIOB
#define SERIAL_RX_GPIO_AF   LL_GPIO_AF_0

#define SERIAL_UART         USART1


#endif /* INC_HARDWARE_H_ */
