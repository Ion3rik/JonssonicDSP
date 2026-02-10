// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Filter parameter limits header file
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/utils/detail/config_utils.h"
#include <cstddef>
#include <jonssonic/core/common/quantities.h>

namespace jnsc::detail {
/**
 * @brief Struct defining limits for filter parameters and utility clamping functions.
 * @tparam T Sample data type (e.g., float, double)
 */
template <typename T>
struct FilterLimits {
    // Maximum number of sections
    static constexpr size_t MAX_SECTIONS = 64;

    // Gain limits
    static constexpr T MAX_GAIN_DB = T(20.0);
    static constexpr T MIN_GAIN_DB = T(-60.0);

    // Frequency limits
    static constexpr T MAX_FREQ_NORM = T(0.45);   // Normalized maximum frequency(Nyquist frequency is 0.5)
    static constexpr T MIN_FREQ_NORM = T(0.0001); // Avoid zero frequency which can cause issues.

    // Q factor limits
    static constexpr T MAX_Q = T(20.0);
    static constexpr T MIN_Q = T(0.1);

    // Clamping functions for parameters
    static size_t clampSections(size_t sections) { return std::clamp(sections, size_t(1), MAX_SECTIONS); }
    static Gain<T> clampGain(Gain<T> gain) {
        T clampedDbGain = std::clamp(gain.toDecibels(), MIN_GAIN_DB, MAX_GAIN_DB);
        return Gain<T>::Decibels(clampedDbGain);
    }
    static Frequency<T> clampFrequency(Frequency<T> freq) {
        T clampedFreq = std::clamp(freq.toNormalized(), MIN_FREQ_NORM, MAX_FREQ_NORM);
        return Frequency<T>::Normalized(clampedFreq);
    }
    static T clampQ(T q) { return std::clamp(q, MIN_Q, MAX_Q); }
};

} // namespace jnsc::detail