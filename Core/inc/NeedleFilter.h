#ifndef INC_NEEDLEFILTER_H_
#define INC_NEEDLEFILTER_H_

#include <cstdint>

class NeedleFilter {
public:
    explicit NeedleFilter(int32_t threshold);
    ~NeedleFilter() = default;

    uint16_t append(uint16_t sample, int &needle);
    uint16_t memory_dump[3] = {0};

private:
    int32_t m_iThreshold;
    int32_t m_iThresholdInv;
    uint16_t memory[3] = {0};
    bool CheckNeedle();
    void DeleteNeedle();
};


#endif /* INC_NEEDLEFILTER_H_ */
