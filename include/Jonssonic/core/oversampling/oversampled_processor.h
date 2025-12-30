// Jonssonic - A C++ audio DSP library
// OversampledProcessor - Runtime-switchable oversampling wrapper
// SPDX-License-Identifier: MIT

#pragma once
#include "Oversampler.h"
#include "../common/AudioBuffer.h"

namespace Jonssonic {

/**
 * @brief Runtime-switchable oversampling processor wrapper
 * 
 * Wraps multiple compile-time configured Oversampler instances and provides
 * runtime switching between oversampling factors (1x, 2x, 4x, 8x, 16x).
 * 
 * This class handles all oversampling orchestration (upsample → process → downsample)
 * and allows you to provide any processing function via a callback/lambda.
 * 
 * @tparam T Sample data type (e.g., float, double)
 * 
 * Example usage:
 * @code
 *   OversampledProcessor<float> processor;
 *   processor.prepare(2, 512); // 2 channels, 512 samples max input block size
 *   
 *   // Process with 4x oversampling
 *   processor.process(4, input, output, 2, 512,
 *       [&](const auto** in, auto** out, size_t ch, size_t n) {
 *           hardClipper.processBlock(in, out, ch, n);
 *       });
 * @endcode
 */
template<typename T>
class OversampledProcessor {
public:
    OversampledProcessor() = default;
    ~OversampledProcessor() = default;

    // No copy or move semantics
    OversampledProcessor(const OversampledProcessor&) = delete;
    OversampledProcessor& operator=(const OversampledProcessor&) = delete;
    OversampledProcessor(OversampledProcessor&&) = delete;
    OversampledProcessor& operator=(OversampledProcessor&&) = delete;

    /**
     * @brief Prepare all oversamplers and allocate buffers
     * @param numChannels Number of audio channels
     * @param maxBlockSize Maximum block size in samples
     */
    void prepare(size_t newNumChannels, size_t maxInputBlockSize) {
        numChannels = newNumChannels;
        oversampler2x.prepare(numChannels, maxInputBlockSize);
        oversampler4x.prepare(numChannels, maxInputBlockSize);
        oversampler8x.prepare(numChannels, maxInputBlockSize);
        oversampler16x.prepare(numChannels, maxInputBlockSize);
        
        // Allocate buffer for maximum oversampling (16x)
        oversampledBuffer.resize(numChannels, maxInputBlockSize * 16);
    }

    /**
     * @brief Reset all oversamplers and clear buffers
     */
    void reset() {
        oversampler2x.reset();
        oversampler4x.reset();
        oversampler8x.reset();
        oversampler16x.reset();
        oversampledBuffer.clear();
    }

    /**
     * @brief Process audio with specified oversampling factor
     * @tparam ProcessFunc Callable type for processing (lambda, function, etc.)
     * @param factor Oversampling factor (1, 2, 4, 8, or 16)
     * @param input Input audio buffer (array of channel pointers)
     * @param output Output audio buffer (array of channel pointers)
     * @param numSamples Number of samples per channel
     * @param processFunc Function to call for processing: (const T**, T**, size_t channels, size_t samples)
     * 
     * The processFunc is called with upsampled audio and should process in-place or to output.
     */
    template<typename ProcessFunc>
    void processBlock(int factor, const T* const* input, T* const* output, 
                 size_t numSamples, ProcessFunc&& processFunc) {
        switch (factor) {
            case 1:  // No oversampling
                processFunc(input, output, numSamples);
                break;
                
            case 2:  // 2x oversampling
                processWithOversampling(oversampler2x, input, output, numSamples, 
                                       std::forward<ProcessFunc>(processFunc));
                break;
                
            case 4:  // 4x oversampling
                processWithOversampling(oversampler4x, input, output, numSamples,
                                       std::forward<ProcessFunc>(processFunc));
                break;
                
            case 8:  // 8x oversampling
                processWithOversampling(oversampler8x, input, output, numSamples,
                                       std::forward<ProcessFunc>(processFunc));
                break;
                
            case 16: // 16x oversampling
                processWithOversampling(oversampler16x, input, output, numSamples,
                                       std::forward<ProcessFunc>(processFunc));
                break;
                
            default: // Fallback to no oversampling
                processFunc(input, output, numSamples);
                break;
        }
    }

    /**
     * @brief Get total latency in samples at base sample rate
     * @param factor Current oversampling factor
     * @return Latency in samples
     */
    size_t getLatencySamples(int factor) const {
        switch (factor) {
            case 2:  return oversampler2x.getLatencySamples();
            case 4:  return oversampler4x.getLatencySamples();
            case 8:  return oversampler8x.getLatencySamples();
            case 16: return oversampler16x.getLatencySamples();
            default: return 0;
        }
    }

private:
    /**
     * @brief Internal helper: upsample → process → downsample
     */
    template<size_t Factor, typename ProcessFunc>
    void processWithOversampling(Oversampler<T, Factor>& Oversampler, 
                                const T* const* input, T* const* output,
                                size_t numSamples, ProcessFunc&& processFunc) {
        // Upsample
        size_t oversampledSamples = Oversampler.upsample(input, oversampledBuffer.writePtrs(), numSamples);
        
        // Process at higher sample rate
        processFunc(oversampledBuffer.readPtrs(), oversampledBuffer.writePtrs(), oversampledSamples);
        
        // Downsample
        Oversampler.downsample(oversampledBuffer.readPtrs(), output, numSamples);
    }

    // State
    size_t numChannels = 0;

    // Oversampler instances for each factor
    Oversampler<T, 2> oversampler2x;
    Oversampler<T, 4> oversampler4x;
    Oversampler<T, 8> oversampler8x;
    Oversampler<T, 16> oversampler16x;
    
    // Internal buffer for oversampled audio
    AudioBuffer<T> oversampledBuffer;
};

} // namespace Jonssonic
