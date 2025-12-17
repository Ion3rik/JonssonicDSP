// Jonssonic - A C++ audio DSP library
// Delay effect header file
// SPDX-License-Identifier: MIT

#pragma once
#include "../core/delays/DelayLine.h"
#include "../core/filters/FirstOrderFilter.h"
#include "../core/filters/UtilityFilters.h"
#include "../core/common/DspParam.h"
#include "../core/generators/Oscillator.h"
#include "../utils/MathUtils.h"
#include "../core/nonlinear/WaveShaper.h"
#include <cmath>

namespace Jonssonic {
/**
 * @brief Delay Effect with Feedback, Damping, and ping-pong cross-talk
 * @param T Sample data type (e.g., float, double)
 */
template<typename T>
class Delay {
public:
    // Tunable constants
    static constexpr T MAX_MODULATION_MS = T(2.0);          // Maximum modulation depth in milliseconds 
    static constexpr T WOW_PORTION_OF_MODULATION = T(0.9);  // Portion of modulation depth for wow effect
    static constexpr int SMOOTHING_TIME_MS = 300;           // Smoothing time for parameter changes in milliseconds
    static constexpr T DAMPING_MIN_HZ = T(2000);            // Minimum damping frequency in Hz



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
        maxModsamples = msToSamples(MAX_MODULATION_MS, sampleRate);

        // Prepare DSP components
        delayLine.prepare(newNumChannels, newSampleRate, maxDelayMs);

        // Damping filter setup
        dampingFilter.prepare(newNumChannels, newSampleRate);
        dampingFilter.setType(FirstOrderType::Lowpass);
        dampingFilter.setFreq(T(18000)); // Default: no damping
        
        // Prepare DC blocker
        dcBlocker.prepare(newNumChannels, newSampleRate);
        
        // LFO setup
        wowLfo.prepare(newNumChannels, newSampleRate);
        wowLfo.setWaveform(Waveform::Sine);
        wowLfo.setFrequency(T(0.3)); // 0.3 Hz for wow
        
        flutterLfo.prepare(newNumChannels, newSampleRate);
        flutterLfo.setWaveform(Waveform::Sine);
        flutterLfo.setFrequency(T(6.0)); // 6 Hz for flutter

        // Set parameter bounds
        feedback.setBounds(T(0), T(1));
        pingPong.setBounds(T(0), T(1));
        modDepth.setBounds(T(0), T(1));

        // Prepare parameters
        feedback.prepare(newNumChannels, newSampleRate, SMOOTHING_TIME_MS);
        pingPong.prepare(newNumChannels, newSampleRate, SMOOTHING_TIME_MS);
        modDepth.prepare(newNumChannels, newSampleRate, SMOOTHING_TIME_MS);

        // Set default parameter values
        setFeedback(T(0), true);
        setPingPong(T(0), true);
        setModDepth(T(0), true);
        setDelayMs(T(500), true); // 500 ms default delay
        setDamping(T(0), true); // No damping by default
    }

    void reset() {
        delayLine.reset();
        dampingFilter.reset();
        dcBlocker.reset();
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
                T modAmount = modDepth.getNextValue(ch) * maxModsamples;
                T wowValue = wowLfo.processSample(ch) * WOW_PORTION_OF_MODULATION * modAmount;
                T flutterValue = flutterLfo.processSample(ch) * (T(1) - WOW_PORTION_OF_MODULATION) * modAmount;
                T totalModulation = wowValue + flutterValue;

                // Read delayed sample with modulation
                T delayedSample = delayLine.readSample(ch, totalModulation);
                
                // Apply damping filter to delayed sample
                T dampedSample = dampingFilter.processSample(ch, delayedSample);

                // Compute feedback: mix this channel with opposite channel (ping-pong)
                size_t oppositeCh = (ch + 1) % numChannels;
                T oppositeDelayed = delayLine.readSample(oppositeCh, totalModulation);
                T pingPongAmount = pingPong.getNextValue(ch);
                T mixedSample = (dampedSample * (T(1) - pingPongAmount) + oppositeDelayed * pingPongAmount);
                
                // Apply normalized feedback gain
                T fbGain = feedback.getNextValue(ch) * ((T(1) - pingPongAmount) + std::sqrt(T(0.5)) * pingPongAmount);// normalize for the cross-feedback
                T feedbackSample = mixedSample * fbGain;

                // Apply DC blocker to feedback path
                feedbackSample = dcBlocker.processSample(ch, feedbackSample);

                // For ping-pong: scale input by (1 - pingPong) so at max pingPong,
                // only channel 0 receives input, creating true ping-pong effect
                T inputGain = (ch == 0) ? T(1) : (T(1) - pingPongAmount);

                // Write input + feedback back into delay line
                delayLine.writeSample(ch, input[ch][n] * inputGain + feedbackSample);

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

    void setDamping(T newDamping, bool skipSmoothing = false) {
        T newDampingHz = DAMPING_MIN_HZ * std::pow(T(18000) / DAMPING_MIN_HZ, T(1) - newDamping);
        dampingFilter.setFreq(newDampingHz);
    }

    void setPingPong(T newPingPong, bool skipSmoothing = false) {
        pingPong.setTarget(newPingPong, skipSmoothing);
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
    T maxModsamples;

    // PROCESSORS
    DelayLine<T, LagrangeInterpolator<T>, SmootherType::OnePole, 1, SMOOTHING_TIME_MS> delayLine;
    FirstOrderFilter<T> dampingFilter; // damping lowpass filter
    DCBlocker<T> dcBlocker; // DC blocker for feedback path
    Oscillator<T> wowLfo; // Slower LFO for wow effect
    Oscillator<T> flutterLfo; // Faster LFO for flutter effect
    WaveShaper<T, WaveShaperType::Atan> softClipper; // Soft clipper for feedback

    // PARAMS
    DspParam<T> feedback;       // Feedback amount
    DspParam<T> pingPong;      // Ping-pong amount
    DspParam<T> modDepth;        // Modulation depth (normalized)


};

} // namespace Jonssonic