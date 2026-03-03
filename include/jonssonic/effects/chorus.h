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
     * @param NUM_TAPS Maximum number of delay taps (voices) for the chorus effect
     * @param MAX_MODULATION_MS Maximum modulation depth in milliseconds
     * @param SMOOTHING_TIME_MS Smoothing time for parameter changes in milliseconds
     * @param MAX_DELAY_MS Maximum delay time in milliseconds
     */
    static constexpr size_t MAX_NUM_TAPS = 8;
    static constexpr T MAX_MODULATION_MS = T(8.0);
    static constexpr int SMOOTHING_TIME_MS = 100;
    static constexpr T MAX_DELAY_MS = T(60.0);

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
        lfo.prepare(numChannels * MAX_NUM_TAPS, sampleRate);
        lfo.setWaveform(Waveform::Sine);

        // Prepare parameters
        modDepthSamples.prepare(numChannels, sampleRate);
        lfoPhaseOffset.prepare(numChannels * MAX_NUM_TAPS, sampleRate);

        // Set Default Parameters
        setRate(T(0.5), true);
        setDepth(T(0.5), true);
        setNumVoices(2, true);
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
            for (size_t n = 0; n < numSamples; ++n) {
                // Write input sample to delay line
                multiTapDelay.writeSample(ch, input[ch][n]);

                // Get the smoothed modulation depth in samples
                T modDepth = modDepthSamples.getNextValue(ch);

                // Loop over active voices
                for (size_t tap = 0; tap < numActiveVoices; ++tap) {
                    // Run LFO for this channel-tap combination to get modulation value
                    T mod = modDepth * lfo.processSample(ch * MAX_NUM_TAPS + tap);

                    // Read delayed sample with interpolation and gain for this tap
                    output[ch][n] = multiTapDelay.readSample(ch, tap, mod);
                }
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
        Time<T> depthMs = Time<T>::Milliseconds(depth * MAX_MODULATION_MS);
        modDepthSamples.setTarget(depthMs.toSamples(sampleRate), skipSmoothing);
    }

    /**
     * @brief Set the number of voices (taps) for the chorus effect.
     * @param numVoices Number of voices (1 to MAX_NUM_TAPS)
     */
    void setNumVoices(size_t numVoices, bool skipSmoothing = false) {
        // Clamp number of voices to valid range
        numActiveVoices = std::min(std::max(numVoices, size_t(1)), MAX_NUM_TAPS);

        // Update the tap gains based on number of active voices
        // (i.e. we activate the first numActiveVoices taps and set
        // their gains, while muting the rest)
        for (size_t tap = 0; tap < MAX_NUM_TAPS; ++tap) {
            T gain = static_cast<T>(tap < numActiveVoices) * T(1.0) / std::sqrt(T(numActiveVoices));
            multiTapDelay.setTapGain(tap, Gain<T>::Linear(gain), skipSmoothing);
        }

        // Update LFO phase offsets to reflect the new number of active voices
        updateLfoPhaseOffsets(skipSmoothing);
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
        // Store the new spread amount
        currentSpread = spread;

        // Update LFO phase offsets for all channel-tap combinations based on the new spread
        updateLfoPhaseOffsets(skipSmoothing);
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
    MultiTapDelayLine<T, MAX_NUM_TAPS> multiTapDelay;
    Oscillator<T> lfo;

    // Parameters
    DspParam<T> modDepthSamples;
    DspParam<T> lfoPhaseOffset;
    size_t numActiveVoices = 0;
    T currentSpread = T(0);

    void updateLfoPhaseOffsets(bool skipSmoothing = false) {

        // Distribute voice phases evenly
        T voicePhaseStep = T(1.0) / T(numActiveVoices);

        for (size_t ch = 0; ch < numChannels; ++ch) {
            // Channel-specific phase offset controlled by spread parameter
            T channelPhaseOffset = currentSpread * static_cast<T>(ch) / static_cast<T>(numChannels);

            for (size_t tap = 0; tap < MAX_NUM_TAPS; ++tap) {
                // Calculate the LFO index for this channel-tap combination
                size_t lfoIndex = ch * MAX_NUM_TAPS + tap;

                // Fixed voice phase (only for active taps)
                T voicePhase = (tap < numActiveVoices) ? (voicePhaseStep * static_cast<T>(tap)) : T(0);

                // Combine voice phase + channel offset
                T totalPhase = voicePhase + channelPhaseOffset;

                // Wrap to 0-1 range
                totalPhase = totalPhase - std::floor(totalPhase);

                // Set phase for this specific LFO
                lfoPhaseOffset.setTarget(lfoIndex, totalPhase, skipSmoothing);
            }
        }
    }
};

} // namespace jnsc::effects
