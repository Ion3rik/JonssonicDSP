// Jonssonic - A C++ audio DSP library
// OverSampler class header file
// SPDX-License-Identifier: MIT

#pragma once
#include "../common/AudioBuffer.h"
#include "../common/Interpolators.h"


namespace Jonssonic {
/**
 * @brief Oversampler class for upsampling and downsampling audio signals.
 * @tparam T Sample data type (e.g., float, double)
 * @tparam Factor Oversampling factor (e.g., 2 for 2x oversampling)
 */

template<
    typename T, 
    size_t Factor, 
    bool firstStageIIR = false,
    size_t IIROrder = 4,
    size_t FIRTaps = 31>

class OverSampler {
public:
    OverSampler() = default;
    ~OverSampler() = default;

    // No copy or move semantics
    OverSampler(const OverSampler&) = delete;
    OverSampler& operator=(const OverSampler&) = delete;
    OverSampler(OverSampler&&) = delete;
    OverSampler& operator=(OverSampler&&) = delete;

    void prepare(size_t newNumChannels, T newSampleRate) {
        numChannels = newNumChannels;
        sampleRate = newSampleRate;
        // Prepare all stages
        if constexpr (Factor >= 2) {
            stage1.prepare(newNumChannels, newSampleRate);
        }
        if constexpr (Factor >= 4) {
            stage2.prepare(newNumChannels, newSampleRate * 2);
        }
        if constexpr (Factor >= 8) {
            stage3.prepare(newNumChannels, newSampleRate * 4);
        }
    }

    void upsample(const T* const* input, T* const* output, size_t numInputSamples) {
        // Stage 1 (2x): IIR or FIR selectable
        // Stage 2 (optional, 2x): always FIR
        // Stage 3 (optional, 2x): always FIR
        // Not implemented yet
    }

    void downsample(const T* const* input, T* const* output, size_t numOutputSamples) {
        // Stage 3 (optional, 2x): always FIR
        // Stage 2 (optional, 2x): always FIR
        // Stage 1 (2x): IIR or FIR selectable
        // Not implemented yet
    }

    static constexpr size_t getUpsampledLength(size_t inputLength) { return inputLength * Factor; }
    static constexpr size_t getDownsampledLength(size_t inputLength) { return inputLength / Factor; }
private:
    // Global state
    size_t numChannels = 0;
    T sampleRate = T(44100);

    // Stage 1: 2x up/down, selectable IIR or FIR
    using Stage1 = std::conditional_t<firstStageIIR, IIRStage, FIRHalfbandStage>;
    // Stage 2: 2x up/down, always FIR
    using Stage2 = FIRHalfbandStage;
    // Stage 3: 2x up/down, always FIR
    using Stage3 = FIRHalfbandStage;

    // Compose stages based on Factor
    Stage1 stage1;
    Stage2 stage2;
    Stage3 stage3;
};

} // namespace Jonssonic