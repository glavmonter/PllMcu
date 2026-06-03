#ifndef INC_LEDS_H_
#define INC_LEDS_H_

#include "common.h"


/*
 * Светодиоды на плате расположены не по номерам битов, а следующим образом:
 * Верх платы
 * Bit   Color
 *  0    g[6]
 *  1    g[5]
 *  2    g[4]
 *  3    g[3]
 *  7    g[2]
 *  6    g[1]
 *  5    g[0]
 *  4    red
 */


typedef enum {
    BAR_A = 0,
    BAR_B,
    BAR_C,
    BARS_MAX
} BarNumber;


class Leds {
public:
    Leds() {
        Normalize();
    }
    void set(uint8_t mask) {
        leds.dw = mask;
        Normalize();
    }

    void Normalize() {
        // Перебросить выводы из виртуальных в реальные на плате индикатора
        //                HW  Virtual
        leds_normalize.b4 = leds.b7; // leds_normalize.set(4, leds[7]);
        leds_normalize.b5 = leds.b0; // leds_normalize.set(5, leds[0]);
        leds_normalize.b6 = leds.b1; // leds_normalize.set(6, leds[1]);
        leds_normalize.b7 = leds.b2; // leds_normalize.set(7, leds[2]);
        leds_normalize.b3 = leds.b3; // leds_normalize.set(3, leds[3]);
        leds_normalize.b2 = leds.b4; // leds_normalize.set(2, leds[4]);
        leds_normalize.b1 = leds.b5; // leds_normalize.set(1, leds[5]);
        leds_normalize.b0 = leds.b6; // leds_normalize.set(0, leds[6]);
        _value = ~leds_normalize.dw;
    }

    void set_red(bool val) {
        leds.b7 = val ? 1 : 0;
        Normalize();
    }

    void set_green(uint8_t val) {
        for (int i = 0; i < 7; i++) {
            uint8_t tst = (1 << i);
            if (val & tst)
                leds.dw |= tst;
            else
                leds.dw &= ~tst;
        }
        Normalize();
    }

    void set_all(uint8_t val) {
        for (int i = 0; i < 8; i++) {
            if (val & (1 << i))
                leds.dw |= (1 << i);
            else
                leds.dw &= ~(1 << i);
        }
        Normalize();
    }

    uint8_t value() const {
        return _value;
    }

private:
    _u8 leds{};
    _u8 leds_normalize{};
    uint8_t _value = 0;
};

#endif /* INC_LEDS_H_ */
