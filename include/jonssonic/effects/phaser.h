// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Phaser effect header file
// SPDX-License-Identifier: MIT

#pragma once

#include "jonssonic/utils/detail/config_utils.h"
#include <algorithm>
#include <cmath>
#include <jonssonic/core/common/dsp_param.h>
#include <jonssonic/core/filters/state_variable_filter.h>
#include <jonssonic/core/generators/oscillator.h>
#include <vector>

namespace jnsc::effects {

/**
 * @brief Phaser audio effect using a modulated TPT all-pass cascade.
 * Two second-order sections at Q = 0.5 provide four first-order phase shifts.
 * The cascade is blended equally with the input to create two sweeping notches.
 */
template <typename T>
class Phaser {
    /** @brief Tunable constants
     * @param NUM_STAGES Number of second-order all-pass sections
     * @param MAX_MODULATION_OCTAVES Maximum sweep above and below the center frequency
     * @param SMOOTHING_TIME_MS Smoothing time for parameter changes in milliseconds
     * @param MAX_FEEDBACK Maximum absolute feedback amount
     */
    static constexpr size_t NUM_STAGES = 2;
    static constexpr T MAX_MODULATION_OCTAVES = T(3);
    static constexpr int SMOOTHING_TIME_MS = 100;
    static constexpr T MAX_FEEDBACK = T(0.9);

  public:
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
     */
    void prepare(size_t newNumChannels, T newSampleRate) {
        // Store global parameters
        numChannels = utils::detail::clampChannels(newNumChannels);
        sampleRate = utils::detail::clampSampleRate(newSampleRate);

        // Prepare DSP components
        allPassStage.prepare(numChannels, sampleRate, NUM_STAGES);
        lfo.prepare(numChannels, sampleRate);
        lfo.setWaveform(Waveform::Triangle);
        feedbackSamples.assign(numChannels, T(0));

        // Prepare parameters with smoothing
        const auto smoothingTime = Time<T>::Milliseconds(T(SMOOTHING_TIME_MS));
        lfo.setControlSmoothingTime(smoothingTime);
        modDepthOctaves.prepare(numChannels, sampleRate);
        centerFrequencyHz.prepare(numChannels, sampleRate);
        lfoPhaseOffset.prepare(numChannels, sampleRate);
        feedback.prepare(numChannels, sampleRate);
        modDepthOctaves.setSmoothingTime(smoothingTime);
        centerFrequencyHz.setSmoothingTime(smoothingTime);
        lfoPhaseOffset.setSmoothingTime(smoothingTime);
        feedback.setSmoothingTime(smoothingTime);
        modDepthOctaves.setBounds(T(0), MAX_MODULATION_OCTAVES);
        centerFrequencyHz.setBounds(T(20), sampleRate * T(0.45));
        lfoPhaseOffset.setBounds(T(0), T(1));
        feedback.setBounds(-MAX_FEEDBACK, MAX_FEEDBACK);

        // Set default parameters
        setRate(T(0.5), true);
        setDepth(T(0.5), true);
        setFeedback(T(0.25), true);
        setCenterFrequency(T(1000), true);
        setSpread(T(0), true);
    }

    /// Reset filter memory, feedback history and LFO phase, preserving parameter targets.
    void reset() {
        allPassStage.reset();
        lfo.reset();
        std::fill(feedbackSamples.begin(), feedbackSamples.end(), T(0));
    }

    /**
     * @brief Process a block of samples for all channels. In-place processing is supported.
     * @param input Input sample pointers (one per channel).
     * @param output Output sample pointers (one per channel), containing the complete phaser blend.
     * @param numSamples Number of samples to process.
     * @note Channel count must match prepare. Feedback history persists between blocks.
     */
    void processBlock(const T* const* input, T* const* output, size_t numSamples) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t n = 0; n < numSamples; ++n) {
                // Sweep in octaves, smoothing controls before applying the LFO.
                T lfoValue = lfo.processSample(ch, lfoPhaseOffset.getNextValue(ch));
                T frequencyHz =
                    centerFrequencyHz.getNextValue(ch) * std::exp2(lfoValue * modDepthOctaves.getNextValue(ch));
                T drySample = input[ch][n];
                T feedbackAmount = feedback.getNextValue(ch);
                T inSample = drySample + feedbackAmount * feedbackSamples[ch];
                T outSample = allPassStage.processSample(ch, inSample, frequencyHz, T(0.5))[3]; // Allpass output
                feedbackSamples[ch] = outSample;

                // Blend the input and all-pass output for the classic phaser sound.
                output[ch][n] = T(0.5) * (drySample + outSample);
            }
        }
    }

    /**
     * @brief Set the LFO rate in Hz.
     * @param skipSmoothing If true, set immediately.
     */
    void setRate(T rateHz, bool skipSmoothing = false) { lfo.setFrequency(Frequency<T>::Hertz(rateHz), skipSmoothing); }

    /** @brief Set normalized modulation depth, 0.0 - 1.0 (up to +/- 3 octaves). */
    void setDepth(T depth, bool skipSmoothing = false) {
        modDepthOctaves.setTarget(std::clamp(depth, T(0), T(1)) * MAX_MODULATION_OCTAVES, skipSmoothing);
    }

    /** @brief Set feedback, -1.0 to 1.0, scaled to the stable feedback range. */
    void setFeedback(T feedbackAmount, bool skipSmoothing = false) {
        feedback.setTarget(std::clamp(feedbackAmount, T(-1), T(1)) * MAX_FEEDBACK, skipSmoothing);
    }

    /** @brief Set the center frequency of the sweep in Hz. */
    void setCenterFrequency(T centerFreqHz, bool skipSmoothing = false) {
        centerFrequencyHz.setTarget(centerFreqHz, skipSmoothing);
    }

    /**
     * @brief Set channel spread, 0.0 - 1.0.
     * At full spread, stereo channels have a 90-degree LFO phase difference.
     */
    void setSpread(T spread, bool skipSmoothing = false) {
        spread = std::clamp(spread, T(0), T(1));
        for (size_t ch = 0; ch < numChannels; ++ch)
            lfoPhaseOffset.setTarget(ch, (spread * T(0.5) * T(ch)) / T(numChannels), skipSmoothing);
    }

    /// Get number of channels.
    size_t getNumChannels() const { return numChannels; }
    /// Get sample rate.
    T getSampleRate() const { return sampleRate; }

  private:
    // Config variables
    size_t numChannels = 0;
    T sampleRate = T(44100);

    // Processors
    Series<StateVariableFilter<T>> allPassStage;
    Oscillator<T> lfo;
    std::vector<T> feedbackSamples;

    // Parameters with smoothing
    DspParam<T> modDepthOctaves;
    DspParam<T> centerFrequencyHz;
    DspParam<T> lfoPhaseOffset;
    DspParam<T> feedback;
};

} // namespace jnsc::effects
