// Jonssonic - A C++ audio DSP library
// Delay effect header file
// SPDX-License-Identifier: MIT

#pragma once
#include "../core/delays/DelayLine.h"
#include "../core/filters/FirstOrderFilter.h"
#include "../core/common/DspParam.h"
#include "../core/generators/Oscillator.h"
#include "../utils/MathUtils.h"

namespace Jonssonic {
/**
 * @brief Delay Effect with Feedback, Damping, and cross-talk
 * @param T Sample data type (e.g., float, double)
 */
template<typename T>
class Delay {
public:
    // Tunable constants
    static constexpr T MAX_MODULATION_MS = T(5.0);          // Maximum modulation depth in milliseconds 
    static constexpr T WOW_PORTION_OF_MODULATION = T(0.7);  // Portion of modulation depth for wow effect
    static constexpr int SMOOTHING_TIME_MS = 100;           // Smoothing time for parameter changes in milliseconds
    static constexpr T MAX_DELAY_MS = T(15.0);              // Maximum delay buffer size


    // Constructor and Destructor
    Delay() = default;
    Delay(size_t newNumChannels, T newSampleRate, T maxDelayMs) {
        prepare(newNumChannels, newSampleRate, maxDelayMs);
    }
    ~Delay() = default;

    // No copy or move semantics
    Delay(const Delay&) = delete;
    Delay& operator=(const Delay&) = delete;
    Delay(Delay&&) = delete;
    Delay& operator=(Delay&&) = delete;

    /**
     * @brief Prepare the delay effect for processing.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     * @param maxDelayMs Maximum delay in milliseconds
     */
    void prepare(size_t newNumChannels, T newSampleRate, T maxDelayMs) {

        // Store global parameters
        numChannels = newNumChannels;
        sampleRate = newSampleRate;

        // Prepare DSP components
        delayLine.prepare(newNumChannels, newSampleRate, maxDelayMs);

        // Damping filter setup
        dampingFilter.prepare(newNumChannels, newSampleRate);
        dampingFilter.setType(FirstOrderType::Lowpass);
        dampingFilter.setFreq(T(20000)); // Default: no damping
        
        // DC blocker setup
        dcBlocker.prepare(newNumChannels, newSampleRate);
        dcBlocker.setType(FirstOrderType::Highpass);
        dcBlocker.setFreqNormalized(T(0.0005)); // ~22 Hz at 44.1kHz
        
        // LFO setup
        wowLfo.prepare(newNumChannels, newSampleRate);
        wowLfo.setWaveform(Waveform::Sine);
        wowLfo.setFrequency(T(0.5)); // 0.5 Hz for wow
        
        flutterLfo.prepare(newNumChannels, newSampleRate);
        flutterLfo.setWaveform(Waveform::Sine);
        flutterLfo.setFrequency(T(5.0)); // 5 Hz for flutter

        // Set parameter bounds
        feedback.setBounds(T(0), T(0.99));
        crossTalk.setBounds(T(0), T(1));
        modDepth.setBounds(T(0), T(1));

        // Prepare parameters
        feedback.prepare(newNumChannels, newSampleRate, SMOOTHING_TIME_MS);
        crossTalk.prepare(newNumChannels, newSampleRate, SMOOTHING_TIME_MS);
        modDepth.prepare(newNumChannels, newSampleRate, SMOOTHING_TIME_MS);

        // Set default parameter values
        delayLine.setDelayMs(T(500), true);
        feedback.setTarget(T(0), true);
        dampingFilter.setFreq(T(20000)); // no damping
        crossTalk.setTarget(T(0), true);
        modDepth.setTarget(T(0), true);
    }

    void clear() {
        delayLine.clear();
        dampingFilter.clear();
        dcBlocker.clear();
    }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input buffer (numChannels x numSamples)
     * @param output Output buffer (numChannels x numSamples)
     * @param numSamples Number of samples to process
     */
    void processBlock(const T* const* input, T* const* output, size_t numSamples)
    {
        for (size_t n = 0; n < numSamples; ++n)
        {
            for (size_t ch = 0; ch < numChannels; ++ch) 
            {   
                // Compute modulation value (wow + flutter)
                T modAmount = modDepth.getNextValue(ch) * MAX_MODULATION_MS;
                T wowValue = wowLfo.processSample(ch) * WOW_PORTION_OF_MODULATION * modAmount;
                T flutterValue = flutterLfo.processSample(ch) * (T(1) - WOW_PORTION_OF_MODULATION) * modAmount;
                T totalModulation = msToSamples(wowValue + flutterValue, sampleRate); // total modulation in samples

                // Read delayed sample with modulation
                T delayedSample = delayLine.readSample(ch, totalModulation);

                // Apply damping filter to delayed sample
                T dampedSample = dampingFilter.processSample(ch, delayedSample);

                // Compute cross-talk sample from opposite channel
                size_t oppositeCh = (ch + 1) % numChannels;
                T crossTalkSample = delayLine.readSample(oppositeCh, totalModulation);
                crossTalkSample = dampingFilter.processSample(oppositeCh, crossTalkSample);
                
                // Mix cross-talk into delayed sample
                dampedSample += crossTalk.getNextValue(ch) * crossTalkSample;

                // Compute feedback sample
                T feedbackSample = dampedSample * feedback.getNextValue(ch);
                
                // Apply DC blocking to feedback path
                feedbackSample = dcBlocker.processSample(ch, feedbackSample);

                // Write input + feedback into delay line
                delayLine.writeSample(ch, input[ch][n] + feedbackSample);

                // Output the damped delayed sample
                output[ch][n] = dampedSample;
            }
        }
    }

    // SETTERS FOR PARAMETERS
    void setDelayMs(T newDelayMs, bool skipSmoothing = false) {
        delayLine.setDelayMs(newDelayMs, skipSmoothing);
    }

    void setDelaySamples(T newDelaySamples, bool skipSmoothing = false) {
        delayLine.setDelaySamples(newDelaySamples, skipSmoothing);
    }

    void setFeedback(T newFeedback, bool skipSmoothing = false) {
        feedback.setTarget(newFeedback, skipSmoothing);
    }

    void setDamping(T newDampingHz, bool skipSmoothing = false) {
        dampingFilter.setFreq(newDampingHz);
    }

    void setCrossTalk(T newCrossTalk, bool skipSmoothing = false) {
        crossTalk.setTarget(newCrossTalk, skipSmoothing);
    }

    void setModDepth(T newModDepth, bool skipSmoothing = false) {
        modDepth.setTarget(newModDepth, skipSmoothing);
    }

    // GETTERS FOR GLOBAL PARAMETERS
    size_t getNumChannels() const {
        return numChannels;
    }
    T getSampleRate() const {
        return sampleRate;
    }

private:
    
    // GLOBAL PARAMETERS
    size_t numChannels = 0;
    T sampleRate = T(44100);

    // PROCESSORS
    DelayLine<T> delayLine; // core delay line
    FirstOrderFilter<T> dampingFilter; // damping lowpass filter
    FirstOrderFilter<T> dcBlocker; // DC blocker for feedback path
    Oscillator<T> wowLfo; // Slower LFO for wow effect
    Oscillator<T> flutterLfo; // Faster LFO for flutter effect

    // PARAMS
    DspParam<T> feedback;       // Feedback amount
    DspParam<T> crossTalk;      // Cross-talk amount
    DspParam<T> modDepth;        // Modulation depth (normalized)


};

} // namespace Jonssonic