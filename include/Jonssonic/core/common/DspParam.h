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

    void prepare(T sampleRate, T timeMs, T modTimeMs = 0) {
        baseSmoother.prepare(sampleRate, timeMs);
        modSmoother.prepare(sampleRate, modTimeMs);
    }

    void reset(T baseValue = 0, T modValue = 0)
    {
        baseSmoother.reset(baseValue);
        modSmoother.reset(modValue);
    }
    // Modulation helpers

    // Additive modulation: base + mod
    T applyAdditiveMod(T mod, bool smoothed = false) {
        T m = mod;
        if (smoothed) {
            modSmoother.setTarget(mod);
            m = modSmoother.getNextValue();
        }
        return getNextValue() + m;
    }


    // Multiplicative modulation: base * mod
    T applyMultiplicativeMod(T mod, bool smoothed = false) {
        T m = mod;
        if (smoothed) {
            modSmoother.setTarget(mod);
            m = modSmoother.getNextValue();
        }
        return getNextValue() * m;
    }

    // Set the modulation target (for smoothed modulation)
    void setModTarget(T value) {
        modSmoother.setTarget(value);
    }

    void setSampleRate(T newSampleRate) {
        baseSmoother.setSampleRate(newSampleRate);
    }

    void setSmoothingTimeMs(T newTimeMs) {
        baseSmoother.setTimeMs(newTimeMs);
    }

    void setTarget(T value) {
        baseSmoother.setTarget(value);
    }

    T getNextValue() {
        return baseSmoother.getNextValue();
    }

    T getCurrentValue() const {
        return baseSmoother.getCurrentValue();
    }

    T getTargetValue() const {
        return baseSmoother.getTargetValue();
    }
private:
    SmoothedValue<T, BaseType, BaseOrder> baseSmoother;
    SmoothedValue<T, ModType, ModOrder> modSmoother;
};

} // namespace Jonssonic