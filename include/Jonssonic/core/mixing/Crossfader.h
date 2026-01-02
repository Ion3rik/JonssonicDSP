// Jonssonic - A Modular Realtime C++ Audio DSP Library
// Crossfader class header file
// SPDX-License-Identifier: MIT

#pragma once

#include <jonssonic/utils/math_utils.h>
#include <jonssonic/core/common/circular_audio_buffer.h>
#include <cmath>

namespace jonssonic::core::mixing
{
/**
 * @brief Crossfader for audio signals.
 * 
 * Crossfades audio signals using equal-power law for smooth perceptual transitions.
 * Addionally supports latency compensation for the dry signal via a circular delay buffer.
 * @param T Sample data type (e.g., float, double)
 */

template<typename T>
class Crossfader
{
public:
    Crossfader() = default;
    ~Crossfader() = default;

    // No copy or move semantics
    Crossfader(const Crossfader&) = delete;
    Crossfader& operator=(const Crossfader&) = delete;
    Crossfader(Crossfader&&) = delete;
    Crossfader& operator=(Crossfader&&) = delete;

    /**
     * @brief Prepare the mixer for processing.
     * @param newNumChannels Number of channels to process
     * @param maxLatencySamples Maximum latency in samples for latency compensation
     */
    void prepare(size_t newNumChannels, size_t maxLatencySamples = 0)
    {
        numChannels = newNumChannels;
        latencyBuffer.resize(newNumChannels, maxLatencySamples);
        crossfadeSamplePos = 0;
    }

    /**
     * @brief Reset crossfader state
     */
    void reset()
    {
        latencyBuffer.clear();
    }

    
    /**
     * @brief Start a new crossfade transition.
     */
        void startCrossfade(size_t newCrossfadeTimeSamples) {
            crossFadeTimeSamples = newCrossfadeTimeSamples;
            crossfadeSamplePos = 0;
        }

    /**
     * @brief Process a block of samples with equal-power crossfading.
     * @param dryInput Dry signal input pointers (one per channel)
     * @param wetInput Wet signal input pointers (one per channel)
     * @param output Output signal pointers (one per channel)
     * @param numSamples Number of samples to process
     * 
     * @note Uses equal-power crossfading law for perceptually smooth transitions.
     *       All arrays must have the same number of channels as prepared.
     */
    void processBlock(const T* const* input1, const T* const* input2, T* const* output, size_t numSamples, size_t input1DelaySamples = 0)
    {
        for (size_t n = 0; n < numSamples; ++n)
        {
            T crossFadeOn = static_cast<T>(crossfadeSamplePos < crossFadeTimeSamples);
            T fadeFactor = static_cast<T>(crossfadeSamplePos) / static_cast<T>(crossFadeTimeSamples-1) * crossFadeOn + (T(1) - crossFadeOn);

            T g1 = std::cos(fadeFactor * pi_over_2<T>);
            T g2 = std::sin(fadeFactor * pi_over_2<T>);

            for (size_t ch = 0; ch < numChannels; ++ch)
            {
                T input1sample = input1[ch][n];
                latencyBuffer.write(ch, input1sample);
                input1sample = latencyBuffer.read(ch, input1DelaySamples);

                output[ch][n] = input1sample * g1 + input2[ch][n] * g2;
            }
            crossfadeSamplePos += static_cast<size_t>(crossFadeOn);
        }
    }

private:
    size_t numChannels = 0;
    size_t crossFadeTimeSamples = 2048; // Crossfade time in samples
    size_t crossfadeSamplePos = 0; // Current position in crossfade
    CircularAudioBuffer<T> latencyBuffer; // Circular buffer for dry signal delay compensation
};

} // namespace jonssonic::core::mixing