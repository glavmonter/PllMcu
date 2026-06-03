#include "utils.h"


namespace utils {

    uint8_t GetGreenLevel(uint8_t level) {
        switch (level) {
            case 0:
                return 0b0000000;
            case 1:
                return 0b0000001;
            case 2:
                return 0b0000011;
            case 3:
                return 0b0000111;
            case 4:
                return 0b0001111;
            case 5:
                return 0b0011111;
            case 6:
                return 0b0111111;
            case 7:
            default:
                return 0b1111111;
        }
    }

}  // namespace utils
