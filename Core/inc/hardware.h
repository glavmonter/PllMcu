#ifndef INC_HARDWARE_H_
#define INC_HARDWARE_H_

#include "config.h"

// SPI Chip select
#define SPI1_CS_PIN         LL_GPIO_PIN_6
#define SPI1_CS_PORT        GPIOA

#define SPI1_SCK_PORT       GPIOA
#define SPI1_SCK_PIN        LL_GPIO_PIN_5

#define SPI1_MOSI_PORT      GPIOA
#define SPI1_MOSI_PIN       LL_GPIO_PIN_7
#define SPI1_GPIO_AF        LL_GPIO_AF_0
#define SPI1_GPIO_PORT      GPIOA

// USART1_TX - PA9
#define MODBUS_TX_PIN       LL_GPIO_PIN_9
#define MODBUS_TX_PORT      GPIOA
#define MODBUS_TX_GPIO_AF   LL_GPIO_AF_4

// USART1_RX - PA10
#define MODBUS_RX_PIN       LL_GPIO_PIN_10
#define MODBUS_RX_PORT      GPIOA
#define MODBUS_RX_GPIO_AF   LL_GPIO_AF_4

#define MODBUS_UART         USART1


#endif /* INC_HARDWARE_H_ */
