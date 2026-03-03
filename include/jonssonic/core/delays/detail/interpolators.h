// Jonssonic - A Modular Realtime C++ Audio DSP Library
// Interpolator structs for delay lines
// SPDX-License-Identifier: MIT

/**
 * @file interpolators.h
 * @brief This file contains various interpolation methods for delay lines.
 * @note Directly depends on CircularAudioBuffer for sample access and internal wrap-around handling.
 * @namespace jnsc::detail
 */

#pragma once
#include <jonssonic/core/common/circular_audio_buffer.h>

namespace jnsc::detail {
/**
 * @brief Nearest neighbor interpolator.
 * @tparam T The data type of the samples (e.g., float, double).
 */
template <typename T>
struct NearestInterpolator {
    /**
     * @brief Interpolate backward in time for delay lines using nearest neighbor method.
     * @param buffer Reference to the circular audio buffer.
     * @param ch Channel index
     * @param delaySamples The delay time in samples (can be fractional).
     */
    static T interpolate(CircularAudioBuffer<T>& buffer, size_t ch, T delaySamples) {
        // Calculate integer read index and fractional part
        size_t idx = static_cast<size_t>(std::floor(delaySamples));
        T frac = delaySamples - idx;

        // Nearest neighbor: choose the closest sample based on the fractional part
        return static_cast<T>(frac < 0.5f) * buffer.read(ch, idx) +
               static_cast<T>(frac >= 0.5f) * buffer.read(ch, idx - 1);
    }
};

/**
 * @brief Linear interpolator.
 * @tparam T The data type of the samples (e.g., float, double).
 */
template <typename T>
struct LinearInterpolator {
    /**
     * @brief Interpolate backward in time for delay lines using linear interpolation.
     * @param buffer Reference to the circular audio buffer.
     * @param ch Channel index
     * @param delaySamples The delay time in samples (can be fractional).
     */
    static T interpolate(CircularAudioBuffer<T>& buffer, size_t ch, T delaySamples) {
        // Calculate integer read index and fractional part
        size_t integerDelay = static_cast<size_t>(std::floor(delaySamples));
        T frac = delaySamples - integerDelay;
        // Linear interpolation between the two nearest samples
        return buffer.read(ch, integerDelay) * (1.0f - frac) + buffer.read(ch, integerDelay + 1) * frac;
    }
};

/**
 * @brief 4-point Lagrange interpolator (3rd order).
 * @tparam T The data type of the samples (e.g., float, double).
 */
template <typename T>
struct LagrangeInterpolator {
    /**
     * @brief Interpolate backward in time for delay lines using 4-point Lagrange interpolation.
     * @param buffer Reference to the circular audio buffer.
     * @param ch Channel index
     * @param delaySamples The delay time in samples (can be fractional).
     */
    static T interpolate(CircularAudioBuffer<T>& buffer, size_t ch, T delaySamples) {
        // Calculate integer read index and fractional part
        size_t integerDelay = static_cast<size_t>(std::floor(delaySamples));
        T frac = delaySamples - integerDelay;

        // Get 4 points: all in the past
        T x0 = buffer.read(ch, integerDelay);
        T xM1 = buffer.read(ch, integerDelay + 1);
        T xM2 = buffer.read(ch, integerDelay + 2);
        T xM3 = buffer.read(ch, integerDelay + 3);

        // Lagrange basis polynomials for fractional position
        float fracSq = frac * frac;
        float fracCube = fracSq * frac;

        // L_0(frac) = -(frac - 1) * (frac - 2) * (frac - 3) / 6
        T c0 = x0 * (-fracCube + 6.0f * fracSq - 11.0f * frac + 6.0f) / 6.0f;

        // L_{-1}(frac) = frac * (frac - 2) * (frac - 3) / 2
        T c1 = xM1 * (fracCube - 5.0f * fracSq + 6.0f * frac) / 2.0f;

        // L_{-2}(frac) = -frac * (frac - 1) * (frac - 3) / 2
        T c2 = xM2 * (-fracCube + 4.0f * fracSq - 3.0f * frac) / 2.0f;

        // L_{-3}(frac) = frac * (frac - 1) * (frac - 2) / 6
        T c3 = xM3 * (fracCube - 3.0f * fracSq + 2.0f * frac) / 6.0f;

        return c0 + c1 + c2 + c3;
    }
};

} // namespace jnsc::detail
