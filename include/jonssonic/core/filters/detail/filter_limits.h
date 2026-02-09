// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Filter parameter limits header file
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/utils/detail/config_utils.h"
#include <cstddef>

namespace jnsc::detail {
/// General filter parameter limits
template <typename T>
struct FilterLimits {
    // Maximum number of biquad sections
    static constexpr size_t MAX_SECTIONS = 64;

    // Gain limits
    static constexpr T MAX_GAIN_LIN = T(10.0);  // +20 dB
    static constexpr T MIN_GAIN_LIN = T(0.001); // -60 dB

    // Frequency limits
    static constexpr T MAX_FREQ_NORM = T(0.5);  // Nyquist
    static constexpr T MIN_FREQ_NORM = T(1e-6); // Small positive value to avoid zero frequency

    // Clamping functions for parameters
    static T clampGain(T gain) { return std::clamp(gain, MIN_GAIN_LIN, MAX_GAIN_LIN); }
    static T clampFrequency(T freqNorm) { return std::clamp(freqNorm, MIN_FREQ_NORM, MAX_FREQ_NORM); }
    static size_t clampSections(size_t sections) { return std::clamp(sections, size_t(1), MAX_SECTIONS); }
};

/// Biquad filter parameter limits
template <typename T>
struct BiquadLimits {
    // Q factor limits
    static constexpr T MAX_Q = T(20);
    static constexpr T MIN_Q = T(0.1);
};

/// Damping filter parameter limits
template <typename T>
struct DampingLimits {
    // T60 limits in seconds
    static constexpr T MAX_T60_SEC = T(60.0);
    static constexpr T MIN_T60_SEC = T(0.001);
};

/// First-order filter parameter limits
template <typename T>
struct FirstOrderLimits {
    // No specific limits for first-order filters currently
};
} // namespace jnsc::detail