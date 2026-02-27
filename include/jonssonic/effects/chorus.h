// Jonssonic - A C++ audio DSP library
// Chorus effect header file
// SPDX-License-Identifier: MIT

#pragma once

#include "jonssonic/utils/detail/config_utils.h"
#include <algorithm>
#include <jonssonic/core/common/audio_buffer.h>
#include <jonssonic/core/common/dsp_param.h>
#include <jonssonic/core/generators/oscillator.h>
#include <jonssonic/models/delays/modulated_delay_stage.h>
#include <jonssonic/utils/math_utils.h>
#include <vector>

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
     * @param MAX_MODULATION_MS Maximum modulation depth in milliseconds
     * @param SMOOTHING_TIME_MS Smoothing time for parameter changes in milliseconds
     * @param MAX_DELAY_MS Maximum delay time in milliseconds
     * @param MAX_FEEDBACK Maximum absolute feedback amount to avoid instability
     * @param INTERNAL_WET_DRY_MIX Internal wet/dry mix for the delay stage (since chorus typically has a strong wet
     * signal)
     */
    static constexpr T MAX_MODULATION_MS = T(8.0);
    static constexpr int SMOOTHING_TIME_MS = 100;
    static constexpr T MAX_DELAY_MS = T(30.0);
    static constexpr T MAX_FEEDBACK = T(0.8);
    static constexpr T INTERNAL_WET_DRY_MIX = T(0.35);

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
     * @param newMaxBufferSize Maximum buffer size in samples.
     * @param newSampleRate Sample rate in Hz.
     */
    void prepare(size_t newNumChannels, T newSampleRate) {
        // Store global parameters
        numChannels = utils::detail::clampChannels(newNumChannels);
        sampleRate = utils::detail::clampSampleRate(newSampleRate);

        // Prepare DSP components
        delayStage.prepare(numChannels, Time<T>::Milliseconds(MAX_DELAY_MS), sampleRate);
        delayStage.setControlSmoothingTime(Time<T>::Milliseconds(T(SMOOTHING_TIME_MS)));
        delayStage.setFeedforward(T(1) - INTERNAL_WET_DRY_MIX, true); // set internal dry path gain

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
    void reset() { delayStage.reset(); }

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
        delayStage.processBlock(input, output, numSamples);
    }

    /**
     * @brief Set the LFO rate (modulation speed).
     * @param rateHz Rate in Hz (typical range: 0.1 - 5 Hz for flanging)
     */
    void setRate(T rateHz, bool skipSmoothing = false) {
        delayStage.setLfoFrequency(Frequency<T>::Hertz(rateHz), skipSmoothing);
    }

    /**
     * @brief Set the modulation depth.
     * @param depth Normalized depth amount 0.0 - 1.0
     */
    void setDepth(T depth, bool skipSmoothing = false) {
        delayStage.setModulationDepth(Time<T>::Milliseconds(depth * MAX_MODULATION_MS), skipSmoothing);
    }

    /**
     * @brief Set the feedback amount.
     * @param feedbackAmount Feedback amount -1.0 to 1.0
     * @note Mapped to (-MAX_FEEDBACK, MAX_FEEDBACK) to avoid instability.
     */
    void setFeedback(T feedbackAmount, bool skipSmoothing = false) {
        T scaledFeedback = std::clamp(feedbackAmount, T(-1.0), T(1.0)) * MAX_FEEDBACK;
        delayStage.setFeedback(scaledFeedback, skipSmoothing);
    }

    /**
     * @brief Set the center delay time.
     * @param delayMs Center delay in milliseconds (typical range: 1 - 7 ms)
     *                The LFO will modulate around this center point.
     */
    void setDelayMs(T delayMs, bool skipSmoothing = false) {
        delayStage.setDelay(Time<T>::Milliseconds(delayMs), skipSmoothing);
    }

    /**
     * @brief Set the channel spread (phase offset between channels).
     * @param spread Spread amount 0.0 - 1.0
     *                     0.0 = mono (all channels in phase)
     *                     1.0 = maximum spread (phase distributed across channels)
     */
    void setSpread(T spread, bool skipSmoothing = false) {
        // Update phase offsets based on spread (normalized to 0-1)
        for (size_t ch = 0; ch < numChannels; ++ch) {
            delayStage.setLfoPhaseOffset(ch, (spread * ch) / T(numChannels), skipSmoothing);
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
    models::ModulatedDelayStage<T, LagrangeInterpolator<T>, true, false, false> delayStage;
};

} // namespace jnsc::effects
