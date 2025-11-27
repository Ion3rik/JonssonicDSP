// Jonssonic - A C++ audio DSP library
// Math utility functions
// Author: Jon Fagerström
// Update: 18.11.2025

#pragma once
#include <cstddef>
#include <numbers>

namespace Jonssonic
{

// Mathematical constants (C++20 std::numbers or fallback)
#ifdef __cpp_lib_math_constants
using std::numbers::pi;
using std::numbers::e;
#else
template<typename T>
inline constexpr T pi = T(3.141592653589793238462643383279502884);

template<typename T>
inline constexpr T e = T(2.718281828459045235360287471352662498);
#endif

// Commonly used derived constants
template<typename T>
inline constexpr T pi_over_2 = pi<T> / T(2);

template<typename T>
inline constexpr T pi_over_4 = pi<T> / T(4);

template<typename T>
inline constexpr T two_pi = pi<T> * T(2);

template<typename T>
inline constexpr T inv_pi = T(1) / pi<T>;

template<typename T>
inline constexpr T sqrt2 = T(1.414213562373095048801688724209698079);

template<typename T>
inline constexpr T inv_sqrt2 = T(1) / sqrt2<T>;

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
 * @brief Convert milliseconds to samples (fractional allowed).
 * @param ms Time in milliseconds
 * @param sampleRate Sample rate in Hz
 * @return Number of samples (fractional allowed)
 */
template<typename T>
inline T msToSamples(T ms, T sampleRate)
{
    return ms * sampleRate / T(1000.0);
}

/**
 * @brief Convert samples to milliseconds (fractional allowed).
 * @param samples Number of samples (fractional allowed)
 * @param sampleRate Sample rate in Hz
 * @return Time in milliseconds
 */
template<typename T>
inline T samplesToMs(T samples, T sampleRate)
{
    return samples * T(1000.0) / sampleRate;
}

} // namespace Jonssonic
