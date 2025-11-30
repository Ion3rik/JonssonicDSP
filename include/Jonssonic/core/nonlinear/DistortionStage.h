// Jonssonic - A C++ audio DSP library
// DistortionStage class header file
// SPDX-License-Identifier: MIT

#pragma once

#include "WaveShaper.h"
#include "../common/DspParam.h"
#include <cstddef>
#include <algorithm>
#include <vector>

namespace Jonssonic {

/**
 * @brief Modular distortion stage with gain, bias, and asymmetry.
 *
 * @tparam SampleType   Sample data type (e.g., float, double)
 * @tparam ShaperType   Nonlinear shaping type (from WaveShaperType)
 */
template <typename SampleType, WaveShaperType ShaperType>

class DistortionStage {
public:
   DistortionStage() = default;
   ~DistortionStage() = default;

    // no copy semantics nor move semantics
    DistortionStage(const DistortionStage&) = delete;
    const DistortionStage& operator=(const DistortionStage&) = delete;
    DistortionStage(DistortionStage&&) = delete;
    const DistortionStage& operator=(DistortionStage&&) = delete;

    void prepare(size_t newNumChannels, SampleType sampleRate, SampleType smoothTimeMs = SampleType(10)) {
        numChannels = newNumChannels;
        inputGain.prepare(newNumChannels, sampleRate, smoothTimeMs);
        outputGain.prepare(newNumChannels, sampleRate, smoothTimeMs);
        bias.prepare(newNumChannels, sampleRate, smoothTimeMs);
        asymmetry.prepare(newNumChannels, sampleRate, smoothTimeMs);
    }

    void reset() {
        inputGain.reset();
        outputGain.reset();
        bias.reset();
        asymmetry.reset();
    }

    SampleType processSample(SampleType x, size_t channel) {
        // Apply input gain
        x *= inputGain.getNextValue(channel);
        // Apply bias
        x += bias.getNextValue(channel);
        // Apply asymmetry
        SampleType asym = asymmetry.getNextValue(channel);
        if (asym != SampleType(0)) {
            if (x >= SampleType(0))
                x *= (SampleType(1) + asym);
            else
                x *= (SampleType(1) - asym);
        }
        // Apply waveshaping
        x = shaper.processSample(x);
        // Apply output gain
        x *= outputGain.getNextValue(channel);
        return x;
    }
private:
    DspParam<SampleType> inputGain;
    DspParam<SampleType> outputGain;
    DspParam<SampleType> bias;
    DspParam<SampleType> asymmetry;
    size_t numChannels;
    WaveShaper<SampleType, WaveShaperType::Custom> shaper;
};

} // namespace Jonssonic
