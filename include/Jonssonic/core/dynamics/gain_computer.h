// Jonssonic - A Modular Realtime C++ Audio DSP Library
// GainComputer class header file
// SPDX-License-Identifier: MIT

#pragma once
#include "detail/gain_computer_policies.h"
#include "jonssonic/utils/detail/config_utils.h"
#include <cmath>
#include <jonssonic/core/common/dsp_param.h>
#include <jonssonic/core/common/quantities.h>
#include <jonssonic/utils/math_utils.h>
#include <limits>

namespace jonssonic::core::dynamics {

// =============================================================================
// Template Declaration
// =============================================================================
/**
 * @brief GainComputer class template.
 *        Computes gain based on input signal level and specified dynamics policy.
 * @tparam T Sample type
 * @tparam Policy Dynamics policy. (Default: CompressorPolicy<T>, see note)
 * @note Current policies include CompressorPolicy, ExpanderDownPolicy, ExpanderUpPolicy,
 * LimiterPolicy, and GatePolicy. Policy classes are defined in @ref
 * detail/gain_computer_policies.h.
 */
template <typename T, typename Policy = CompressorPolicy<T>>
class GainComputer {
  public:
    /// Default constructor.
    GainComputer() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     */
    GainComputer(size_t newNumChannels, T newSampleRate) { prepare(newNumChannels, newSampleRate); }

    /// Default destructor.
    ~GainComputer() = default;

    /// No copy nor move semantics
    GainComputer(const GainComputer &) = delete;
    GainComputer &operator=(const GainComputer &) = delete;
    GainComputer(GainComputer &&) = delete;
    GainComputer &operator=(GainComputer &&) = delete;

    /**
     * @brief Prepare the gain computer for processing.
     * @param numChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     */
    void prepare(size_t numNewChannels, T newSampleRate) {
        numChannels = utils::detail::clampChannels(numNewChannels);
        sampleRate = utils::detail::clampSampleRate(newSampleRate);
        policy.prepare(numChannels, sampleRate);
    }
    /**
     * @brief Process a single sample for a given channel.
     * @param ch Channel index
     * @param input Input signal sample
     * @return Computed gain in dB
     * @note Have to call @ref prepare() before processing.
     */
    T processSample(size_t ch, T input) { return policy.processSample(ch, input); }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input (signal) sample pointers (one per channel)
     * @param output Output (gain in linear) sample pointers (one per channel)
     * @param numSamples Number of samples to process
     * @note Have to call @ref prepare() before processing.
     */
    void processBlock(const T *const *input, T *const *output, size_t numSamples) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t n = 0; n < numSamples; ++n) {
                output[ch][n] = processSample(ch, input[ch][n]);
            }
        }
    }

    /**
     * @brief Set control parameter smoothing time.
     * @param time Smoothing time
     */
    void setControlSmoothingTime(Time<T> time) { policy.setControlSmoothingTime(time); }

    /**
     * @brief Set the threshold in dB.
     * @param newThresholdDb New threshold value in dB
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setThreshold(T newThresholdDb, bool skipSmoothing = false) {
        policy.setThreshold(newThresholdDb, skipSmoothing);
    }

    /**
     * @brief Set the compression ratio.
     * @param newRatio New ratio value
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setRatio(T newRatio, bool skipSmoothing = false) {
        policy.setRatio(newRatio, skipSmoothing);
    }
    /**
     * @brief Set the knee width in dB.
     * @param newKneeDb New knee width value in dB
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setKnee(T newKneeDb, bool skipSmoothing = false) {
        policy.setKnee(newKneeDb, skipSmoothing);
    }

    /// Get the number of channels.
    size_t getNumChannels() const { return numChannels; }

    /// Get the sample rate.
    T getSampleRate() const { return sampleRate; }

  private:
    size_t numChannels = 0;
    T sampleRate = T(44100);
    Policy policy; // Policy instance (e.g., CompressorPolicy<T>)
};

} // namespace jonssonic::core::dynamics