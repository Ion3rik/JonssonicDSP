// Jonssonic - A C++ audio DSP library
// Comb filter class header file
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/utils/detail/config_utils.h"
#include <jonssonic/core/common/dsp_param.h>
#include <jonssonic/core/common/modulation.h>
#include <jonssonic/core/common/quantities.h>
#include <jonssonic/core/delays/delay_line.h>

namespace jonssonic::core::delays {
// MODULATION STRUCTURES
namespace CombMod {
/**
 * @brief Modulation input structure for CombFilter single sample processing
 * @param T Sample data type (e.g., float, double)
 */
template <typename T> struct Sample : public common::ModulationInput<T> {
    T delayMod = T(0);
    T feedbackMod = T(1);
    T feedforwardMod = T(1);
};

/**
 * @brief Modulation input structure for CombFilter block processing
 * @param T Sample data type (e.g., float, double)
 */
template <typename T> struct Block : public common::ModulationInput<T> {
    const T *const *delayMod = nullptr;       // modulation buffer for delay time (in samples)
    const T *const *feedbackMod = nullptr;    // modulation buffer for feedback gain
    const T *const *feedforwardMod = nullptr; // modulation buffer for feedforward gain
};
} // namespace CombMod

/**
 * @brief General Comb Filter supporting feedback and feedforward
 * @param T Sample data type (e.g., float, double)
 * @param Interpolator Interpolator type for fractional delay support (default: LinearInterpolator)
 * @param SmootherType Type of smoother for delay time modulation (default: OnePole)
 * @param SmootherOrder Order of the smoother for delay time modulation (default: 1)
 * @param SmoothingTimeMs Smoothing time in milliseconds for delay time changes (default: 10ms)
 */
template <typename T,
          typename Interpolator = common::LinearInterpolator<T>,
          common::SmootherType SmootherType = common::SmootherType::OnePole,
          int SmootherOrder = 1>
class CombFilter {
    /// Type aliases for convenience, readability, and future-proofing
    using DelayLineType = DelayLine<T, Interpolator, SmootherType, SmootherOrder>;
    using DspParamType = common::DspParam<T, SmootherType, SmootherOrder>;

  public:
    /// Default constructor
    CombFilter() = default;
    /**
     * Parmetrized constructor that calls @ref prepare.
     */
    CombFilter(size_t newNumChannels, T newSampleRate, Time<T> newMaxDelay) {
        prepare(newNumChannels, newSampleRate, newMaxDelay);
    }
    /// Default destructor
    ~CombFilter() = default;

    /// No copy semantics nor move semantics
    CombFilter(const CombFilter &) = delete;
    const CombFilter &operator=(const CombFilter &) = delete;
    CombFilter(CombFilter &&) = delete;
    const CombFilter &operator=(CombFilter &&) = delete;

    /**
     * @brief Prepare the comb filter for processing.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     * @param newMaxDelay Maximum delay time in various units.
     */
    void prepare(size_t newNumChannels, T newSampleRate, Time<T> newMaxDelay) {
        // Clamp and store number of channels and sample rate
        numChannels = utils::detail::clampChannels(newNumChannels);
        sampleRate = utils::detail::clampSampleRate(newSampleRate);

        // Prepare internal delay line and parameters
        delayLine.prepare(numChannels, sampleRate, newMaxDelay);
        feedbackGain.prepare(numChannels, sampleRate);
        feedforwardGain.prepare(numChannels, sampleRate);
        feedbackGain.setBounds(T(-1), T(1));
        feedforwardGain.setBounds(T(-1), T(1));
        feedbackGain.setTarget(T(0), true);
        feedforwardGain.setTarget(T(0), true);
    }

    /// Reset the comb filter state
    void reset() {
        delayLine.reset();
        feedbackGain.reset();
        feedforwardGain.reset();
    }

    /**
     * @brief Process a single sample for a specific channel.
     * @param ch Channel index
     * @param input Input sample
     * @return Output sample
     */
    T processSample(size_t ch, T input) {
        // Get smoothed gain values
        T ffGain = feedforwardGain.getNextValue(ch);
        T fbGain = feedbackGain.getNextValue(ch);

        // Read delayed signal
        T delayed = delayLine.readSample(ch);

        // Compute what to write back (input + delayed feedback)
        T toWrite = input + delayed * fbGain;
        delayLine.writeSample(ch, toWrite);

        // Compute output (input + delayed feedforward)
        T output = input + delayed * ffGain;

        return output;
    }

    /**
     * @brief Process a single sample for a specific channel, with modulation.
     * @param ch Channel index
     * @param input Input sample
     * @param modStruct Modulation structure containing modulation data
     * @return Output sample
     */
    T processSample(size_t ch, T input, CombMod::Sample<T> &modStruct) {
        // Apply modulation to feedback and feedforward gains
        T modulatedFbGain = feedbackGain.applyMultiplicativeMod(ch, modStruct.feedbackMod);
        T modulatedFfGain = feedforwardGain.applyMultiplicativeMod(ch, modStruct.feedforwardMod);

        // Read delayed signal with modulation
        T delayed = delayLine.readSample(ch, modStruct.delayMod);

        // Compute what to write back (input + delayed feedback)
        T toWrite = input + delayed * modulatedFbGain;
        delayLine.writeSample(ch, toWrite);

        // Compute output (input + delayed feedforward)
        T output = input + delayed * modulatedFfGain;

        return output;
    }

    /**
     * @brief Process a block of samples for all channels (no modulation).
     * @param input Input sample pointers (one per channel)
     * @param output Output sample pointers (one per channel)
     * @param numSamples Number of samples to process
     * @note Input and output must have the same number of channels as prepared.
     */
    void processBlock(const T *const *input, T *const *output, size_t numSamples) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t i = 0; i < numSamples; ++i) {
                // Get smoothed gain values
                T ffGain = feedforwardGain.getNextValue(ch);
                T fbGain = feedbackGain.getNextValue(ch);

                // Read delayed signal
                T delayed = delayLine.readSample(ch);

                // Compute what to write back (input + delayed feedback)
                T toWrite = input[ch][i] + delayed * fbGain;
                delayLine.writeSample(ch, toWrite);

                // Compute output (input + delayed feedforward)
                output[ch][i] = input[ch][i] + delayed * ffGain;
            }
        }
    }

    /**
     * @brief Process a block of samples for all channels, with modulation.
     * @param input Input sample pointers (one per channel)
     * @param output Output sample pointers (one per channel)
     * @param modStruct Modulation structure containing modulation buffers
     * @param numSamples Number of samples to process
     *
     * @note Input and output must have the same number of channels as prepared.
     */
    void processBlock(const T *const *input,
                      T *const *output,
                      CombMod::Block<T> &modStruct,
                      size_t numSamples) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t i = 0; i < numSamples; ++i) {
                // Apply modulation to feedback and feedforward gains
                T modulatedFbGain =
                    feedbackGain.applyMultiplicativeMod(ch, modStruct.feedbackMod[ch][i]);
                T modulatedFfGain =
                    feedforwardGain.applyMultiplicativeMod(ch, modStruct.feedforwardMod[ch][i]);

                // Read delayed signal with modulation
                T delayed = delayLine.readSample(ch, modStruct.delayMod[ch][i]);

                // Compute what to write back (input + delayed feedback)
                T toWrite = input[ch][i] + delayed * modulatedFbGain;
                delayLine.writeSample(ch, toWrite);

                // Compute output (input + delayed feedforward)
                output[ch][i] = input[ch][i] + delayed * modulatedFfGain;
            }
        }
    }

    /**
     * @brief Set the control smoothing time in various units.
     * @param time Time value representing the smoothing time
     */
    void setControlSmoothingTime(Time<T> time) {
        feedbackGain.setSmoothingTime(time);
        feedforwardGain.setSmoothingTime(time);
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
     * @brief Set the feedback gain for all channels.
     * @param gain Gain struct.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setFeedbackGain(Gain<T> gain, bool skipSmoothing = false) {
        feedbackGain.setTarget(gain.toLinear(), skipSmoothing);
    }
    /**
     * @brief Set the feedback gain for a specific channel.
     * @param ch Channel index
     * @param gain Gain struct.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setFeedbackGain(size_t ch, Gain<T> gain, bool skipSmoothing = false) {
        feedbackGain.setTarget(ch, gain.toLinear(), skipSmoothing);
    }
    /**
     * @brief Set the feedforward gain for all channels.
     * @param gain Gain struct.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setFeedforwardGain(Gain<T> gain, bool skipSmoothing = false) {
        feedforwardGain.setTarget(gain.toLinear(), skipSmoothing);
    }
    /**
     * @brief Set the feedforward gain for a specific channel.
     * @param ch Channel index
     * @param gain Gain struct
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setFeedforwardGain(size_t ch, Gain<T> gain, bool skipSmoothing = false) {
        feedforwardGain.setTarget(ch, gain.toLinear(), skipSmoothing);
    }

  private:
    size_t numChannels = 0;
    T sampleRate = T(44100);
    DelayLineType delayLine;
    DspParamType feedbackGain;
    DspParamType feedforwardGain;
};
} // namespace jonssonic::core::delays
