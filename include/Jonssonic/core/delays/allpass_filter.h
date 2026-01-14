// Jonssonic - A C++ audio DSP library
// Allpass filter class header file
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/utils/detail/config_utils.h"
#include <jonssonic/core/common/dsp_param.h>
#include <jonssonic/core/common/modulation.h>
#include <jonssonic/core/common/quantities.h>
#include <jonssonic/core/delays/delay_line.h>

namespace jnsc {
// MODULATION STRUCTURES
namespace AllpassMod {
/**
 * @brief Modulation input structure for AllpassFilter single sample processing
 * @param T Sample data type (e.g., float, double)
 */
template <typename T>
struct Sample : public ModulationInput<T> {
    T delayMod = T(0);
    T gainMod = T(1);
};

/**
 * @brief Modulation input structure for AllpassFilter block processing
 * @param T Sample data type (e.g., float, double)
 */
template <typename T>
struct Block : public ModulationInput<T> {
    const T* const* delayMod = nullptr; // modulation buffer for delay time (in samples)
    const T* const* gainMod = nullptr;  // modulation buffer for gain
};
} // namespace AllpassMod

/**
 * @brief Allpass Filter
 * @param T Sample data type (e.g., float, double)
 * @param Interpolator Interpolator type for fractional delay support (default: LinearInterpolator)
 */
template <typename T, typename Interpolator = LinearInterpolator<T>>
class AllpassFilter {
  public:
    /// Default constructor
    AllpassFilter() = default;
    /**
     * Parmetrized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     * @param newMaxDelay Maximum delay time in various units.
     */
    AllpassFilter(size_t newNumChannels, T newSampleRate, Time<T> newMaxDelay) {
        prepare(newNumChannels, newSampleRate, newMaxDelay);
    }

    /// Default destructor
    ~AllpassFilter() = default;

    /// No copy semantics nor move semantics
    AllpassFilter(const AllpassFilter&) = delete;
    const AllpassFilter& operator=(const AllpassFilter&) = delete;
    AllpassFilter(AllpassFilter&&) = delete;
    const AllpassFilter& operator=(AllpassFilter&&) = delete;

    /**
     * @brief Prepare the allpass filter for processing.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     * @param newMaxDelayMs Maximum delay time in milliseconds
     */
    void prepare(size_t newNumChannels, T newSampleRate, Time<T> newMaxDelay) {
        // Clamp and store number of channels and sample rate
        numChannels = utils::detail::clampChannels(newNumChannels);
        sampleRate = utils::detail::clampSampleRate(newSampleRate);

        // Prepare internal delay line and gain parameter
        delayLine.prepare(numChannels, sampleRate, newMaxDelay);
        gain.prepare(numChannels, sampleRate);
        gain.setBounds(T(-1), T(1));
        gain.setTarget(T(0), true);
    }

    /// Reset the allpass filter state
    void reset() {
        delayLine.reset();
        gain.reset();
    }

    /**
     * @brief Process a single sample for a specific channel.
     * @param ch Channel index
     * @param input Input sample
     * @return Output sample
     */
    T processSample(size_t ch, T input) {
        // Get smoothed gain value
        T gainValue = gain.getNextValue(ch);

        // Read delayed feedback state
        T delayed = delayLine.readSample(ch);

        // Compute output
        T output = gainValue * input + delayed;

        // Compute and write new feedback state
        T newFeedback = input - gainValue * output;
        delayLine.writeSample(ch, newFeedback);

        return output;
    }

    /**
     * @brief Process a single sample for a specific channel, with modulation.
     * @param ch Channel index
     * @param input Input sample
     * @param modStruct Modulation structure containing modulation data
     * @return Output sample
     */
    T processSample(size_t ch, T input, AllpassMod::Sample<T>& modStruct) {
        // Apply modulation to feedback and feedforward gains
        T modulatedGain = gain.applyMultiplicativeMod(ch, modStruct.gainMod);

        // Read delayed feedback state with modulation
        T delayed = delayLine.readSample(ch, modStruct.delayMod);

        // Compute output
        T output = modulatedGain * input + delayed;

        // Compute and write new feedback state
        T newFeedback = input - modulatedGain * output;
        delayLine.writeSample(ch, newFeedback);

        return output;
    }

    /**
     * @brief Process a block of samples for all channels (no modulation).
     * @param input Input sample pointers (one per channel)
     * @param output Output sample pointers (one per channel)
     * @param numSamples Number of samples to process
     * @note Input and output must have the same number of channels as prepared.
     */
    void processBlock(const T* const* input, T* const* output, size_t numSamples) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t i = 0; i < numSamples; ++i) {
                // Get smoothed gain value
                T gainValue = gain.getNextValue(ch);

                // Read delayed feedback state
                T delayed = delayLine.readSample(ch);

                // Compute output
                output[ch][i] = gainValue * input[ch][i] + delayed;

                // Compute and write new feedback state
                T newFeedback = input[ch][i] - gainValue * output[ch][i];
                delayLine.writeSample(ch, newFeedback);
            }
        }
    }

    /**
     * @brief Process a block of samples for all channels, with modulation.
     * @param input Input sample pointers (one per channel)
     * @param output Output sample pointers (one per channel)
     * @param modStruct Modulation structure containing modulation buffers
     * @param numSamples Number of samples to process
     * @note Input and output must have the same number of channels as prepared.
     */
    void processBlock(const T* const* input,
                      T* const* output,
                      AllpassMod::Block<T>& modStruct,
                      size_t numSamples) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t i = 0; i < numSamples; ++i) {
                // Apply modulation to feedback and feedforward gains
                T modulatedGain = gain.applyMultiplicativeMod(ch, modStruct.gainMod[ch][i]);

                // Read delayed feedback state with modulation
                T delayed = delayLine.readSample(ch, modStruct.delayMod[ch][i]);

                // Compute output
                output[ch][i] = modulatedGain * input[ch][i] + delayed;

                // Compute and write new feedback state
                T newFeedback = input[ch][i] - modulatedGain * output[ch][i];
                delayLine.writeSample(ch, newFeedback);
            }
        }
    }

    /**
     * @brief Set the parameter smoothing time in various units.
     * @param time Smoothing time struct.
     */
    void setControlSmoothingTime(Time<T> time) {
        gain.setSmoothingTime(time);
        delayLine.setControlSmoothingTime(time);
    }

    /**
     * @brief Set the delay time in various units for all channels.
     * @param newDelay Delay time struct.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setDelay(Time<T> newDelay, bool skipSmoothing = false) {
        delayLine.setDelay(newDelay, skipSmoothing);
    }

    /**
     * @brief Set the delay time in various units for a specific channel.
     * @param ch Channel index
     * @param newDelay Delay time struct.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setDelay(size_t ch, Time<T> newDelay, bool skipSmoothing = false) {
        delayLine.setDelay(ch, newDelay, skipSmoothing);
    }

    /**
     * @brief Set the gain for all channels.
     * @param newGain Gain value
     */
    void setGain(Gain<T> newGain, bool skipSmoothing = false) {
        gain.setTarget(newGain.toLinear(), skipSmoothing);
    }
    /**
     * @brief Set the gain for a specific channel.
     * @param ch Channel index
     * @param newGain Gain value
     */
    void setGain(size_t ch, Gain<T> newGain, bool skipSmoothing = false) {
        gain.setTarget(ch, newGain.toLinear(), skipSmoothing);
    }

  private:
    // Config variables
    size_t numChannels = 0;
    T sampleRate = T(44100);

    // DSP Components
    DelayLine<T, Interpolator> delayLine;
    DspParam<T> gain;
};
} // namespace jnsc
