// Jonssonic - A Modular Realtime C++ Audio DSP Library
// First-order filter coefficient computation functions
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/utils/detail/config_utils.h"
#include <cmath>
#include <jonssonic/utils/math_utils.h>

namespace jnsc::detail {

/**
 * @brief Compute first-order lowpass filter coefficients.
 * @param normFreq Normalized cutoff frequency (0..0.5, where 0.5 = Nyquist)
 * @param b0 Feedforward coefficient 0 (output)
 * @param b1 Feedforward coefficient 1 (output)
 * @param a1 Feedback coefficient 1 (output)
 */
template <typename T>
inline void computeFirstOrderLowpassCoeffs(T normFreq, T& b0, T& b1, T& a1) {

    // Pre-warp the normalized frequency for bilinear transform
    T k = std::tan(utils::pi<T> * normFreq);

    // Compute a0 normalized coefficients
    T a0 = T(1) / (T(1) + k);
    b0 = k * a0;
    b1 = k * a0;
    a1 = (T(1) - k) * a0;
}

/**
 * @brief Compute first-order highpass filter coefficients.
 * @param normFreq Normalized cutoff frequency (0..0.5, where 0.5 = Nyquist)
 * @param b0 Feedforward coefficient 0 (output)
 * @param b1 Feedforward coefficient 1 (output)
 * @param a1 Feedback coefficient 1 (output)
 */
template <typename T>
inline void computeFirstOrderHighpassCoeffs(T normFreq, T& b0, T& b1, T& a1) {
    // Pre-warp the normalized frequency for bilinear transform
    T k = std::tan(utils::pi<T> * normFreq);

    // Compute a0 normalized coefficients
    T a0 = T(1) / (T(1) + k);
    b0 = a0;
    b1 = -a0;
    a1 = (T(1) - k) * a0;
}

/**
 * @brief Compute first-order allpass filter coefficients.
 * @param normFreq Normalized center frequency (0..0.5, where 0.5 = Nyquist)
 * @param b0 Feedforward coefficient 0 (output)
 * @param b1 Feedforward coefficient 1 (output)
 * @param a1 Feedback coefficient 1 (output)
 */
template <typename T>
inline void computeFirstOrderAllpassCoeffs(T normFreq, T& b0, T& b1, T& a1) {
    T x = std::exp(-utils::two_pi<T> * normFreq);
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
template <typename T>
inline void computeFirstOrderLowshelfCoeffs(T normFreq, T gainLinear, T& b0, T& b1, T& a1) {
    // Precompute constants
    T G = gainLinear;
    T omega_c = utils::two_pi<T> * normFreq;
    T tan_wc_over_2 = std::tan(omega_c / T(2));
    T sqrt_g = std::sqrt(gainLinear);

    // Compute a0 normalized coefficients
    T a0 = tan_wc_over_2 + sqrt_g;
    b0 = (G * tan_wc_over_2 + sqrt_g) / a0;
    b1 = (G * tan_wc_over_2 - sqrt_g) / a0;
    a1 = (tan_wc_over_2 - sqrt_g) / a0;
}

/**
 * @brief Compute first-order highshelf filter coefficients.
 * @param normFreq Normalized transition frequency (0..0.5, where 0.5 = Nyquist)
 * @param gainLinear Linear gain
 * @param b0 Feedforward coefficient 0 (output)
 * @param b1 Feedforward coefficient 1 (output)
 * @param a1 Feedback coefficient 1 (output)
 */
template <typename T>
inline void computeFirstOrderHighshelfCoeffs(T normFreq, T gainLinear, T& b0, T& b1, T& a1) {
    // Precompute constants
    T G = gainLinear;
    T omega_c = utils::two_pi<T> * normFreq;
    T tan_wc_over_2 = std::tan(omega_c / T(2));
    T sqrt_g = std::sqrt(gainLinear);

    // Compute a0 normalized coefficients
    T a0 = sqrt_g * tan_wc_over_2 + T(1);
    b0 = (sqrt_g * tan_wc_over_2 + G) / a0;
    b1 = (sqrt_g * tan_wc_over_2 - G) / a0;
    a1 = (sqrt_g * tan_wc_over_2 - T(1)) / a0;
}

} // namespace jnsc::detail
