// Jonssonic - A C++ audio DSP library
// Damping filter class header file
// SPDX-License-Identifier: MIT

#pragma once
#include <cmath>
#include <algorithm>
#include "FilterTypes.h"

namespace Jonssonic {

// =============================================================================
// Template Declaration
// =============================================================================
template<typename T, DampingType Type = DampingType::OnePole>
class DampingFilter;

// =============================================================================
// One-Pole Damping Filter Specialization
// =============================================================================

/**
 * @brief DampingFilter specialization for One-Pole type.
 *       Implements a one-pole lowpass filter parametrized by T60 at a frequency.
 */
template<typename T>
class DampingFilter<T, DampingType::OnePole> {
public:
    // Constructors and Destructor
    DampingFilter() = default;
    DampingFilter(size_t newNumChannels, T newSampleRate) {
        prepare(newNumChannels, newSampleRate);
    }

    /**
     * @brief Prepare the filter for processing.
     * @param newSampleRate Sample rate in Hz
     * @param newNumChannels Number of channels
     * @note Must be called before processing.
     */
    void prepare(size_t newNumChannels, T newSampleRate) {
        numChannels = newNumChannels;
        sampleRate = newSampleRate;
        z1.assign(numChannels, T(0));
        a.assign(numChannels, T(0));
        b.assign(numChannels, T(0));
    }

    /**
     * @brief Set filter coefficients based on T60 at DC and Nyquist.
     * @param T60_DC Desired decay time (seconds) at DC (0 Hz)
     * @param T60_NYQ Desired decay time (seconds) at Nyquist (fs/2 Hz)
     * @note Must call prepare() before this.
     */
    void setByT60(size_t ch, T T60_DC, T T60_NYQ, size_t delaySamples) {
        T g0 = std::pow(T(10), -3.0 * delaySamples / (T60_DC * sampleRate));
        T g1 = std::pow(T(10), -3.0 * delaySamples / (T60_NYQ * sampleRate));
        a[ch] = (g0 + g1) / 2;
        b[ch] = (g0 - g1) / 2;
    }

    /**
     * @brief Process a single sample through the filter for a given channel.
     * @param ch Channel index
     * @param x Input sample
     * @return Filtered output sample
     */
    T processSample(size_t ch, T x) {
        assert(ch < numChannels && "Channel index out of bounds");
        T y = a[ch] * x + b[ch] * z1[ch];
        z1[ch] = y;
        return y;
    }

    void reset() { std::fill(z1.begin(), z1.end(), T(0)); }

private:
    T sampleRate = T(44100);
    size_t numChannels = 0;
    std::vector<T> a; // feedforward coefficient
    std::vector<T> b;  // feedback coefficient
    std::vector<T> z1;   // state per channel
};

// Future specializations (e.g., Shelving) can be added here

} // namespace Jonssonic
