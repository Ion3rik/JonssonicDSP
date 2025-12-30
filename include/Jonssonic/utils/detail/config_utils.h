// JonssonicDSP - Config utility functions
// SPDX-License-Identifier: MIT

#pragma once

#include <jonssonic/jonssonic_config.h>
#include <algorithm>
#include <cstddef>

namespace jonssonic::utils::detail {

/**
 * @brief Clamp a sample rate to the supported range.
 * @tparam T Floating-point type (float, double)
 * @param sr Sample rate to clamp
 * @return Clamped sample rate
 */
template <typename T>
constexpr T clampSampleRate(T sr) {
    return std::clamp(sr, static_cast<T>(JONSSONIC_MIN_SAMPLE_RATE), static_cast<T>(JONSSONIC_MAX_SAMPLE_RATE));
}

/**
 * @brief Clamp a channel count to the supported range.
 * @param ch Number of channels to clamp
 * @return Clamped channel count
 */
constexpr std::size_t clampChannels(std::size_t ch) {
    return std::clamp(ch, static_cast<std::size_t>(1), static_cast<std::size_t>(JONSSONIC_MAX_CHANNELS));
}

} // namespace jonssonic::utils::detail
