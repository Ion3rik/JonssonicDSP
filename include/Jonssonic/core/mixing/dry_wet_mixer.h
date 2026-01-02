// Jonssonic - A Modular Realtime C++ Audio DSP Library
// Dry/Wet Mixer class 
// SPDX-License-Identifier: MIT

#pragma once

#include <jonssonic/core/common/dsp_param.h>
#include <jonssonic/utils/math_utils.h>
#include <jonssonic/core/common/circular_audio_buffer.h>
#include <cmath>

namespace jonssonic::core::mixing
{
/**
 * @brief Dry/Wet mixer for audio signals.
 * 
 * Mixes dry and wet signals based on a mix parameter (0.0 = full dry, 1.0 = full wet).
 */

template<typename T>
class DryWetMixer
{
public:
    DryWetMixer() = default;
    ~DryWetMixer() = default;

    // No copy or move semantics
    DryWetMixer(const DryWetMixer&) = delete;
    DryWetMixer& operator=(const DryWetMixer&) = delete;
    DryWetMixer(DryWetMixer&&) = delete;
    DryWetMixer& operator=(DryWetMixer&&) = delete;

    /**
     * @brief Prepare the mixer for processing.
     * @param newNumChannels Number of channels to process
     * @param newSampleRate Sample rate in Hz
     * @param smoothingTimeMs Smoothing time for mix parameter changes (default: 15ms)
     */
    void prepare(size_t newNumChannels, T newSampleRate, T smoothingTimeMs = T(15), size_t maxDryDelaySamples = 0)
    {
        numChannels = newNumChannels;
        mix.prepare(newNumChannels, newSampleRate, smoothingTimeMs);
        mix.setBounds(T(0), T(1)); // Clamp between 0 and 1
        mix.setTarget(T(1), true); // Default to full wet
        dryDelayBuffer.resize(newNumChannels, maxDryDelaySamples);
    }

    /**
     * @brief Clear the mixer state (same as reset).
     */
    void reset()
    {
        mix.reset();
    }

    /**
     * @brief Set the dry/wet mix amount for all channels.
     * @param amount Mix amount (0.0 = full dry, 1.0 = full wet)
     * @param skipSmoothing If true, jump immediately to target value without smoothing
     */
    void setMix(T newMix, bool skipSmoothing = false)
    {
        mix.setTarget(newMix, skipSmoothing);
    }

    /**
     * @brief Set the dry/wet mix amount for a specific channel.
     * @param ch Channel index
     * @param amount Mix amount (0.0 = full dry, 1.0 = full wet)
     * @param skipSmoothing If true, jump immediately to target value without smoothing
     */
    void setMix(size_t ch, T newMix, bool skipSmoothing = false)
    {
        mix.setTarget(ch, newMix, skipSmoothing);
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
    void processBlock(const T* const* dryInput, const T* const* wetInput, T* const* output, size_t numSamples, size_t dryDelaySamples = 0)
    {
        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            for (size_t n = 0; n < numSamples; ++n)
            {
                T mixValue = mix.getNextValue(ch);
                
                // Equal-power crossfade: cos(x) for dry, sin(x) for wet
                T dryGain = std::cos(mixValue * pi_over_2<T>);
                T wetGain = std::sin(mixValue * pi_over_2<T>);

                // Apply dry delay if needed
                T drySample = dryInput[ch][n];
                dryDelayBuffer.write(ch, drySample);
                drySample = dryDelayBuffer.read(ch, dryDelaySamples);
                
                // Mix dry and wet signals
                output[ch][n] = drySample * dryGain + wetInput[ch][n] * wetGain;
            }
        }
    }

    /**
     * @brief Get reference to the mix parameter for advanced modulation.
     * @return Reference to the internal DspParam
     * 
     * @note Use this for direct parameter modulation or to access advanced DspParam features.
     */
    DspParam<T>& getMixParameter() { return mix; }
    
    /**
     * @brief Get const reference to the mix parameter.
     * @return Const reference to the internal DspParam
     */
    const DspParam<T>& getMixParameter() const { return mix; }

private:
    size_t numChannels = 0;
    common::DspParam<T, SmootherType::OnePole, 1> mix; // Mix parameter with smoothing
    common::CircularAudioBuffer<T> dryDelayBuffer; // Circular buffer for dry signal delay compensation
};

} // namespace jonssonic::core::mixing