// Jonssonic - A Modular Realtime C++ Audio DSP Library
// Interpolator structs for audio processing
// SPDX-License-Identifier: MIT

/**
 * @file Interpolators.h
 * @brief This file contains various interpolation methods used in audio processing.
 * @namespace jnsc::common
 */

#pragma once

namespace jnsc {
/**
 * @brief None interpolation, direct sample access.
 * @tparam T The data type of the samples (e.g., float, double).
 * This struct provides a method to access samples without interpolation.
 * @returns The sample value at the given index.
 */
template <typename T>
struct NoneInterpolator {
    static T
    interpolateBackward(const T* buffer, size_t idx, float /*frac*/, size_t /*bufferSize*/) {
        return buffer[idx];
    }

    static T
    interpolateForward(const T* buffer, size_t idx, float /*frac*/, size_t /*bufferSize*/) {
        return buffer[idx];
    }
};
/**
 * @brief Nearest neighbor interpolator.
 * @tparam T The data type of the samples (e.g., float, double).
 * This struct provides a method to perform nearest neighbor interpolation.
 * @returns The interpolated sample value.
 */
template <typename T>
struct NearestInterpolator {
    // For delay lines - interpolate backward in time
    static T interpolateBackward(const T* buffer, size_t idx, float frac, size_t bufferSize) {
        size_t prevIdx = (idx + bufferSize - 1) & (bufferSize - 1);
        return (frac < 0.5f) ? buffer[idx] : buffer[prevIdx];
    }

    // For wavetables/resampling - interpolate forward
    static T interpolateForward(const T* buffer, size_t idx, float frac, size_t bufferSize) {
        size_t nextIdx = (idx + 1) & (bufferSize - 1);
        return (frac < 0.5f) ? buffer[idx] : buffer[nextIdx];
    }
};

/**
 * @brief Linear interpolator.
 * @tparam T The data type of the samples (e.g., float, double).
 * This struct provides a method to perform linear interpolation between two samples.
 * @returns The interpolated sample value.
 */
template <typename T>
struct LinearInterpolator {
    // For delay lines - interpolate backward in time
    static T interpolateBackward(const T* buffer, size_t idx, float frac, size_t bufferSize) {
        size_t prevIdx = (idx + bufferSize - 1) & (bufferSize - 1);
        return buffer[idx] * (1.0f - frac) + buffer[prevIdx] * frac;
    }

    // For wavetables/resampling - interpolate forward
    static T interpolateForward(const T* buffer, size_t idx, float frac, size_t bufferSize) {
        size_t nextIdx = (idx + 1) & (bufferSize - 1);
        return buffer[idx] * (1.0f - frac) + buffer[nextIdx] * frac;
    }
};

/**
 * @brief 4-point Lagrange interpolator (3rd order).
 * @tparam T The data type of the samples (e.g., float, double).
 *
 * High-quality interpolation using 4 points. Provides excellent frequency
 * response and is commonly used in professional audio applications for
 * modulated delays, pitch shifting, and resampling.
 *
 * @returns The interpolated sample value.
 */
template <typename T>
struct LagrangeInterpolator {
    // For delay lines - interpolate backward in time
    // Uses points: x[-3], x[-2], x[-1], x[0] where x[0] is the current index
    // Only uses past samples to avoid causality issues
    static T interpolateBackward(const T* buffer, size_t idx, float frac, size_t bufferSize) {
        // Get 4 points: all in the past
        size_t idx0 = idx;                                        // x[0]
        size_t idxM1 = (idx + bufferSize - 1) & (bufferSize - 1); // x[-1]
        size_t idxM2 = (idx + bufferSize - 2) & (bufferSize - 1); // x[-2]
        size_t idxM3 = (idx + bufferSize - 3) & (bufferSize - 1); // x[-3]

        T x0 = buffer[idx0];
        T xM1 = buffer[idxM1];
        T xM2 = buffer[idxM2];
        T xM3 = buffer[idxM3];

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

    // For wavetables/resampling - interpolate forward
    // Uses points: x[-1], x[0], x[1], x[2] where x[0] is the current index
    static T interpolateForward(const T* buffer, size_t idx, float frac, size_t bufferSize) {
        // Get 4 points: one before, current, and two ahead
        size_t idx0 = idx;
        size_t idxM1 = (idx + bufferSize - 1) & (bufferSize - 1); // x[-1]
        size_t idxP1 = (idx + 1) & (bufferSize - 1);              // x[+1]
        size_t idxP2 = (idx + 2) & (bufferSize - 1);              // x[+2]

        T xM1 = buffer[idxM1];
        T x0 = buffer[idx0];
        T xP1 = buffer[idxP1];
        T xP2 = buffer[idxP2];

        // Lagrange basis polynomials for fractional position
        float fracSq = frac * frac;
        float fracCube = fracSq * frac;

        // L_{-1}(frac) = -frac * (frac - 1) * (frac - 2) / 6
        T c0 = xM1 * (-fracCube + 3.0f * fracSq - 2.0f * frac) / 6.0f;

        // L_0(frac) = (frac + 1) * (frac - 1) * (frac - 2) / 2
        T c1 = x0 * (fracCube - 2.0f * fracSq - frac + 2.0f) / 2.0f;

        // L_1(frac) = -(frac + 1) * frac * (frac - 2) / 2
        T c2 = xP1 * (-fracCube + fracSq + 2.0f * frac) / 2.0f;

        // L_2(frac) = (frac + 1) * frac * (frac - 1) / 6
        T c3 = xP2 * (fracCube - fracSq) / 6.0f;

        return c0 + c1 + c2 + c3;
    }
};

} // namespace jnsc
