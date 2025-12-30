// Jonssonic - A Modular Realtime C++ Audio DSP Library
// First-order filter coefficient computation functions
// SPDX-License-Identifier: MIT

#pragma once
#include <cmath>
#include "../../utils/math_utils.h"

namespace jonssonic::filters::detail
{ 

/**
 * @brief Compute first-order lowpass filter coefficients.
 * @param normFreq Normalized cutoff frequency (0..0.5, where 0.5 = Nyquist)
 * @param b0 Feedforward coefficient 0 (output)
 * @param b1 Feedforward coefficient 1 (output)
 * @param a1 Feedback coefficient 1 (output)
 */
template<typename T>
inline void computeFirstOrderLowpassCoeffs(T normFreq, T& b0, T& b1, T& a1) {
    // Standard first-order lowpass (bilinear transform)
    // normFreq: normalized cutoff (0..0.5, where 0.5 = Nyquist)
    T theta = two_pi<T> * normFreq;
    T gamma = std::cos(theta) / (1.0 + std::sin(theta));
    a1 = -gamma;
    b0 = (1.0 - gamma) / 2.0;
    b1 = (1.0 - gamma) / 2.0;
}

/**
 * @brief Compute first-order highpass filter coefficients.
 * @param normFreq Normalized cutoff frequency (0..0.5, where 0.5 = Nyquist)
 * @param b0 Feedforward coefficient 0 (output)
 * @param b1 Feedforward coefficient 1 (output)
 * @param a1 Feedback coefficient 1 (output)
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
 * @param normFreq Normalized center frequency (0..0.5, where 0.5 = Nyquist)
 * @param b0 Feedforward coefficient 0 (output)
 * @param b1 Feedforward coefficient 1 (output)
 * @param a1 Feedback coefficient 1 (output)
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
 * @param normFreq Normalized transition frequency (0..0.5, where 0.5 = Nyquist)
 * @param gainLinear Linear gain
 * @param b0 Feedforward coefficient 0 (output)
 * @param b1 Feedforward coefficient 1 (output)
 * @param a1 Feedback coefficient 1 (output)
 */
template<typename T>
inline void computeFirstOrderLowshelfCoeffs(T normFreq, T gainLinear, T& b0, T& b1, T& a1) {
    // Prewarp frequency
    T omega_c = two_pi<T> * normFreq;
    T tan_wc_2 = std::tan(omega_c / T(2));

    T sqrt_g = std::sqrt(gainLinear);

    // Denominator normalization factor
    T denom = tan_wc_2 + sqrt_g;

    b0 = (gainLinear * tan_wc_2 + sqrt_g) / denom;
    b1 = (gainLinear * tan_wc_2 - sqrt_g) / denom;
    a1 = (tan_wc_2 - sqrt_g) / denom;
}

/**
 * @brief Compute first-order highshelf filter coefficients.
 * @param normFreq Normalized transition frequency (0..0.5, where 0.5 = Nyquist)
 * @param gainLinear Linear gain
 * @param b0 Feedforward coefficient 0 (output)
 * @param b1 Feedforward coefficient 1 (output)
 * @param a1 Feedback coefficient 1 (output)
 */
template<typename T>
inline void computeFirstOrderHighshelfCoeffs(T normFreq, T gainLinear, T& b0, T& b1, T& a1) {
    // Prewarp frequency
    T omega_c = two_pi<T> * normFreq;
    T tan_wc_2 = std::tan(omega_c / T(2));

    T sqrt_g = std::sqrt(gainLinear);

    // Denominator normalization factor
    T denom = tan_wc_2 + sqrt_g;

    b0 = (gainLinear * tan_wc_2 + sqrt_g) / denom;
    b1 = -(gainLinear * tan_wc_2 - sqrt_g) / denom;
    a1 = (tan_wc_2 - sqrt_g) / denom;
}

} // namespace jonssonic::filters::detail
