// Jonssonic - A C++ audio DSP library
// Flanger effect header file
// SPDX-License-Identifier: MIT

#pragma once

#include "../core/delays/DelayLine.h"
#include "../core/generators/Oscillator.h"
#include "../core/common/DspParam.h"
#include "../core/filters/UtilityFilters.h"
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
    // Tunable constants
    static constexpr T MAX_MODULATION_MS = T(5.0);      // Maximum modulation depth in milliseconds (±3ms at depth=1.0)
    static constexpr int SMOOTHING_TIME_MS = 100;       // Smoothing time for parameter changes in milliseconds
    static constexpr T MAX_DELAY_MS = T(15.0);          // Maximum delay buffer size
    static constexpr T MAX_FEEDBACK = T(0.9);          // Maximum feedback amount to avoid instability
    
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
     */
    void prepare(size_t newNumChannels, T newSampleRate)
    {
        // Store global parameters
        numChannels = newNumChannels;
        sampleRate = newSampleRate;
        
        // Prepare DSP components
        delayLine.prepare(newNumChannels, newSampleRate, MAX_DELAY_MS);
        lfo.prepare(newNumChannels, newSampleRate);
        lfo.setWaveform(Waveform::Triangle); 
        lfo.setAntiAliasing(false);
        
        // DC blocker setup
        dcBlocker.prepare(newNumChannels, newSampleRate);

        // Set parameter safety bounds
        phaseOffset.setBounds(T(0), T(1));
        feedback.setBounds(T(-MAX_FEEDBACK), T(MAX_FEEDBACK));

        // Initialize parameters
        phaseOffset.prepare(newNumChannels, newSampleRate, SMOOTHING_TIME_MS);
        depthInSamples.prepare(newNumChannels, newSampleRate, SMOOTHING_TIME_MS);
        feedback.prepare(newNumChannels, newSampleRate, SMOOTHING_TIME_MS);

        // Set Default Parameters
        setRate(T(0.5), true);          // 0.5 Hz
        setDepth(T(0.5), true);         // 50% depth
        setFeedback(T(0.0), true);      // No feedback
        setDelayMs(T(5.0), true);       // 5 ms center delay
        setSpread(T(0.0), true);        // No spread
    }

    /**
     * @brief Reset the effect state and clear buffers.
     */
    void reset()
    {
        delayLine.reset();
        lfo.reset();
        dcBlocker.reset();
        phaseOffset.reset();
        depthInSamples.reset();
        feedback.reset();
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
                // process LFO with phase offset
                T lfoValue = lfo.processSample(ch, phaseOffset.getNextValue(ch));

                // Convert to unipolar [0, 1]
                lfoValue = lfoValue * T(0.5) + T(0.5);
                
                // Scale LFO by modulation depth
                lfoValue *= depthInSamples.getNextValue(ch);
                
                // Read delayed sample with modulation
                T delayedSample = delayLine.readSample(ch, lfoValue);
        
                // Compute feedback with DC blocking
                T feedbackSignal = delayedSample * feedback.getNextValue(ch);
                //feedbackSignal = dcBlocker.processSample(ch, feedbackSignal);
                
                // Compute what to write back (input + delayed feedback)
                T toWrite = input[ch][n] + feedbackSignal;
                delayLine.writeSample(ch, toWrite);

                // Mix dry and delayed (50/50 for classic flanger comb filtering)
                output[ch][n] = input[ch][n] + delayedSample;
            }
        }
    }

    /**
     * @brief Set the LFO rate (modulation speed).
     * @param rateHz Rate in Hz (typical range: 0.1 - 5 Hz for flanging)
     */
    void setRate(T rateHz, bool skipSmoothing = false)
    {
        lfo.setFrequency(rateHz);
    }

    /**
     * @brief Set the modulation depth.
     * @param depth Depth amount 0.0 - 1.0
     *                    (0 = no modulation, 1 = full ±3ms modulation)
     */
    void setDepth(T depth, bool skipSmoothing = false)
    {
        if (sampleRate > T(0))
        {
            depthInSamples.setTarget((MAX_MODULATION_MS * depth * sampleRate) / T(1000.0), skipSmoothing);
        }
    }

    /**
     * @brief Set the feedback amount.
     * @param feedbackAmount Feedback amount -1.0 to 1.0
     *                       (negative = inverted feedback)
     *                       WARNING: |feedback| >= 1.0 causes runaway gain
     */
    void setFeedback(T feedbackAmount, bool skipSmoothing = false)
    {
        feedback.setTarget(feedbackAmount, skipSmoothing);
    }

    /**
     * @brief Set the center delay time.
     * @param delayMs Center delay in milliseconds (typical range: 1 - 7 ms)
     *                The LFO will modulate around this center point.
     */
    void setDelayMs(T delayMs, bool skipSmoothing = false)
    {
        delayLine.setDelayMs(delayMs, skipSmoothing);
    }

    /**
     * @brief Set the channel spread (phase offset between channels).
     * @param spread Spread amount 0.0 - 1.0
     *                     0.0 = mono (all channels in phase)
     *                     1.0 = maximum spread (phase distributed across channels)
     */
    void setSpread(T spread, bool skipSmoothing = false)
    {
        // Update phase offsets based on spread (normalized to 0-1)
        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            phaseOffset.setTarget(ch, (spread * ch) / T(numChannels), skipSmoothing);
        }
    }

private:
    // Global Parameters
    size_t numChannels = 0;
    T sampleRate = T(0);

    // Core processors
    DelayLine<T, LagrangeInterpolator<T>, SmootherType::OnePole, 1, SMOOTHING_TIME_MS> delayLine;
    Oscillator<T, SmootherType::OnePole, 1, SMOOTHING_TIME_MS> lfo;
    DCBlocker<T> dcBlocker;

    // DSP parameters
    DspParam<T> phaseOffset;
    DspParam<T> depthInSamples;
    DspParam<T> feedback;
};

} // namespace Jonssonic
