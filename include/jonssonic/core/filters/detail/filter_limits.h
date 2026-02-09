// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Filter parameter limits header file
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/utils/detail/config_utils.h"
#include <cstddef>

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
    static constexpr T MAX_GAIN_LIN = T(10.0);  // +20 dB
    static constexpr T MIN_GAIN_LIN = T(0.001); // -60 dB

    // Frequency limits
    static constexpr T MAX_FREQ_NORM = T(0.45); // Below Nyquist (0.5) to avoid unstable behavior
    static constexpr T MIN_FREQ_NORM = T(1e-6); // Small positive value to avoid unstable behavior at 0 Hz

    // Q factor limits
    static constexpr T MAX_Q = T(20.0);
    static constexpr T MIN_Q = T(0.1);

    // Clamping functions for parameters
    static size_t clampSections(size_t sections) { return std::clamp(sections, size_t(1), MAX_SECTIONS); }
    static T clampGain(T gain) { return std::clamp(gain, MIN_GAIN_LIN, MAX_GAIN_LIN); }
    static T clampFrequency(T freqNorm) { return std::clamp(freqNorm, MIN_FREQ_NORM, MAX_FREQ_NORM); }
    static T clampQ(T q) { return std::clamp(q, MIN_Q, MAX_Q); }
};

} // namespace jnsc::detail