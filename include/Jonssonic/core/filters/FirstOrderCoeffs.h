// Jonssonic - A C++ audio DSP library
// First-order filter coefficient computation functions
// SPDX-License-Identifier: MIT

#pragma once
#include <cmath>
#include "../../utils/MathUtils.h"

namespace Jonssonic {

/**
 * @brief Compute first-order lowpass filter coefficients.
 * @param normFreq Normalized cutoff frequency (0..0.5, where 0.5 = Nyquist)
 * @param b0 Feedforward coefficient 0 (output)
 * @param b1 Feedforward coefficient 1 (output)
 * @param a1 Feedback coefficient 1 (output)
 */
template<typename T>
inline void computeFirstOrderLowpassCoeffs(T normFreq, T& b0, T& b1, T& a1) {
    T x = std::exp(-two_pi<T> * normFreq);
    b0 = T(1) - x;
    b1 = T(0);
    a1 = x;
}

/**
 * @brief Compute first-order highpass filter coefficients.
 */
template<typename T>
inline void computeFirstOrderHighpassCoeffs(T normFreq, T& b0, T& b1, T& a1) {
    T x = std::exp(-two_pi<T> * normFreq);
    b0 = (T(1) + x) / 2;
    b1 = -(T(1) + x) / 2;
    a1 = x;
}

/**
 * @brief Compute first-order allpass filter coefficients.
 */
template<typename T>
inline void computeFirstOrderAllpassCoeffs(T normFreq, T& b0, T& b1, T& a1) {
    T x = std::exp(-two_pi<T> * normFreq);
    b0 = x;
    b1 = T(1);
    a1 = x;
}

/**
 * @brief Compute first-order lowshelf filter coefficients.
 * @param gainLinear Linear gain (not dB)
 */
template<typename T>
inline void computeFirstOrderLowshelfCoeffs(T normFreq, T gainLinear, T& b0, T& b1, T& a1) {
    T x = std::exp(-two_pi<T> * normFreq);
    b0 = (T(1) - x) * gainLinear;
    b1 = T(0);
    a1 = x;
}

/**
 * @brief Compute first-order highshelf filter coefficients.
 * @param gainLinear Linear gain (not dB)
 */
template<typename T>
inline void computeFirstOrderHighshelfCoeffs(T normFreq, T gainLinear, T& b0, T& b1, T& a1) {
    T x = std::exp(-two_pi<T> * normFreq);
    b0 = ((T(1) + x) / 2) * gainLinear;
    b1 = -((T(1) + x) / 2) * gainLinear;
    a1 = x;
}

} // namespace Jonssonic
