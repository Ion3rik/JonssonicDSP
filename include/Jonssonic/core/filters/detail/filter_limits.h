// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Filter parameter limits header file
// SPDX-License-Identifier: MIT

#pragma once
#include <cstddef>

namespace jonssonic::core::filters::detail
{   
/// General filter parameter limits
template<typename T>
struct FilterLimits
{
    // Maximum number of biquad sections
    static constexpr size_t MAX_SECTIONS = 64;

    // Maximum gains
    static constexpr T MAX_GAIN_DB = T(20);
    static constexpr T MAX_GAIN_LIN = T(10);

    // Minimum gains
    static constexpr T MIN_GAIN_DB = T(-60);
    static constexpr T MIN_GAIN_LIN = T(0.001);

    // Frequency limits
    static constexpr T MAX_FREQ_NORM = T(0.5); // Nyquist
    static constexpr T MIN_FREQ_NORM = T(1e-6); // Small positive value to avoid zero frequency
};

/// Biquad filter parameter limits
template<typename T>
struct BiquadLimits
{
    // Q factor limits
    static constexpr T MAX_Q = T(20);
    static constexpr T MIN_Q = T(0.1);
};

/// Damping filter parameter limits
template<typename T>
struct DampingLimits
{
    // T60 limits in seconds
    static constexpr T MAX_T60_SEC = T(60.0);
    static constexpr T MIN_T60_SEC = T(0.001);
};

/// First-order filter parameter limits
template<typename T>
struct FirstOrderLimits
{
    // No specific limits for first-order filters currently
};
} // namespace jonssonic::filters::detail