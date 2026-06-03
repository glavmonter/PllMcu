#ifndef INC_UTILS_H_
#define INC_UTILS_H_
#include "etl/absolute.h"

namespace utils {

    template <typename T>
        bool in_range(T low, T value, T high) {
            return (value > low) and (value < high);
    }

    template <typename T>
    uint8_t GetLedBar(T value, T calibration) {
        auto delta = etl::absolute(value - calibration);
        uint8_t green_bar = 0;

        if (delta > 3000) {
            green_bar = 7;
        } else if (in_range<int32_t>(2500, delta, 2999)) {
            green_bar = 6;
        } else if (in_range<int32_t>(2000, delta, 2499)) {
            green_bar = 5;
        } else if (in_range<int32_t>(1500, delta, 1999)) {
            green_bar = 4;
        } else if (in_range<int32_t>(1000, delta, 1499)) {
            green_bar = 3;
        } else if (in_range<int32_t>(500, delta, 999)) {
            green_bar = 2;
        } else if (in_range<int32_t>(0, delta, 499)) {
            green_bar = 1;
        } else {
            green_bar = 1;
        }
        return green_bar;
    }

    uint8_t GetGreenLevel(uint8_t level);
}


#endif /* INC_UTILS_H_ */
