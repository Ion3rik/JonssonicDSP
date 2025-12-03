// Jonssonic - A C++ audio DSP library
// Math utility functions
// Author: Jon Fagerström
// Update: 18.11.2025

#pragma once
#include <cstddef>
#include <numbers>
#include <complex>
#include <vector>


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

//==============================================================================
// Simple DFT/FFT Implementation (for test/analysis use)
//==============================================================================


// Compute the DFT of a real input vector (output is complex)
template<typename T>
std::vector<std::complex<T>> complexSpectrum(const std::vector<T>& input) {
    size_t N = input.size();
    std::vector<std::complex<T>> output(N);
    const T pi = Jonssonic::pi<T>;
    for (size_t k = 0; k < N; ++k) {
        std::complex<T> sum(0, 0);
        for (size_t n = 0; n < N; ++n) {
            T angle = -2 * pi * k * n / N;
            sum += input[n] * std::exp(std::complex<T>(0, angle));
        }
        output[k] = sum;
    }
    return output;
}


// Compute the magnitude spectrum of a real input vector
// If oneSided=true, returns only the first N/2+1 bins (for real input)
// If dB=true, returns 20*log10(mag+1e-12)
template<typename T>
std::vector<T> magnitudeSpectrum(const std::vector<T>& input, bool oneSided = false, bool dB = false) {
    auto spec = complexSpectrum(input);
    size_t N = spec.size();
    size_t outLen = oneSided ? (N / 2 + 1) : N;
    std::vector<T> mag(outLen);
    constexpr T minMag = T(1e-12); // minimum magnitude to avoid log(0)
    constexpr T minDb = T(-120);   // minimum dB value to clamp to
    for (size_t i = 0; i < outLen; ++i) {
        T val = std::abs(spec[i]);
        if (dB) {
            val = 20 * std::log10(std::max(val, minMag));
            if (val < minDb) val = minDb;
        }
        mag[i] = val;
    }
    return mag;
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
