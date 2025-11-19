// Jonssonic - A C++ audio DSP library
// Flanger effect header file
// Author: Jon Fagerström
// Update: 19.11.2025

#pragma once

#include "../core/delays/DelayLine.h"
#include "../core/generators/Oscillator.h"
#include "../core/common/Interpolators.h"
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
    /**
     * @brief Construct a Flanger effect.
     * @param numChannels Number of audio channels (default 0, call prepare() before use)
     * @param maxDelayMs Maximum delay time in milliseconds (default 10ms)
     * @param sampleRate Sample rate in Hz (default 44100)
     */
    Flanger(size_t numChannels = 0, T maxDelayMs = T(10.0), T sampleRate = T(44100.0))
        : numChannels(numChannels),
          sampleRate(sampleRate),
          maxDelayMs(maxDelayMs),
          delayLine(0, 0),
          lfo(0, sampleRate),
          feedbackBuffer(numChannels)
    {
        if (numChannels > 0 && sampleRate > 0) {
            prepare(numChannels, sampleRate);
        }
        
        // Set default parameters
        setRate(T(0.5));        // 0.5 Hz LFO
        setDepth(T(0.0));       // 0% - effect bypassed initially
        setFeedback(T(0.0));    // 0% - no feedback initially
        setBaseDelay(T(3.0));   // 3ms base delay
        
        // Configure LFO
        lfo.setWaveform(Waveform::Sine);
        lfo.setAntiAliasing(false); // LFO doesn't need anti-aliasing
    }

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
        numChannels = newNumChannels;
        sampleRate = newSampleRate;
        
        // Recalculate max delay in samples
        size_t maxDelaySamples = msToSamples(maxDelayMs, sampleRate);
        
        delayLine.prepare(newNumChannels, maxDelaySamples);
        lfo.prepare(newNumChannels, newSampleRate);
        lfo.setFrequency(rate);
        
        // Resize feedback buffer
        feedbackBuffer.resize(newNumChannels);
        for (auto& fb : feedbackBuffer) {
            fb.clear();
        }
        
        updateDelayParameters();
    }

    /**
     * @brief Reset the effect state and clear buffers.
     */
    void clear()
    {
        lfo.reset();

    }

    /**
     * @brief Set the LFO rate (modulation speed).
     * @param rateHz Rate in Hz (typically 0.1 - 5 Hz)
     */
    void setRate(T rateHz)
    {
        rate = std::max(T(0.01), std::min(rateHz, T(20.0)));
        lfo.setFrequency(rate);
    }

    /**
     * @brief Set the modulation depth.
     * @param depthAmount Depth amount 0.0 - 1.0 (0 = no modulation, 1 = full depth)
     */
    void setDepth(T depthAmount)
    {
        depth = std::max(T(0.0), std::min(depthAmount, T(1.0)));
        updateDelayParameters();
    }

    /**
     * @brief Set the feedback amount.
     * @param feedbackAmount Feedback amount -1.0 to 1.0 (negative = inverted feedback)
     */
    void setFeedback(T feedbackAmount)
    {
        feedback = std::max(T(-0.95), std::min(feedbackAmount, T(0.95)));
    }

    /**
     * @brief Set the base delay time.
     * @param delayMs Base delay in milliseconds (typically 0.5 - 5 ms)
     */
    void setBaseDelay(T delayMs)
    {
        baseDelayMs = std::max(T(0.1), std::min(delayMs, maxDelayMs));
        updateDelayParameters();
    }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input sample pointers (one per channel)
     * @param output Output sample pointers (one per channel) - wet signal only
     * @param numSamples Number of samples to process
     */
    void processBlock(const T* const* input, T* const* output, size_t numSamples)
    {
        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            const T* inChannel = input[ch];
            T* outChannel = output[ch];
            T* delayBuffer = delayLine.getBuffer().getChannelPointer(ch);
            std::vector<T>& fbBuffer = feedbackBuffer[ch];
            fbBuffer.resize(numSamples);

            size_t workingWriteIndex = delayLine.getWriteIndex();  // Local copy for this channel
            
            for (size_t i = 0; i < numSamples; ++i)
            {

            }

    }

private:
    void updateDelayParameters()
    {
        // Convert milliseconds to samples
        baseDelaySamples = msToSamples(baseDelayMs, sampleRate);
        depthInSamples = depth * baseDelaySamples;
        
        // Set the base delay
        delayLine.setDelay(baseDelaySamples);
    }

    size_t numChannels;
    T sampleRate;
    T maxDelayMs;

    // Effects components
    DelayLine<T, LinearInterpolator> delayLine;
    Oscillator<T> lfo;
    std::vector<std::vector<T>> feedbackBuffer;

    // Parameters
    T rate = T(0.5);          // LFO rate in Hz
    T depth = T(0.0);         // Modulation depth 0-1
    T feedback = T(0.0);      // Feedback amount -1 to 1
    T baseDelayMs = T(3.0);   // Base delay in ms

    // Computed values
    size_t baseDelaySamples = 0;
    T depthInSamples = T(0);
};

} // namespace Jonssonic
