#include <new>
#include "stm32f0xx_hal.h"
#include "common.h"
#include <FreeRTOS.h>


void *operator new(size_t size) {
    assert_param(0);
    void *p = pvPortMalloc(size);
    return p;
}

void operator delete(void *p) {
}


void delay_us(uint32_t us) {
volatile uint32_t cnt = us * 8;
    __asm volatile (
            "  .syntax unified    \n"
            "  mov    r1, %0      \n"
            "  .L_for:            \n"
            "  subs   r1, r1, #1  \n"
            "  bne    .L_for      \n"
            :
            : "r"(cnt)
            : "%r1"
    );
}



void asort(etl::ivector<uint16_t> &v) {
int i, j;
bool swapped;
int n = v.size();

    for (i = 0; i < n - 1; i++) {
        swapped = false;
        for (j = 0; j < n - i - 1; j++) {
            if (v[j] > v[j+1]) {
                uint16_t tmp = v[j];
                v[j] = v[j+1];
                v[j+1] = tmp;
                swapped = true;
            }
        }
        if (swapped == false) {
            break;
        }
    }
}


char *_float_to_char(float x, char *p) {
    char *s = p + FLOAT_BUFFER_SIZE - 1; // go to end of buffer
    uint16_t decimals;  // variable to store the decimals
    int units;  // variable to store the units (part to left of decimal place)
    if (x < 0) { // take care of negative numbers
        decimals = (int)(x * -1000) % 1000; // make 1000 for 3 decimals etc.
        units = (int)(-1 * x);
    } else { // positive numbers
        decimals = (int)(x * 1000) % 1000;
        units = (int)x;
    }

    *--s = (decimals % 10) + '0';
    decimals /= 10; // repeat for as many decimal places as you need
    *--s = (decimals % 10) + '0';
    decimals /= 10; // repeat for as many decimal places as you need
    *--s = (decimals % 10) + '0';
    *--s = '.';

    while (units > 0) {
        *--s = (units % 10) + '0';
        units /= 10;
    }
    if (x < 0) *--s = '-'; // unary minus sign for negative numbers
    return s;
}


