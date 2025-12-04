// Jonssonic - A C++ audio DSP library
// OverSampler class header file
// SPDX-License-Identifier: MIT

#pragma once
#include "../common/AudioBuffer.h"
#include "../common/Interpolators.h"
#include "OverSamplerFilters.h"


namespace Jonssonic {
/**
 * @brief Oversampler class for upsampling and downsampling audio signals.
 * @tparam T Sample data type (e.g., float, double)
 * @tparam Factor Oversampling factor (supported: 2, 4, 8, 16)
 * @note Currently Fixed FIR halfband filters are used for each stage.
 */

template<typename T, size_t Factor = 4>
class OverSampler {
    // Compile-time checks
    static_assert(Factor == 2 || Factor == 4 || Factor == 8 || Factor == 16, "Supported oversampling Factors are 2, 4, 8, and 16");
public:
    OverSampler() = default;
    ~OverSampler() = default;

    // No copy or move semantics
    OverSampler(const OverSampler&) = delete;
    OverSampler& operator=(const OverSampler&) = delete;
    OverSampler(OverSampler&&) = delete;
    OverSampler& operator=(OverSampler&&) = delete;

    void prepare(size_t newNumChannels, size_t newMaxBlockSize) {
        numChannels = newNumChannels;
        
        // Prepare all stages
        if constexpr (Factor >= 2) {
            stage1.prepare(newNumChannels); // prepare stage 1
        }
        if constexpr (Factor >= 4) {
            stage2.prepare(newNumChannels); // prepare stage 2
            intermediateBuffer1to2.resize(newNumChannels, newMaxBlockSize*2); // buffer for factor 1 to 2 oversampling
        }
        if constexpr (Factor >= 8) {
            stage3.prepare(newNumChannels); // prepare stage 3
            intermediateBuffer2to4.resize(newNumChannels, newMaxBlockSize*4); // buffer for factor 2 to 4 oversampling
        }
        if constexpr (Factor == 16) {
            stage4.prepare(newNumChannels); // prepare stage 4
            intermediateBuffer4to8.resize(newNumChannels, newMaxBlockSize*8); // buffer for factor 4 to 8 oversampling
        }

    }

    void upsample(const T* const* input, T* const* output, size_t numInputSamples) {
        // Factor 2
        if constexpr (Factor == 2) {
            stage1.upsample(input, output, numInputSamples); // stage1 1x to 2x
        }

        // Factor 4
        if constexpr (Factor == 4) {
            // stage1 1x to 2x
            stage1.upsample(input, intermediateBuffer1to2.writePtrs(), numInputSamples); 
            // stage2 2x to 4x
            stage2.upsample(intermediateBuffer1to2.readPtrs(), output, 2*numInputSamples);
        }

        // Factor 8
        if constexpr (Factor == 8) {
            // stage1 1x to 2x
            stage1.upsample(input, intermediateBuffer1to2.writePtrs(), numInputSamples); 
            // stage2 2x to 4x
            stage2.upsample(intermediateBuffer1to2.readPtrs(), intermediateBuffer2to4.writePtrs(), 2*numInputSamples); 
            // stage3 4x to 8x
            stage3.upsample(intermediateBuffer2to4.readPtrs(), output, 4*numInputSamples);
        }

        // Factor 16
        if constexpr (Factor == 16) {
            // stage1 1x to 2x
            stage1.upsample(input, intermediateBuffer1to2.writePtrs(), numInputSamples); 
            // stage2 2x to 4x
            stage2.upsample(intermediateBuffer1to2.readPtrs(), intermediateBuffer2to4.writePtrs(), 2*numInputSamples); 
            // stage3 4x to 8x
            stage3.upsample(intermediateBuffer2to4.readPtrs(), intermediateBuffer4to8.writePtrs(), 4*numInputSamples); 
            // stage4 8x to 16x
            stage4.upsample(intermediateBuffer4to8.readPtrs(), output, 8*numInputSamples);
        }
    }
    void downsample(const T* const* input, T* const* output, size_t numOutputSamples) {
        // Factor 2
        if constexpr (Factor == 2) {
            stage1.downsample(input, output, numOutputSamples); // stage1 2x to 1x
        }

        // Factor 4
        if constexpr (Factor == 4) {
            // stage2 4x to 2x
            stage2.downsample(input, intermediateBuffer1to2.writePtrs(), 2*numOutputSamples);
            // stage1 2x to 1x
            stage1.downsample(intermediateBuffer1to2.readPtrs(), output, numOutputSamples);
        }

        // Factor 8
        if constexpr (Factor == 8) {
            // stage3 8x to 4x
            stage3.downsample(input, intermediateBuffer2to4.writePtrs(), 4*numOutputSamples);
            // stage2 4x to 2x
            stage2.downsample(intermediateBuffer2to4.readPtrs(), intermediateBuffer1to2.writePtrs(), 2*numOutputSamples);
            // stage1 2x to 1x
            stage1.downsample(intermediateBuffer1to2.readPtrs(), output, numOutputSamples);
        }

        // Factor 16
        if constexpr (Factor == 16) {
            // stage4 16x to 8x
            stage4.downsample(input, intermediateBuffer4to8.writePtrs(), 8*numOutputSamples);
            // stage3 8x to 4x
            stage3.downsample(intermediateBuffer4to8.readPtrs(), intermediateBuffer2to4.writePtrs(), 4*numOutputSamples);
            // stage2 4x to 2x
            stage2.downsample(intermediateBuffer2to4.readPtrs(), intermediateBuffer1to2.writePtrs(), 2*numOutputSamples);
            // stage1 2x to 1x
            stage1.downsample(intermediateBuffer1to2.readPtrs(), output, numOutputSamples);
        }
    }

    static constexpr size_t getUpsampledLength(size_t inputLength) { return inputLength * Factor; }
    static constexpr size_t getDownsampledLength(size_t inputLength) { return inputLength / Factor; }

    size_t getLatencySamples() const {
        T latency = 0;
        if constexpr (Factor >= 2) {
            latency += stage1.getLatencySamples();      // run at base rate
        }
        if constexpr (Factor >= 4) {
            latency += stage2.getLatencySamples() / 2; // run at 2x rate
        }
        if constexpr (Factor >= 8) {
            latency += stage3.getLatencySamples() / 4; // run at 4x rate
        }
        if constexpr (Factor == 16) {
            latency += stage4.getLatencySamples() / 8; // run at 8x rate
        }
        return static_cast<size_t>(latency); 
    }
private:
    // Global state
    size_t numChannels = 0;

    // FIR Halfband filter stages
    FIRHalfbandStage<T, 31> stage1;     // 2x stage
    FIRHalfbandStage<T, 31> stage2;     // 4x stage
    FIRHalfbandStage<T, 31> stage3;     // 8x stage
    FIRHalfbandStage<T, 31> stage4;     // 16x stage

    // Intermediate buffers for multi-stage processing
    AudioBuffer<T> intermediateBuffer1to2; // for 1x to 2x oversampling
    AudioBuffer<T> intermediateBuffer2to4; // for 2x to 4x oversampling
    AudioBuffer<T> intermediateBuffer4to8; // for 4x to 8x oversampling

};

} // namespace Jonssonic