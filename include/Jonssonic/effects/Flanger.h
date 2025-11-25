// Jonssonic - A C++ audio DSP library
// Flanger effect header file
// Author: Jon Fagerström
// Update: 19.11.2025

#pragma once

#include "../core/delays/DelayLine.h"
#include "../core/generators/Oscillator.h"
#include "../core/common/Interpolators.h"
#include "../core/common/AudioBuffer.h"
#include "../utils/MathUtils.h"
#include <vector>
#include <algorithm>

namespace Jonssonic
{

/**
 * @brief Flanger audio effect.
 * 
 * Classic flanger effect using a short modulated delay line with feedback.
 * Creates sweeping comb-filter effects by mixing the input signal with a 
 * time-varying delayed copy of itself.
 */
template<typename T>
class Flanger
{
public:
    /// Maximum modulation depth in milliseconds (±3ms at depth=1.0)
    static constexpr T MAX_MODULATION_MS = T(3.0);
    
    /**
     * @brief Default constructor for Flanger effect.
     * Call prepare() before processing audio.
     */
    Flanger() = default;

    ~Flanger() = default;

    // No copy or move semantics
    Flanger(const Flanger&) = delete;
    Flanger& operator=(const Flanger&) = delete;
    Flanger(Flanger&&) = delete;
    Flanger& operator=(Flanger&&) = delete;

    /**
     * @brief Prepare the flanger for processing.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     * @param newMaxDelayMs Maximum delay buffer size in milliseconds (default 10ms)
     */
    void prepare(size_t newNumChannels, T newSampleRate, T newMaxDelayMs = T(10.0))
    {
        numChannels = newNumChannels;
        sampleRate = newSampleRate;
        maxDelayMs = newMaxDelayMs;
        
        // Prepare components
        delayLine.prepare(newNumChannels, newSampleRate, newMaxDelayMs);
        lfo.prepare(newNumChannels, newSampleRate);

        // Initialize states
        feedbackState.assign(numChannels, T(0));
        phaseOffset.assign(numChannels, T(0));
        
        // Configure LFO
        lfo.setWaveform(Waveform::Sine);
        lfo.setAntiAliasing(false);
        lfo.setFrequency(lfoRate);
        
        // Set initial delay and depth
        delayLine.setDelayMs(centerDelayMs);
        depthInSamples = (MAX_MODULATION_MS * depth * sampleRate)* T(0.001);
    }

    /**
     * @brief Reset the effect state and clear buffers.
     */
    void clear()
    {
        delayLine.clear();
        std::fill(phaseOffset.begin(), phaseOffset.end(), T(0));
    }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input sample pointers (one per channel)
     * @param output Output sample pointers (one per channel) - wet signal only
     * @param numSamples Number of samples to process
     * 
     * @note Input and output must have the same number of channels as prepared.
     *       Processing is done sample-by-sample due to the feedback loop.
     */
    void processBlock(const T* const* input, T* const* output, size_t numSamples)
    {
        for (size_t ch = 0; ch < numChannels; ++ch) 
        {
            for (size_t n = 0; n < numSamples; ++n)
            {
                // Mix input with feedback
                T inputWithFeedback = input[ch][n] + feedbackState[ch] * feedback;
                
                // process LFO with phase offset
                T lfoValue = lfo.processSample(ch, phaseOffset[ch]);
                
                // Scale LFO by modulation depth
                lfoValue *= depthInSamples;
                
                // Process through delay line
                T delayedSample = delayLine.processSample(inputWithFeedback, lfoValue, ch);
        
                // Copy to output and update feedback state
                output[ch][n] = delayedSample;
                feedbackState[ch] = delayedSample;

            }
        }
    }

    /**
     * @brief Set the LFO rate (modulation speed).
     * @param rateHz Rate in Hz (typical range: 0.1 - 5 Hz for flanging)
     */
    void setRate(T rateHz)
    {
        lfoRate = rateHz;
        lfo.setFrequency(lfoRate);
    }

    /**
     * @brief Set the modulation depth.
     * @param depthAmount Depth amount 0.0 - 1.0
     *                    (0 = no modulation, 1 = full ±3ms modulation)
     */
    void setDepth(T depthAmount)
    {
        depth = depthAmount;
        if (sampleRate > T(0))
        {
            depthInSamples = (MAX_MODULATION_MS * depth * sampleRate) / T(1000.0);
        }
    }

    /**
     * @brief Set the feedback amount.
     * @param feedbackAmount Feedback amount -1.0 to 1.0
     *                       (negative = inverted feedback)
     *                       WARNING: |feedback| >= 1.0 causes runaway gain
     */
    void setFeedback(T feedbackAmount)
    {
        feedback = feedbackAmount;
    }

    /**
     * @brief Set the center delay time.
     * @param delayMs Center delay in milliseconds (typical range: 1 - 7 ms)
     *                The LFO will modulate around this center point.
     */
    void setDelayMs(T delayMs)
    {
        centerDelayMs = delayMs;
        delayLine.setDelayMs(delayMs);
    }

    /**
     * @brief Set the channel spread (phase offset between channels).
     * @param spreadAmount Spread amount 0.0 - 1.0
     *                     0.0 = mono (all channels in phase)
     *                     1.0 = maximum spread (phase distributed across channels)
     */
    void setSpread(T spreadAmount)
    {
        spread = spreadAmount;
        // Update phase offsets based on spread
        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            phaseOffset[ch] = (spread * T(2) * M_PI * ch) / T(numChannels);
        }
    }

private:
    // Configuration
    size_t numChannels = 0;
    T sampleRate = T(0);
    T maxDelayMs = T(0);

    // Core processors
    DelayLine<T, LinearInterpolator<T>> delayLine;
    Oscillator<T> lfo;

    // User parameters 
    T lfoRate = T(0.5);           // LFO rate in Hz
    T depth = T(0.7);             // Modulation depth 0-1 (70% default)
    T feedback = T(0.5);          // Feedback amount -1 to 1 (50% default)
    T centerDelayMs = T(3.0);     // Center delay in ms (3ms default)
    T spread = T(1.0);            // Channel spread 0-1 (100% default)

    // Internal parameters
    T depthInSamples = T(0);
    std::vector<T> phaseOffset;
    std::vector<T> feedbackState;
};

} // namespace Jonssonic
