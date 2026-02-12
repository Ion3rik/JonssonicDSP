// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// One-pole decay filter class header file
// SPDX-License-Identifier: MIT

#pragma once

#include "jonssonic/models/reverb/detail/decay_limits.h"

namespace jnsc::models::detail {

/**
 * @brief OnePoleDecay implements a simple one-pole decay filter that is parametrized via T60 decay times at DC and
 * Nyquist.
 * @tparam T Sample data type (e.g., float, double)
 */
template <typename T>
class OnePoleDecay {
  public:
    /// Default constructor.
    OnePoleDecay() = default;
    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels.
     * @param newSampleRate Sample rate in Hz.
     */
    OnePoleDecay(size_t newNumChannels, T newSampleRate) { prepare(newNumChannels, newSampleRate); }

    /// Default destructor.
    ~OnePoleDecay() = default;

    /// Reset the filter state
    void reset() { std::fill(z1.begin(), z1.end(), T(0)); }

    /**
     * @brief Prepare the filter for processing.
     * @param newSampleRate Sample rate in Hz.
     * @param newNumChannels Number of channels.
     */
    void prepare(size_t newNumChannels, T newSampleRate) {
        // Store config variables
        numChannels = utils::detail::clampChannels(newNumChannels);
        sampleRate = utils::detail::clampSampleRate(newSampleRate);

        // Initialize state and coefficient vectors
        z1.assign(numChannels, T(0));
        a.assign(numChannels, T(0));
        b.assign(numChannels, T(0));

        // Mark as prepared
        togglePrepared = true;
    }

    /**
     * @brief Set filter coefficients based on T60 at DC and Nyquist.
     * @param newT60Dc Desired decay time at DC (0 Hz)
     * @param newT60Nyq Desired decay time at Nyquist (fs/2 Hz)
     * @param newDelayTime Delay time used in the damping calculation
     * @note Coeffs are only updated if @ref prepare has been called before.
     */
    void setDecayTimes(size_t ch, Time<T> newT60Dc, Time<T> newT60Nyq, Time<T> newDelayTime) {
        assert(ch < numChannels && "Channel index out of bounds.");
        assert(sampleRate > T(0) && "Sample rate must be set and greater than zero.");

        // Early exit if not prepared
        if (!togglePrepared)
            return;

        // Clamp T60 values
        T T60_DC = DecayLimits<T>::clampTime(newT60Dc, sampleRate).toSeconds(sampleRate);
        T T60_NYQ = DecayLimits<T>::clampTime(newT60Nyq, sampleRate).toSeconds(sampleRate);

        // Compute coefficients based on T60 values
        T g0 = std::pow(T(10), -3.0 * newDelayTime.toSeconds(sampleRate) / T60_DC);
        T g1 = std::pow(T(10), -3.0 * newDelayTime.toSeconds(sampleRate) / T60_NYQ);
        a[ch] = (g0 + g1) / 2;
        b[ch] = (g0 - g1) / 2;
    }

    /**
     * @brief Process a single sample through the filter for a given channel.
     * @param ch Channel index
     * @param x Input sample
     * @return Filtered output sample
     * @note Must call @ref prepare before processing.
     */
    T processSample(size_t ch, T x) {
        T y = a[ch] * x + b[ch] * z1[ch];
        z1[ch] = y;
        return y;
    }

    /// Get number of prepared channels
    size_t getNumChannels() const { return numChannels; }

    /// Check if the filter is prepared
    bool isPrepared() const { return togglePrepared; }

  private:
    bool togglePrepared = false;
    T sampleRate = T(44100);
    size_t numChannels = 0;
    std::vector<T> a;  // feedforward coefficient
    std::vector<T> b;  // feedback coefficient
    std::vector<T> z1; // state per channel
};

} // namespace jnsc::models::detail