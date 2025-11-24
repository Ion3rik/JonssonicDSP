// Jonssonic - A C++ audio DSP library
// DspParam class with, smoothing and modulation support
// Author: Jon Fagerström
// Update: 23.11.2025

#pragma once
#include "Jonssonic/core/common/SmoothedValue.h"

namespace Jonssonic {
/**
 * @brief DSP parameter class with smoothing and modulation capabilities.
 */
template<
    typename T,
    SmootherType BaseType = SmootherType::OnePole, int BaseOrder = 1,
    SmootherType ModType = SmootherType::None, int ModOrder = 1
>
class DspParam {
public:

    DspParam() = default;
    ~DspParam() = default;

    // no copy semantics nor move semantics
    DspParam(const DspParam&) = delete;
    const DspParam& operator=(const DspParam&) = delete;
    DspParam(DspParam&&) = delete;
    const DspParam& operator=(DspParam&&) = delete;

    void prepare(size_t newNumChannels, T timeMs, T sampleRate, T modTimeMs = 0) {
        baseSmoother.prepare(newNumChannels, timeMs, sampleRate);
        modSmoother.prepare(newNumChannels, modTimeMs, sampleRate);
    }

    void reset()
    {
        baseSmoother.reset();
        modSmoother.reset();
    }
    // Modulation helpers

    // Additive modulation: base + mod
    T applyAdditiveMod(T mod, size_t ch) {
        T m = modSmoother.process(mod, ch);
       return getNextValue(ch) + m;
    }

    // Multiplicative modulation: base * mod
    T applyMultiplicativeMod(T mod, size_t ch) {
        T m = modSmoother.process(mod, ch);
        return getNextValue(ch) * m;
    }

    // Force set current value on all channels
    DspParam& operator=(const T& value) {
        baseSmoother = value;
        return *this;
    }

    // Set target value for all channels
    void setTarget(T value) {
        baseSmoother.setTarget(value);
    }
    // Set target value for specific channel
    void setTarget(T value, size_t ch) {
        baseSmoother.setTarget(value, ch);
    }

    T getNextValue(size_t ch) {
        return baseSmoother.getNextValue(ch);
    }

    T getCurrentValue(size_t ch) const {
        return baseSmoother.getCurrentValue(ch);
    }

    T getTargetValue(size_t ch) const {
        return baseSmoother.getTargetValue(ch);
    }

private:
    SmoothedValue<T, BaseType, BaseOrder> baseSmoother;
    SmoothedValue<T, ModType, ModOrder> modSmoother;
};

} // namespace Jonssonic