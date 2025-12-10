// Jonssonic - A C++ audio DSP library
// Delay effect header file
// SPDX-License-Identifier: MIT

#pragma once
#include "../core/delays/DelayLine.h"
#include "../core/filters/FirstOrderFilter.h"
#include "../core/common/DspParam.h"
#include "../core/generators/Oscillator.h"
#include "../utils/MathUtils.h"
#include "../core/nonlinear/WaveShaper.h"

namespace Jonssonic {
/**
 * @brief Delay Effect with Feedback, Damping, and ping-pong cross-talk
 * @param T Sample data type (e.g., float, double)
 */
template<typename T>
class Delay {
public:
    // Tunable constants
    static constexpr T MAX_MODULATION_MS = T(3.0);          // Maximum modulation depth in milliseconds 
    static constexpr T WOW_PORTION_OF_MODULATION = T(0.7);  // Portion of modulation depth for wow effect
    static constexpr int SMOOTHING_TIME_MS = 100;           // Smoothing time for parameter changes in milliseconds


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
        
        // LFO setup
        wowLfo.prepare(newNumChannels, newSampleRate);
        wowLfo.setWaveform(Waveform::Sine);
        wowLfo.setFrequency(T(0.3)); // 0.3 Hz for wow
        
        flutterLfo.prepare(newNumChannels, newSampleRate);
        flutterLfo.setWaveform(Waveform::Sine);
        flutterLfo.setFrequency(T(6.0)); // 6 Hz for flutter

        // Set parameter bounds
        //feedback.setBounds(T(0), T(1));
        pingPong.setBounds(T(0), T(1));
        modDepth.setBounds(T(0), T(1));

        // Prepare parameters
        feedback.prepare(newNumChannels, newSampleRate, SMOOTHING_TIME_MS);
        pingPong.prepare(newNumChannels, newSampleRate, SMOOTHING_TIME_MS);
        modDepth.prepare(newNumChannels, newSampleRate, SMOOTHING_TIME_MS);

        // Set default parameter values
        delayLine.setDelayMs(T(500), true);
        feedback.setTarget(T(0), true);
        dampingFilter.setFreq(T(20000)); // no damping
        pingPong.setTarget(T(0), true);
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

                // Compute feedback: mix own channel with opposite channel (ping-pong)
                size_t oppositeCh = (ch + 1) % numChannels;
                T oppositeDelayed = delayLine.readSample(oppositeCh, totalModulation);
                
                T pingPongAmount = pingPong.getNextValue(ch);
                T mixedSample = (dampedSample * (T(1) - pingPongAmount) + oppositeDelayed * pingPongAmount);
                
                // Apply DC blocking to feedback path BEFORE soft clipping
                mixedSample = dcBlocker.processSample(ch, mixedSample);
                
                // Soft-clip first, then apply feedback gain
                T clippedSample = softClipper.processSample(mixedSample);
                T feedbackSample = clippedSample * feedback.getNextValue(ch);

                // For ping-pong: scale input by (1 - pingPong) so at max pingPong,
                // only channel 0 receives input, creating true ping-pong effect
                T inputGain = (ch == 0) ? T(1) : (T(1) - pingPongAmount);

                // Write input + soft-clipped feedback into delay line
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

    void setDamping(T newDampingHz, bool skipSmoothing = false) {
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

    // PROCESSORS
    DelayLine<T, LagrangeInterpolator<T>, SmootherType::OnePole, 1, SMOOTHING_TIME_MS> delayLine;
    FirstOrderFilter<T> dampingFilter; // damping lowpass filter
    FirstOrderFilter<T> dcBlocker; // DC blocker for feedback path
    Oscillator<T> wowLfo; // Slower LFO for wow effect
    Oscillator<T> flutterLfo; // Faster LFO for flutter effect
    WaveShaper<T, WaveShaperType::Atan> softClipper; // Soft clipper for feedback

    // PARAMS
    DspParam<T> feedback;       // Feedback amount
    DspParam<T> pingPong;      // Ping-pong amount
    DspParam<T> modDepth;        // Modulation depth (normalized)


};

} // namespace Jonssonic