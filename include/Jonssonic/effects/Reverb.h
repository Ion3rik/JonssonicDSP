// Jonssonic - A C++ audio DSP library
// Delay line class header file
// SPDX-License-Identifier: MIT

#pragma once
#include "../common/AudioBuffer.h"
#include "../../utils/MathUtils.h"
#include "../core/delays/DelayLine.h" 
#include "../core/common/DspParam.h"
#include "../core/filters/FirstOrderFilter.h"
#include "../../utils/BufferUtils.h"


namespace Jonssonic
{
/**
 * @brief Reverberation effect implmented with Feedback Delay Network (FDN)
 * @tparam T Sample data type (e.g., float, double)
 */

template<typename T>
class Reverb
{
public:
    // Tunable constants
    /**
     * @brief Smoothing time for parameter changes in milliseconds.
     */
    static constexpr int SMOOTHING_TIME_MS = 100;

    /**
     * @brief Maximum delay buffer size in milliseconds.
     */
    static constexpr T MAX_DELAY_MS = T(2000.0);    

    /**
     * @brief Number of delay lines in the FDN.
     */
    static constexpr size_t FDN_SIZE = 8;              


    // Constructors and Destructor
    Reverb() = default;
    Reverb(size_t newNumChannels, size_t maxBlockSize, T newSampleRate) {
        prepare(newNumChannels, maxBlockSize, newSampleRate);
    }
    ~Reverb() = default;

    // No copy or move semantics
    Reverb(const Reverb&) = delete;
    Reverb& operator=(const Reverb&) = delete;
    Reverb(Reverb&&) = delete;
    Reverb& operator=(Reverb&&) = delete;

    /**
     * @brief Prepare the reverb for processing.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     */
    void prepare(size_t newNumChannels, size_t maxBlockSize, T newSampleRate)
    {
        // Store global parameters
        numChannels = newNumChannels;
        sampleRate = newSampleRate;

        // Prepare internal interleaved buffer
        INTERLEAVED(interleavedBuffer.resize, numChannels, maxBlockSize);

        // Prepare FDN components
        INTERLEAVED(fdnDelays.prepare, FDN_SIZE, newSampleRate, MAX_DELAY_MS);
    }

    /**
     * @brief Reset the effect state and clear buffers.
     */
    void clear()
    {
        // Clear internal states
    }

    /**
     * @brief Process a block of audio samples.
     * @param input Input sample pointers (one per channel)
     * @param output Output sample pointers (one per channel)
     * @param numSamples Number of samples to process
     */
    void processBlock(const T* const* input, T* const* output, size_t numSamples)
    {   
        // Convert to interleaved and route to FDN internal buffers


        // Process FDN

        // Convert back to planar
    }

    //==============================================================================
    // SETTERS
    //==============================================================================

    /**
     * @brief Set the reverb time in seconds.
     * @param timeInSeconds Reverb time in seconds
     * @param skipSmoothing If true, skip smoothing of parameter change
     */
    void setReverbTimeS(T timeInSeconds, bool skipSmoothing = false)
    {
        // Set reverb time parameter
    }

    /**
     * @brief Set the size of the reverb space.
     *        In practise scales the FDN delay lengths.
     * @param sizeNormalized Size parameter normalized [0.0, 1.0]
     * @param skipSmoothing If true, skip smoothing of parameter change
     */
    void setSize(T sizeNormalized, bool skipSmoothing = false)
    {
        // Set size parameter
    }

    /**
     * @brief Set the pre delay time in milliseconds.
     * @param timeInMs Pre-delay time in milliseconds
     * @param skipSmoothing If true, skip smoothing of parameter change
     */
    void setPreDelayTimeMs(T timeInMs, bool skipSmoothing = false)
    {
        // Set pre-delay time parameter
    }


    /**
     * @brief Set the low cut frequency in Hz.
     * @param freqHz Low cut frequency in Hz
     * @param skipSmoothing If true, skip smoothing of parameter change
     */
    void setLowCutFrequencyHz(T freqHz, bool skipSmoothing = false)
    {
        // Set low cut frequency parameter
    }

    /**
     * @brief Set the damping amount.
     * @param dampingNormalized Damping amount normalized [0.0, 1.0]
     * @param skipSmoothing If true, skip smoothing of parameter change
     */
    void setDamping(T dampingNormalized, bool skipSmoothing = false)
    {
        // Set damping parameter
    }

    //==============================================================================
    // GETTERS
    //==============================================================================
    /**
     * @brief Get the number of channels.
     */
    size_t getNumChannels() const { return numChannels; }

    /**
     * @brief Get the sample rate in Hz.
     */
    T getSampleRate() const { return sampleRate; }


private:
    size_t numChannels = 0;
    T sampleRate = T(44100);

    // Processor components
    AudioBuffer<T> interleavedBuffer; // Temporary interleaved buffer for processing
    DelayLine<T, LinearInterpolator<T>, SmootherType::OnePole, 1, SMOOTHING_TIME_MS> fdnDelays; // FDN delay lines
    DelayLine<T, LinearInterpolator<T>, SmootherType::OnePole, 1, SMOOTHING_TIME_MS> preDelay; // Pre-delay line
    FirstOrderFilter<T> dampingFilter; // Low cut filter for damping


    

}
}; // namespace Jonssonic