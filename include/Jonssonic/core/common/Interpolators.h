
// Jonssonic - A C++ audio DSP library
// Interpolator structs for audio processing
// Author: Jon Fagerström
// Update: 18.11.2025

/**
 * @file Interpolators.h
 * @brief This file contains various interpolation methods used in audio processing.
 * @namespace Jonssonic
 */

#pragma once

namespace Jonssonic
{
/**
 * @brief Nearest neighbor interpolator.
 * @tparam T The data type of the samples (e.g., float, double).
 * This struct provides a method to perform nearest neighbor interpolation.
 * @returns The interpolated sample value.
 */
template<typename T>
struct NearestInterpolator
{
    static T interpolate(const T* buffer, size_t idx, float frac)
    {
        return (frac < 0.5f) ? buffer[idx] : buffer[idx + 1];
    }
};

/**
 * @brief Linear interpolator.
 * @tparam T The data type of the samples (e.g., float, double).
 * This struct provides a method to perform linear interpolation between two samples.
 * @returns The interpolated sample value.
 */
template<typename T>
struct LinearInterpolator
{
    static T interpolate(const T* buffer, size_t idx, float frac)
    {
        return buffer[idx] * (1.0f - frac) + buffer[idx + 1] * frac;
    }
};

} // namespace Jonssonic::core
