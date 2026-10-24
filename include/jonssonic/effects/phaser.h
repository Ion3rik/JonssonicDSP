// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Phaser effect class header file
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/utils/detail/config_utils.h"
#include <jonssonic/core/common/dsp_param.h>
#include <jonssonic/core/filters/state_variable_filter.h>
#include <jonssonic/core/generators/oscillator.h>

namespace jnsc::effects {
/**
 * @brief Phaser audio effect.
 * Classic phaser effect using a modulated all-pass filter chain.
 * Creates sweeping notches in the frequency response by mixing the input signal with a
 * time-varying phase-shifted versions of itself.
 */
template <typename T>
class Phaser {
  public:
    /**
     * @brief Tunable constants
     * @param NUM_STAGES Number of all-pass filter stages
     * @param MAX_MODULATION_HZ Maximum modulation depth in Hz
     * @param MAX_FEEDBACK Maximum feedback amount (0.0 - 1.0)
     * @param SMOOTHING_TIME_MS Smoothing time for parameter changes in milliseconds
     *
     */
    /// Default constructor.
    Phaser() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     */
    Phaser(size_t newNumChannels, T newSampleRate) { prepare(newNumChannels, newSampleRate); }

    /// Default destructor.
    ~Phaser() = default;

    /// No copy nor move semantics
    Phaser(const Phaser&) = delete;
    Phaser& operator=(const Phaser&) = delete;
    Phaser(Phaser&&) = delete;
    Phaser& operator=(Phaser&&) = delete;

    /**
     * @brief Prepare the phaser for processing.
     * @param newNumChannels Number of channels.
     * @param newSampleRate Sample rate in Hz.
     *
     * This initializes internal buffers and sets default parameter values.
     * Must be called before processing audio.
     */
    void prepare(size_t newNumChannels, T newSampleRate);

    /**
     * @brief Reset the effect state and clear buffers.
     */
    void reset();

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input sample pointers (one per channel)
     * @param output Output sample pointers (one per channel) - wet signal only
     * @param numSamples Number of samples to process
     *
     * Note: Input and output must have the same number of channels as prepared.
     *       Processing is done sample-by-sample due to the feedback loop.
     */
    void processBlock(const T* const* input, T* const* output, size_t numSamples) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            T outSample = T(0);
            for (size_t i = 0; i < numSamples; ++i) {
                // Input plus feedback
                T inSample = input[ch][i] + feedback.getCurrentValue(ch) * outSample;

                // Get LFO modulation
                T lfoValue = lfo.processSample(ch, lfoPhaseOffset.getCurrentValue(ch));

                // Process through all-pass stage with LFO modulation
                outSample = allPassStage.processSample(ch, inSample);
                output[ch][i] = outSample;
            }
        }
    }

    /**
     * @brief Set the LFO rate in Hz.
     * @param rateHz LFO rate in Hz.
     * @param skipSmoothing If true, bypass parameter smoothing.
     */
    void setRate(T rateHz, bool skipSmoothing = false) { lfo.setFrequency(Frequency<T>::Hertz(rateHz), skipSmoothing); }

    /**
     * @brief Set the normalized modulation depth.
     * @param depthAmount Normalized depth amount (0.0 - 1.0)
     * @param skipSmoothing If true, bypass parameter smoothing.
     */
    void setDepth(T depthAmount, bool skipSmoothing = false) {
        modDepthHz.setTarget(depthAmount * MAX_MODULATION_HZ, skipSmoothing);
    }

    /**
     * @brief Set the feedback amount.
     * @param feedbackAmount Feedback amount (0.0 - 1.0)
     * @param skipSmoothing If true, bypass parameter smoothing.
     */
    void setFeedback(T feedbackAmount, bool skipSmoothing = false) {
        feedback.setTarget(feedbackAmount * MAX_FEEDBACK, skipSmoothing);
    }

    /**
     * @brief Set the center frequency of the notches.
     * @param centerFreqHz Center frequency in Hz (typical range: 500 - 2000 Hz)
     * @param skipSmoothing If true, bypass parameter smoothing.
     */
    void setCenterFrequency(T centerFreqHz, bool skipSmoothing = false) {
        allPassStage.setFrequency(Frequency<T>::Hertz(centerFreqHz), skipSmoothing);
    }

    /**
     * @brief Set the channel spread (phase offset between channels).
     * @param spread Spread amount 0.0 - 1.0 (0 = no spread, 1 = maximum spread)
     * @param skipSmoothing If true, bypass parameter smoothing.
     */
    void setSpread(T spread, bool skipSmoothing = false) {
        // Distribute phase offsets across channels based on spread amount
        T phaseShift = spread * T(0.5) * T(1) / T(numChannels);
        for (size_t ch = 0; ch < numChannels; ++ch)
            lfoPhaseOffset.setTarget(ch, phaseShift * T(ch), skipSmoothing);
    }

  private:
    // Config variables
    size_t numChannels = 0;
    T sampleRate = T(44100);

    // DSP components
    Series<StateVariableFilter<T>> allPassStage;
    Oscillator<T> lfo;

    // Parameters with smoothing
    DspParam<T> modDepthHz;
    DspParam<T> lfoPhaseOffset;
    DspParam<T> feedback;
};
} // namespace jnsc::effects