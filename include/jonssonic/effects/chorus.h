// Jonssonic - A C++ audio DSP library
// Chorus effect header file
// SPDX-License-Identifier: MIT

#pragma once

#include "jonssonic/utils/detail/config_utils.h"
#include <jonssonic/core/common/dsp_param.h>
#include <jonssonic/core/delays/multi_tap_delay_line.h>
#include <jonssonic/core/generators/oscillator.h>

namespace jnsc::effects {

/**
 * @brief Chorus audio effect.
 *
 * Classic chorus effect using a modulated delay line with feedback.
 * Creates sweeping comb-filter effects by mixing the input signal with a
 * time-varying delayed copy of itself.
 */
template <typename T>
class Chorus {
    /** @brief Tunable constants
     * @param NUM_VOICES Number of delay taps (voices) for the chorus effect
     * @param NORM_FACTOR Normalization factor = 1/sqrt(NUM_VOICES)
     * @param MAX_MODULATION_MS Maximum modulation depth in milliseconds
     * @param MAX_FEEDBACK Maximum feedback amount (0.0 - 1.0)
     * @param SMOOTHING_TIME_MS Smoothing time for parameter changes in milliseconds
     * @param MAX_DELAY_MS Maximum delay time in milliseconds
     */
    static constexpr size_t NUM_VOICES = 2;
    static constexpr T NORM_FACTOR = T(0.5); // NOTE: remember to update if NUM_VOICES changes!
    static constexpr T MAX_MODULATION_MS = T(10.0);
    static constexpr T MAX_FEEDBACK = T(0.9);
    static constexpr int SMOOTHING_TIME_MS = 50;
    static constexpr T MAX_DELAY_MS = T(50.0);

  public:
    /// Default constructor.
    Chorus() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     */
    Chorus(size_t newNumChannels, T newSampleRate) { prepare(newNumChannels, newSampleRate); }

    /// Default destructor.
    ~Chorus() = default;

    /// No copy nor move semantics
    Chorus(const Chorus&) = delete;
    Chorus& operator=(const Chorus&) = delete;
    Chorus(Chorus&&) = delete;
    Chorus& operator=(Chorus&&) = delete;

    /**
     * @brief Prepare the chorus for processing.
     * @param newNumChannels Number of channels.
     * @param newSampleRate Sample rate in Hz.
     */
    void prepare(size_t newNumChannels, T newSampleRate) {
        // Store global parameters
        numChannels = utils::detail::clampChannels(newNumChannels);
        sampleRate = utils::detail::clampSampleRate(newSampleRate);

        // Prepare DSP components
        multiTapDelay.prepare(numChannels, sampleRate, Time<T>::Milliseconds(MAX_DELAY_MS));
        multiTapDelay.setControlSmoothingTime(Time<T>::Milliseconds(T(SMOOTHING_TIME_MS)));
        lfo.prepare(numChannels * NUM_VOICES, sampleRate);
        lfo.setWaveform(Waveform::Sine);

        // Prepare parameters
        modDepthSamples.prepare(numChannels, sampleRate);
        lfoPhaseOffset.prepare(numChannels * NUM_VOICES, sampleRate);
        feedback.prepare(numChannels, sampleRate);

        // Set voice gains
        Gain<T> voiceGain = Gain<T>::Linear(NORM_FACTOR);
        for (size_t tap = 0; tap < NUM_VOICES; ++tap)
            multiTapDelay.setTapGain(tap, voiceGain);

        // Set Default Parameters
        setRate(T(0.5), true);
        setDepth(T(0.5), true);
        setFeedback(T(0.0), true);
        setDelayMs(T(15.0), true);
        setSpread(T(1.0), true);
    }

    /**
     * @brief Reset the effect state and clear buffers.
     */
    void reset() { multiTapDelay.reset(); }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input sample pointers (one per channel)
     * @param output Output sample pointers (one per channel) - wet signal only
     * @param numSamples Number of samples to process
     *
     * @note Input and output must have the same number of channels as prepared.
     *       Processing is done sample-by-sample due to the feedback loop.
     */
    void processBlock(const T* const* input, T* const* output, size_t numSamples) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            // Calculate the base LFO index for this channel
            size_t voiceBaseIdx = ch * NUM_VOICES;
            for (size_t n = 0; n < numSamples; ++n) {
                // Get current input sample
                T inputSample = input[ch][n];

                // Initialize output to zero before accumulating
                T outputSample = T(0);

                // Get the modulation depth for this channel
                T modDepth = modDepthSamples.getNextValue(ch);

                // Loop over active voices
                for (size_t tap = 0; tap < NUM_VOICES; ++tap) {
                    // Get the phase offset for this channel-tap combination
                    T phaseOffset = lfoPhaseOffset.getNextValue(index(ch, tap));

                    // Run LFO for this channel-tap combination to get modulation value (unipolar: 0 to +1)
                    T lfoValue = lfo.processSample(voiceBaseIdx + tap, phaseOffset) * T(0.5) + T(0.5);
                    T mod = modDepth * lfoValue;

                    // Read delayed sample with interpolation and gain for this tap and accumulate to output
                    outputSample += multiTapDelay.readSample(ch, tap, mod);
                }

                // Write the input sample plus feedback into the delay line
                multiTapDelay.writeSample(ch, inputSample + feedback.getNextValue(ch) * outputSample * NORM_FACTOR);
                // Store the final output sample
                output[ch][n] = outputSample;
            }
        }
    }

    /**
     * @brief Set the LFO rate (modulation speed).
     * @param rateHz Rate in Hz (typical range: 0.1 - 5 Hz)
     */
    void setRate(T rateHz, bool skipSmoothing = false) { lfo.setFrequency(Frequency<T>::Hertz(rateHz), skipSmoothing); }

    /**
     * @brief Set the modulation depth.
     * @param depth Normalized depth amount 0.0 - 1.0
     */
    void setDepth(T depth, bool skipSmoothing = false) {
        if (sampleRate <= T(0))
            return; // Avoid division by zero
        T mds = Time<T>::Milliseconds(depth * MAX_MODULATION_MS).toSamples(sampleRate);
        modDepthSamples.setTarget(mds, skipSmoothing);
    }

    /**
     * @brief Set the feedback amount.
     * @param feedbackAmount Normalized feedback amount 0.0 - 1.0
     */
    void setFeedback(T feedbackAmount, bool skipSmoothing = false) {
        feedback.setTarget(feedbackAmount * MAX_FEEDBACK, skipSmoothing);
    }

    /**
     * @brief Set the center delay time.
     * @param delayMs Center delay in milliseconds (typical range: 10 - 30 ms)
     *                The LFO will modulate around this center point.
     */
    void setDelayMs(T delayMs, bool skipSmoothing = false) {
        multiTapDelay.setDelay(Time<T>::Milliseconds(delayMs), skipSmoothing);
    }

    /**
     * @brief Set the channel spread (phase offset between channels).
     * @param spread Spread amount 0.0 - 1.0 (0 = all channels in phase, 1 = maximum phase offset)
     */
    void setSpread(T spread, bool skipSmoothing = false) {
        // Distribute voices
        T voiceSpread = T(0.5) / static_cast<T>(NUM_VOICES);

        // Spread param offsets channels further apart
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t tap = 0; tap < NUM_VOICES; ++tap) {
                T offset = (spread * ch) / static_cast<T>(numChannels) + (tap * voiceSpread);
                lfoPhaseOffset.setTarget(index(ch, tap), offset, skipSmoothing);
            }
        }
    }

    /// Get number of channels
    size_t getNumChannels() const { return numChannels; }
    /// Get sample rate
    T getSampleRate() const { return sampleRate; }

  private:
    // Config variables
    size_t numChannels = 0;
    T sampleRate = T(44100);

    // Processors
    MultiTapDelayLine<T, NUM_VOICES> multiTapDelay;
    Oscillator<T> lfo;

    // Parameters
    DspParam<T> modDepthSamples;
    DspParam<T> lfoPhaseOffset;
    DspParam<T> feedback;
    T normFactor;

    // Helper function for indexing
    inline size_t index(size_t ch, size_t tap) { return ch * NUM_VOICES + tap; }
};

} // namespace jnsc::effects
