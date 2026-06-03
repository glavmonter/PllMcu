#ifndef INC_TEMPLATES_H_
#define INC_TEMPLATES_H_

#include "stm32l0xx_hal.h"

#define EEPROM_BASE_ADDR    0x08080000


template <typename T>
    T ReadEEPROM(size_t offset) {
        auto address = reinterpret_cast<T *>(EEPROM_BASE_ADDR + offset);
    return *address;
}



#endif /* INC_TEMPLATES_H_ */
