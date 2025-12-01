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
    size_t FIRTaps = 13>

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

    // FIR half-band stage for 2x up/downsampling
    struct FIRHalfbandStage {
        void prepare(size_t channels, T rate) {
            // Resize the state buffer
            stateBuffer.resize(channels, FIRTaps);
      
            // Set coefficients
            coeffs = getCoeffs();
        }

        void upsample(const T* const* input, T* const* output, size_t numInputSamples) {
            constexpr size_t N = FIRTaps;
            const size_t numChannels = stateBuffer.getNumChannels();
            for (size_t ch = 0; ch < numChannels; ++ch) {
                T* state = stateBuffer.getWritePointer(ch);
                const T* in = input[ch];
                T* out = output[ch];
                for (size_t n = 0; n < numInputSamples; ++n) {
                    // Shift state buffer left by 1, append new sample
                    for (size_t i = 0; i < N - 1; ++i) {
                        state[i] = state[i + 1];
                    }
                    state[N - 1] = in[n];
                    // Even phase: output sample at input position (all coeffs)
                    T y0 = T(0);
                    for (size_t k = 0; k < N; k += 2) {
                        y0 += coeffs[k] * state[N - 1 - k];
                    }
                    // Odd phase: output sample between input samples (odd coeffs only)
                    T y1 = T(0);
                    for (size_t k = 1; k < N; k += 2) {
                        y1 += coeffs[k] * state[N - 1 - k];
                    }
                    out[2 * n]     = y0;
                    out[2 * n + 1] = y1;
                }
            }
        }

        void downsample(const T* const* input, T* const* output, size_t numOutputSamples) {
            // Not implemented yet
        }

        static constexpr size_t filterLength = 13; // Example, real design may differ
        AudioBuffer<T> stateBuffer; 
        std::vector<T> coeffs;

        static std::vector<T> getCoeffs() {
            if constexpr (FIRTaps == 13) {
                // FIRTaps = 13 half-band low-pass filter coefficients
                return {
                    T(0.0), T(-0.008026), T(0.0), T(0.045635), T(0.0), T(-0.166478), T(0.5),
                    T(1.0),
                    T(0.5), T(-0.166478), T(0.0), T(0.045635), T(0.0), T(-0.008026), T(0.0)
                };
            } else {
                // Placeholder: zero coefficients for other tap counts
                // Replace with real design or generator for other tap counts
                std::vector<T> coeffs(FIRTaps, T(0));
                coeffs[FIRTaps/2] = T(1); // Center tap for pass-through
                return coeffs;
            }
        }
    };


    // IIR stage for 2x up/downsampling (skeleton)
    struct IIRStage {
        void prepare(size_t channels, T rate) {
            // Prepare IIR filter state for each channel
            // Not implemented yet
        }
        void upsample(const T* const* input, T* const* output, size_t numInputSamples) {
            // Not implemented yet
        }
        void downsample(const T* const* input, T* const* output, size_t numOutputSamples) {
            // Not implemented yet
        }
        // Add IIR filter state here
    };

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