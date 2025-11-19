// Jonssonic - A C++ audio DSP library
// Math utility functions
// Author: Jon Fagerström
// Update: 18.11.2025

#pragma once
#include <cstddef>

namespace Jonssonic
{
/**
 * @brief Calculate the next power of two greater than or equal to n.
 * @param n Input value
 * @return The next power of two
 */
inline size_t nextPowerOfTwo(size_t n)
{
    if (n == 0) return 1;
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    return n + 1;
}

/**
 * @brief Convert milliseconds to samples.
 * @param ms Time in milliseconds
 * @param sampleRate Sample rate in Hz
 * @return Number of samples
 */
template<typename T>
inline size_t msToSamples(T ms, T sampleRate)
{
    return static_cast<size_t>(ms * sampleRate / T(1000.0));
}

/**
 * @brief Convert samples to milliseconds.
 * @param samples Number of samples
 * @param sampleRate Sample rate in Hz
 * @return Time in milliseconds
 */
template<typename T>
inline T samplesToMs(size_t samples, T sampleRate)
{
    return static_cast<T>(samples) * T(1000.0) / sampleRate;
}

} // namespace Jonssonic
