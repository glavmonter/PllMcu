#ifndef INC_COMMON_H_
#define INC_COMMON_H_

#include <stdint.h>

#define PIN_SET(port, pin)   do { port->BSRR = pin; } while (0)
#define PIN_RESET(port, pin) do { port->BRR  = pin; } while (0)


void delay_us(uint32_t us);

union _u8 {
    uint8_t dw;
    struct {
        uint16_t  b0: 1;
        uint16_t  b1: 1;
        uint16_t  b2: 1;
        uint16_t  b3: 1;
        uint16_t  b4: 1;
        uint16_t  b5: 1;
        uint16_t  b6: 1;
        uint16_t  b7: 1;
    };
};


#define FLOAT_BUFFER_SIZE                   16
char *_float_to_char(float x, char *p);

#endif /* INC_COMMON_H_ */
