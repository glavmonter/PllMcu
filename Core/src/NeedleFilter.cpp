#include <cstring>
#include "NeedleFilter.h"

NeedleFilter::NeedleFilter(int32_t threshold) : m_iThreshold(threshold), m_iThresholdInv(-threshold) {
}

uint16_t NeedleFilter::append(uint16_t sample, int &needle) {
    needle = -1;
    memory[0] = memory[1];
    memory[1] = memory[2];
    memory[2] = sample;

    memory_dump[0] = memory_dump[1];
    memory_dump[1] = memory_dump[2];
    memory_dump[2] = sample;

    if (m_iThreshold > 0 and CheckNeedle()) {
        needle = memory[1];
        DeleteNeedle();
    }
    return memory[0];
}


bool NeedleFilter::CheckNeedle() {
    int32_t delta_l = memory[0] - memory[1];
    int32_t delta_r = memory[2] - memory[1];

    bool all_over = (delta_l > m_iThreshold) and (delta_r > m_iThreshold);
    bool all_under = (delta_l < m_iThresholdInv) and (delta_r < m_iThresholdInv);
    return all_over or all_under;
}

void NeedleFilter::DeleteNeedle() {
    uint32_t ave = (memory[0] + memory[2]) >> 1;
    memory[1] = ave;
}
