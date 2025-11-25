// Jonssonic - A C++ audio DSP library
// DspParam class with, smoothing and modulation support
// Author: Jon Fagerström
// Update: 23.11.2025

#pragma once
#include "Jonssonic/core/common/SmoothedValue.h"
#include <limits>

namespace Jonssonic {
/**
 * @brief DSP parameter class with smoothing and modulation capabilities.
 */
template<typename T, SmootherType Type = SmootherType::OnePole, int Order = 1>
class DspParam {
public:

    DspParam() = default;
    ~DspParam() = default;

    // no copy semantics nor move semantics
    DspParam(const DspParam&) = delete;
    const DspParam& operator=(const DspParam&) = delete;
    DspParam(DspParam&&) = delete;
    const DspParam& operator=(DspParam&&) = delete;

    void prepare(size_t newNumChannels, T newSampleRate, T newTimeMs = T(10)) {
        smoother.prepare(newNumChannels, newSampleRate, newTimeMs);
    }

    void reset() {
        smoother.reset();
    }

    void setBounds(T newMin, T newMax) {
        min = newMin;
        max = newMax;
    }

    // Additive modulation: base + mod (clamped)
    T applyAdditiveMod(T mod, size_t ch) {
        return clamp(getNextValue(ch) + mod);
    }

    // Multiplicative modulation: base * mod (clamped)
    T applyMultiplicativeMod(T mod, size_t ch) {
        return clamp(getNextValue(ch) * mod);
    }

    // Force set current value on all channels (clamped)
    DspParam& operator=(const T& value) {
        smoother = clamp(value);
        return *this;
    }

    // Set target value for all channels (clamped)
    void setTarget(T value) {
        smoother.setTarget(clamp(value));
    }
    // Set target value for specific channel (clamped)
    void setTarget(T value, size_t ch) {
        smoother.setTarget(clamp(value), ch);
    }

    T getNextValue(size_t ch) {
        return smoother.getNextValue(ch);
    }

    T getCurrentValue(size_t ch) const {
        return smoother.getCurrentValue(ch);
    }

    T getTargetValue(size_t ch) const {
        return smoother.getTargetValue(ch);
    }

private:
    SmoothedValue<T, Type, Order> smoother;
    T min = std::numeric_limits<T>::lowest();
    T max = std::numeric_limits<T>::max();

    // Clamp helper
    T clamp(T value) const {
        return std::clamp(value, min, max);
    }
};

} // namespace Jonssonic