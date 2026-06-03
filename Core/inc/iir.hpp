#ifndef INC_IIR_HPP_
#define INC_IIR_HPP_
#include "etl/algorithm.h"


template <typename Tinput, typename Toutput, const int order=2>
class IIR {
public:
    typedef Tinput output_type;

    IIR(const Tinput *a, const Tinput *b, Tinput gain): gain(gain) {
        etl::copy_n(a, order + 1, ACoeff);
        etl::copy_n(b, order + 1, BCoeff);
    }

    Tinput sample(Tinput sample) {
        // shift the old samples
        for (int n = order; n > 0; n--) {
            x[n] = x[n - 1];
            y[n] = y[n - 1];
        }

        x[0] = sample;
        y[0] = ACoeff[0] * x[0];
        for (int n = 1; n <= order; n++) {
            y[0] += ACoeff[n] * x[n] - BCoeff[n] * y[n];
        }
        y[0] /= BCoeff[0];

        last_value = y[0] / gain;
        return last_value;
    }

    Tinput value() {
        return last_value;
    }

private:
    Tinput last_value = 0;
    Tinput gain;
    Tinput ACoeff[order + 1]={};
    Tinput BCoeff[order + 1]={};

    //This variable should be signed (input sample width + Coefs width + 2 )-bit width to avoid saturation.
    Toutput y[order + 1] = {};  // Выходные узлы, знаковые
    Tinput  x[order + 1] = {};  // Входные узлы
};



namespace filter75But {
    // ФНЧ Баттерворда 2 порядка
    // Частота среза 75 Гц, дискретизация: 640 Гц (50000 сэмплов на частоте таймера 32 МГц)
    using IIR = IIR<int32_t, int64_t, 2>;
    const int32_t coeffa[3] = {11510, 23021, 11510};
    const int32_t coeffb[3] = {16384, -16462, 5833};
    constexpr int32_t gain = 8;
}

namespace filter75Cheb2 {
    // ФНЧ Чебышева 2 порядка
    // Частота среза 75 Гц, дискретизация: 640 Гц (50000 сэмплов на частоте таймера 32 МГц)
    using IIR = IIR<int32_t, int64_t, 2>;
    const int32_t coeffa[3] = {10975, 21950, 10975};
    const int32_t coeffb[3] = {32768, -20321, 9503};
    constexpr int32_t gain = 2;
}


namespace filter25Cheb {
    // ФНЧ Чебышева 3 порядка
    // Частота среза 25 Гц, дискретизация: 640 Гц (50000 сэмплов на частоте таймера 32 МГц)
    using IIR = IIR<int32_t, int64_t, 3>;
    const int32_t coeffa[4] = {7339, 22019, 22019, 7339};
    const int32_t coeffb[4] = {8192, -20984, 18404, -5496};
    constexpr int32_t gain = 512;
}

namespace filter75Cheb4 {
    // ФНЧ Чебышева 4 порядка
    // Частота среза 75 Гц, дискретизация: 640 Гц (50000 сэмплов на частоте таймера 32 МГц)
    using IIR = IIR<int32_t, int64_t, 4>;
    const int32_t coeffa[5] = {3378, 13512, 20268, 13512, 3378};
    const int32_t coeffb[5] = {8192, -20137, 22057, -11935, 2671};
    constexpr int32_t gain = 64;
}


namespace filter75Cheb4f {
    // ФНЧ Чебышева 4 порядка
    // Частота среза 75 Гц, дискретизация: 640 Гц (50000 сэмплов на частоте таймера 32 МГц)
    using IIR = IIR<float, float, 4>;
    const float coeffa[5] = {0.00644319980693206910f, 0.02577279922772827600f, 0.03865919884159241300f, 0.02577279922772827600f, 0.00644319980693206910f};
    const float coeffb[5] = {1.00000000000000000000f, -2.45818580480551410000f, 2.69254849287928000000f, -1.45696423452679480000f, 0.32610134060666474000f};
    constexpr int32_t gain = 1;
}


namespace filter10Cheb2i {
    /*
     * Filter type: Low Pass
     * Filter model: Butterworth
     * Filter order: 2
     * Sampling Frequency: 640 Hz
     * Cut Frequency: 10.000000 Hz
     * Coefficents Quantization: 16-bit
     * Z domain Zeros
     * z = -1.000000 + j 0.000000
     * z = -1.000000 + j 0.000000
     * Z domain Poles
     * z = 0.930664 + j -0.065006
     * z = 0.930664 + j 0.065006
     */
    using IIR = IIR<int32_t, int64_t, 2>;
    const int32_t coeffa[3] = {9302, 18604, 9302};
    const int32_t coeffb[3] = {16384, -30496, 14260};
    constexpr int32_t gain = 256;
}


namespace filter10bessel2i {
    /*
     * Filter type: Low Pass
     * Filter model: Bessel
     * Filter order: 2
     * Sampling Frequency: 600 Hz
     * Cut Frequency: 10.000000 Hz
     * Coefficents Quantization: 16-bit
     * Z domain Zeros
     *   z = -1.000000 + j 0.000000
     *   z = -1.000000 + j 0.000000
     * Z domain Poles
     *   z = 0.911957 + j -0.047520
     *   z = 0.911957 + j 0.047520
     */
    using IIR = IIR<int32_t, int64_t, 2>;
    const int32_t coeffa[3] = {10491, 20983, 10491};
    const int32_t coeffb[3] = {16384, -29883, 13663};
    constexpr int32_t gain = 256;
}

namespace filter10bessel4i {
    /*
     * Filter type: Low Pass
     * Filter model: Bessel
     * Filter order: 4
     * Sampling Frequency: 100 Hz
     * Cut Frequency: 10.000000 Hz
     * Coefficents Quantization: 16-bit
     * Z domain Zeros
     *   z = -1.000000 + j 0.000000
     *   z = -1.000000 + j 0.000000
     *   z = -1.000000 + j 0.000000
     *   z = -1.000000 + j 0.000000
     * Z domain Poles
     *   z = 0.538304 + j -0.101868
     *   z = 0.538304 + j 0.101868
     *   z = 0.570643 + j -0.350070
     *   z = 0.570643 + j 0.350070
     */
    using IIR = IIR<int32_t, int64_t, 4>;
    const int32_t coeffa[5] = {4495, 17982, 26974, 17982, 4495};
    const int32_t coeffb[5] = {8192, -18169, 16196, -6759, 1102};
    constexpr int32_t gain = 128;
}


namespace filter5Bessel4i {
    /*
     * Filter type: Low Pass
     * Filter model: Bessel
     * Filter order: 4
     * Sampling Frequency: 100 Hz
     * Cut Frequency: 5.000000 Hz
     * Coefficents Quantization: 16-bit
     * Z domain Zeros
     *   z = -1.000000 + j 0.000000
     *   z = -1.000000 + j 0.000000
     *   z = -1.000000 + j 0.000000
     *   z = -1.000000 + j 0.000000
     * Z domain Poles
     *   z = 0.749121 + j -0.041454
     *   z = 0.749121 + j 0.041454
     *   z = 0.783838 + j -0.218334
     *   z = 0.783838 + j 0.218334
     */

    using IIR = IIR<int32_t, int64_t, 4>;
    const int32_t coeffa[5] = {3265, 13060, 19591, 13060, 3265};
    const int32_t coeffb[5] = {8192, -25116, 29276, -15355, 3053};
    constexpr int32_t gain = 1024;
}


namespace filter3bessel4i {
    /*
     * Filter type: Low Pass
     * Filter model: Bessel
     * Filter order: 4
     * Sampling Frequency: 100 Hz
     * Cut Frequency: 3.000000 Hz
     * Coefficents Quantization: 16-bit
     * Z domain Zeros
     *   z = -1.000000 + j 0.000000
     *   z = -1.000000 + j 0.000000
     *   z = -1.000000 + j 0.000000
     *   z = -1.000000 + j 0.000000
     * Z domain Poles
     *   z = 0.823997 + j -0.000000
     *   z = 0.850269 + j 0.000000
     *   z = 0.876978 + j -0.148063
     *   z = 0.876978 + j 0.148063
     */

    using IIR = IIR<int32_t, int64_t, 4>;
    const int32_t coeffa[5] = {4068, 16275, 24413, 16275, 4068};
    const int32_t coeffb[5] = {4096, -14042, 18138, -10458, 2270};
    constexpr int32_t gain = 16384;
}

namespace bessel_test {
    /*
     * Filter type: Low Pass
     * Filter model: Bessel
     * Filter order: 4
     * Sampling Frequency: 600 Hz
     * Cut Frequency: 15.000000 Hz
     * Coefficents Quantization: 16-bit
     * Z domain Zeros
     *   z = -1.000000 + j 0.000000
     *   z = -1.000000 + j 0.000000
     *   z = -1.000000 + j 0.000000
     *   z = -1.000000 + j 0.000000
     * Z domain Poles
     *   z = 0.780868 + j -0.000000
     *   z = 0.885229 + j -0.154463
     *   z = 0.885229 + j 0.154463
     *   z = 0.969914 + j 0.000000
     */

    using IIR = IIR<int32_t, int64_t, 4>;
    const int32_t coeffa[5] = {4043, 16173, 24260, 16173, 4043};
    const int32_t coeffb[5] = {4096, -14423, 19106, -11283, 2505};
    constexpr int32_t gain = 32768;
}


namespace bessel_test2 {
    /*
     * Filter type: Low Pass
     * Filter model: Bessel
     * Filter order: 2
     * Sampling Frequency: 200 Hz
     * Cut Frequency: 5.000000 Hz
     * Coefficents Quantization: 16-bit
     * Z domain Zeros
     *   z = -1.000000 + j 0.000000
     *   z = -1.000000 + j 0.000000
     * Z domain Poles
     *   z = 0.869843 + j -0.068750
     *   z = 0.869843 + j 0.068750
     */
    using IIR = IIR<int32_t, int64_t, 2>;
    const int32_t coeffa[3] = {11366, 22733, 11366};
    const int32_t coeffb[3] = {16384, -28503, 12474};
    constexpr int32_t gain = 128;
}
#endif /* INC_IIR_HPP_ */
