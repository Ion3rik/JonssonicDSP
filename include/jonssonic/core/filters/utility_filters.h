// Jonssonic – A Modular Realtime C++ Audio DSP Library
// UtilityFilters: Common utility filter wrappers and aliases
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/utils/detail/config_utils.h"
#include <cmath>
#include <vector>

namespace jnsc {

/**
 * @brief DC Blocker - A first-order highpass filter with very low cutoff
 * @param T Sample data type (e.g., float, double)
 */

template <typename T>
class DCBlocker {
  public:
    /// Default constructor
    DCBlocker() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     */
    DCBlocker(size_t newNumChannels, T newSampleRate) { prepare(newNumChannels, newSampleRate); }

    /// Default destructor
    ~DCBlocker() = default;

    /// No copy or move semantics
    DCBlocker(const DCBlocker&) = delete;
    DCBlocker& operator=(const DCBlocker&) = delete;
    DCBlocker(DCBlocker&&) = delete;
    DCBlocker& operator=(DCBlocker&&) = delete;

    /**
     * @brief Prepare the DC blocker for processing.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     */
    void prepare(size_t newNumChannels, T newSampleRate) {
        numChannels = newNumChannels;
        sampleRate = newSampleRate;
        // Hardcoded cutoff frequency for DC blocking
        constexpr T cutoffHz = T(10);
        if (sampleRate > T(0)) {
            R = T(1) - (T(2) * T(M_PI) * cutoffHz / sampleRate);
            if (R < T(0))
                R = T(0);
            if (R > T(0.9999))
                R = T(0.9999);
        }
        x1.assign(numChannels, T(0));
        y1.assign(numChannels, T(0));
    }

    /// Reset the DC blocker state
    void reset() {
        std::fill(x1.begin(), x1.end(), T(0));
        std::fill(y1.begin(), y1.end(), T(0));
    }

    /**
     * @brief Process a single sample for a given channel.
     * @param ch Channel index
     * @param input Input sample
     * @return Filtered output sample
     * @note Must call @ref prepare before processing.
     */
    T processSample(size_t ch, T input) {
        if (ch >= numChannels)
            return input;
        T y = input - x1[ch] + R * y1[ch];
        x1[ch] = input;
        y1[ch] = y;
        return y;
    }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input sample pointers (one per channel)
     * @param output Output sample pointers (one per channel)
     * @param numSamples Number of samples to process
     * @note Must call @ref prepare before processing.
     */
    void processBlock(const T* const* input, T* const* output, size_t numSamples) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t n = 0; n < numSamples; ++n) {
                output[ch][n] = processSample(ch, input[ch][n]);
            }
        }
    }

  private:
    size_t numChannels = 0;
    T sampleRate = T(44100);
    T R = T(0.995);        // Pole radius
    std::vector<T> x1, y1; // State variables
};

} // namespace jnsc
