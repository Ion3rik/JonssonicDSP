// Jonssonic - A Modular Realtime C++ Audio DSP Library
// First-order filter coefficient computation functions
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/utils/detail/config_utils.h"
#include <cmath>
#include <jonssonic/utils/math_utils.h>

namespace jnsc::detail {

/**
 * @brief Enumeration for coefficient computation methods.
 */
enum class CoeffMethod { Bilinear, SmallAngle };

/**
 * @brief Compute first-order lowpass filter coefficients using the specified method.
 * @param normFreq Normalized cutoff frequency (0..0.5, where 0.5 = Nyquist)
 * @param b0 Feedforward coefficient 0 (output)
 * @param b1 Feedforward coefficient 1 (output)
 * @param a1 Feedback coefficient 1 (output)
 */
template <typename T, CoeffMethod Method = CoeffMethod::Bilinear>
inline void computeFirstOrderLowpassCoeffs(T normFreq, T& b0, T& b1, T& a1) {
    // Compute k based on the selected method
    T k = T(0);
    if constexpr (Method == CoeffMethod::Bilinear)
        k = std::tan(utils::pi<T> * normFreq); // bilinear transform method (accurate for all frequencies)
    if constexpr (Method == CoeffMethod::SmallAngle)
        k = utils::pi<T> * normFreq; // small-angle approximation for tan(x) ~ x when x -> 0

    // Compute a0 normalized coefficients
    T a0 = T(1) / (T(1) + k);
    b0 = k * a0;
    b1 = k * a0;
    a1 = (T(1) - k) * a0;
}

/**
 * @brief Compute first-order highpass filter coefficients using the specified method.
 * @param normFreq Normalized cutoff frequency (0..0.5, where 0.5 = Nyquist)
 * @param b0 Feedforward coefficient 0 (output)
 * @param b1 Feedforward coefficient 1 (output)
 * @param a1 Feedback coefficient 1 (output)
 */
template <typename T, CoeffMethod Method = CoeffMethod::Bilinear>
inline void computeFirstOrderHighpassCoeffs(T normFreq, T& b0, T& b1, T& a1) {
    // Compute k based on the selected method
    T k = T(0);
    if constexpr (Method == CoeffMethod::Bilinear)
        k = std::tan(utils::pi<T> * normFreq); // bilinear transform method (accurate for all frequencies)
    if constexpr (Method == CoeffMethod::SmallAngle)
        k = utils::pi<T> * normFreq; // small-angle approximation for tan(x) ~ x when x -> 0

    // Compute a0 normalized coefficients
    T a0 = T(1) / (T(1) + k);
    b0 = a0;
    b1 = -a0;
    a1 = (T(1) - k) * a0;
}

/**
 * @brief Compute first-order allpass filter coefficients using the specified method.
 * @param normFreq Normalized center frequency (0..0.5, where 0.5 = Nyquist)
 * @param b0 Feedforward coefficient 0 (output)
 * @param b1 Feedforward coefficient 1 (output)
 * @param a1 Feedback coefficient 1 (output)
 */
template <typename T, CoeffMethod Method = CoeffMethod::Bilinear>
inline void computeFirstOrderAllpassCoeffs(T normFreq, T& b0, T& b1, T& a1) {
    // Compute k based on the selected method
    T k = T(0);
    if constexpr (Method == CoeffMethod::Bilinear)
        k = std::tan(utils::pi<T> * normFreq); // bilinear transform method (accurate for all frequencies)
    if constexpr (Method == CoeffMethod::SmallAngle)
        k = utils::pi<T> * normFreq; // small-angle approximation for tan(x) ~ x when x -> 0

    // Compute coefficients (b0 = a1 for allpass)
    T coeff = (T(1) - k) / (T(1) + k);
    b0 = coeff;
    b1 = T(1);
    a1 = coeff;
}

/**
 * @brief Compute first-order lowshelf filter coefficients using the bilinear transform method.
 * @param normFreq Normalized transition frequency (0..0.5, where 0.5 = Nyquist)
 * @param gainLinear Linear gain
 * @param b0 Feedforward coefficient 0 (output)
 * @param b1 Feedforward coefficient 1 (output)
 * @param a1 Feedback coefficient 1 (output)
 */
template <typename T>
inline void computeFirstOrderLowshelfCoeffs(T normFreq, T gainLinear, T& b0, T& b1, T& a1) {
    // Bilinear transform prewarp
    T k = std::tan(utils::pi<T> * normFreq);
    T sqrt_g = std::sqrt(gainLinear);

    // Compute a0 normalized coefficients
    T a0 = k + sqrt_g;
    b0 = (gainLinear * k + sqrt_g) / a0;
    b1 = (gainLinear * k - sqrt_g) / a0;
    a1 = (k - sqrt_g) / a0;
}

/**
 * @brief Compute first-order highshelf filter coefficients using the bilinear transform method.
 * @param normFreq Normalized transition frequency (0..0.5, where 0.5 = Nyquist)
 * @param gainLinear Linear gain
 * @param b0 Feedforward coefficient 0 (output)
 * @param b1 Feedforward coefficient 1 (output)
 * @param a1 Feedback coefficient 1 (output)
 */
template <typename T>
inline void computeFirstOrderHighshelfCoeffs(T normFreq, T gainLinear, T& b0, T& b1, T& a1) {
    // Bilinear transform prewarp
    T k = std::tan(utils::pi<T> * normFreq);
    T sqrt_g = std::sqrt(gainLinear);

    // Compute a0 normalized coefficients
    T a0 = sqrt_g * k + T(1);
    b0 = (sqrt_g * k + gainLinear) / a0;
    b1 = (sqrt_g * k - gainLinear) / a0;
    a1 = (sqrt_g * k - T(1)) / a0;
}

} // namespace jnsc::detail
