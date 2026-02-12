// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Filter parameter limits header file
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/utils/detail/config_utils.h"
#include <jonssonic/core/common/quantities.h>

namespace jnsc::models::detail {
/**
 * @brief Struct defining limits for decay filter parameters,
 * @tparam T Sample data type (e.g., float, double)
 */
template <typename T>
struct DecayLimits {
    // T60 limits in seconds
    static constexpr T MIN_T60_S = 0.001; // 10 ms
    static constexpr T MAX_T60_S = 30.0;  // 30 seconds

    // Clamping function for T60 values
    static Time<T> clampTime(Time<T> time, T sampleRate) {
        T clampedTimeS = std::clamp(time.toSeconds(sampleRate), MIN_T60_S, MAX_T60_S);
        return Time<T>::Seconds(clampedTimeS);
    }
};

} // namespace jnsc::models::detail