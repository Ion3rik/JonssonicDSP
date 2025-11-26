
// Jonssonic - A C++ audio DSP library
// Interpolator structs for audio processing
// SPDX-License-Identifier: MIT

/**
 * @file Interpolators.h
 * @brief This file contains various interpolation methods used in audio processing.
 * @namespace Jonssonic
 */

#pragma once

namespace Jonssonic
{
/** 
* @brief None interpolation, direct sample access.
* @tparam T The data type of the samples (e.g., float, double).
* This struct provides a method to access samples without interpolation.
* @returns The sample value at the given index.
*/
template<typename T>
struct NoneInterpolator
{
    static T interpolateBackward(const T* buffer, size_t idx, float /*frac*/, size_t /*bufferSize*/)
    {
        return buffer[idx];
    }
    
    static T interpolateForward(const T* buffer, size_t idx, float /*frac*/, size_t /*bufferSize*/)
    {
        return buffer[idx];
    }
};
/**
 * @brief Nearest neighbor interpolator.
 * @tparam T The data type of the samples (e.g., float, double).
 * This struct provides a method to perform nearest neighbor interpolation.
 * @returns The interpolated sample value.
 */
template<typename T>
struct NearestInterpolator
{
    // For delay lines - interpolate backward in time
    static T interpolateBackward(const T* buffer, size_t idx, float frac, size_t bufferSize)
    {
        size_t prevIdx = (idx + bufferSize - 1) & (bufferSize - 1);
        return (frac < 0.5f) ? buffer[idx] : buffer[prevIdx];
    }
    
    // For wavetables/resampling - interpolate forward
    static T interpolateForward(const T* buffer, size_t idx, float frac, size_t bufferSize)
    {
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
template<typename T>
struct LinearInterpolator
{
    // For delay lines - interpolate backward in time
    static T interpolateBackward(const T* buffer, size_t idx, float frac, size_t bufferSize)
    {
        size_t prevIdx = (idx + bufferSize - 1) & (bufferSize - 1);
        return buffer[idx] * (1.0f - frac) + buffer[prevIdx] * frac;
    }
    
    // For wavetables/resampling - interpolate forward
    static T interpolateForward(const T* buffer, size_t idx, float frac, size_t bufferSize)
    {
        size_t nextIdx = (idx + 1) & (bufferSize - 1);
        return buffer[idx] * (1.0f - frac) + buffer[nextIdx] * frac;
    }
};

} // namespace Jonssonic::core
