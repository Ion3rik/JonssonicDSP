// Jonssonic - A Modular Realtime C++ Audio DSP Library
// Biquad filter coefficient computation functions
// Based on Robert Bristow-Johnson's Audio EQ Cookbook
// SPDX-License-Identifier: MIT

#pragma once
#include <cmath>
#include <jonssonic/utils/math_utils.h>

namespace jonssonic::core::filters::detail
{

/**
 * @brief Compute lowpass biquad filter coefficients.
 * @param normFreq Normalized cutoff frequency (0..0.5, where 0.5 = Nyquist)
 * @param Q Quality factor
 * @param b0 Feedforward coefficient 0 (output)
 * @param b1 Feedforward coefficient 1 (output)
 * @param b2 Feedforward coefficient 2 (output)
 * @param a1 Feedback coefficient 1 (output)
 * @param a2 Feedback coefficient 2 (output)
 */
template<typename T>
inline void computeLowpassCoeffs(T normFreq, T Q, T& b0, T& b1, T& b2, T& a1, T& a2)
{
    T w0 = utils::two_pi<T> * normFreq;
    T cosw0 = std::cos(w0);
    T sinw0 = std::sin(w0);
    T alpha = sinw0 / (T(2) * Q);
    T a0 = T(1) + alpha;
    b0 = ((T(1) - cosw0) / T(2)) / a0;
    b1 = (T(1) - cosw0) / a0;
    b2 = ((T(1) - cosw0) / T(2)) / a0;
    a1 = (-T(2) * cosw0) / a0;
    a2 = (T(1) - alpha) / a0;
}

/**
 * @brief Compute highpass biquad filter coefficients.
 * @param normFreq Normalized cutoff frequency (0..0.5, where 0.5 = Nyquist)
 * @param Q Quality factor
 * @param b0 Feedforward coefficient 0 (output)
 * @param b1 Feedforward coefficient 1 (output)
 * @param b2 Feedforward coefficient 2 (output)
 * @param a1 Feedback coefficient 1 (output)
 * @param a2 Feedback coefficient 2 (output)
 */
template<typename T>
inline void computeHighpassCoeffs(T normFreq, T Q, T& b0, T& b1, T& b2, T& a1, T& a2)
{
    T w0 = utils::two_pi<T> * normFreq;
    T cosw0 = std::cos(w0);
    T sinw0 = std::sin(w0);
    T alpha = sinw0 / (T(2) * Q);
    T a0 = T(1) + alpha;
    b0 = ((T(1) + cosw0) / T(2)) / a0;
    b1 = -(T(1) + cosw0) / a0;
    b2 = ((T(1) + cosw0) / T(2)) / a0;
    a1 = (-T(2) * cosw0) / a0;
    a2 = (T(1) - alpha) / a0;
}

/**
 * @brief Compute bandpass biquad filter coefficients (constant skirt gain).
 * @param normFreq Normalized center frequency (0..0.5, where 0.5 = Nyquist)
 * @param Q Quality factor
 * @param b0 Feedforward coefficient 0 (output)
 * @param b1 Feedforward coefficient 1 (output)
 * @param b2 Feedforward coefficient 2 (output)
 * @param a1 Feedback coefficient 1 (output)
 * @param a2 Feedback coefficient 2 (output)
 */
template<typename T>
inline void computeBandpassCoeffs(T normFreq, T Q, T& b0, T& b1, T& b2, T& a1, T& a2)
{
    T w0 = utils::two_pi<T> * normFreq;
    T cosw0 = std::cos(w0);
    T sinw0 = std::sin(w0);
    T alpha = sinw0 / (T(2) * Q);
    T a0 = T(1) + alpha;
    b0 = alpha / a0;
    b1 = T(0);
    b2 = -alpha / a0;
    a1 = (-T(2) * cosw0) / a0;
    a2 = (T(1) - alpha) / a0;
}

/**
 * @brief Compute allpass biquad filter coefficients.
 * @param normFreq Normalized center frequency (0..0.5, where 0.5 = Nyquist)
 * @param Q Quality factor
 * @param b0 Feedforward coefficient 0 (output)
 * @param b1 Feedforward coefficient 1 (output)
 * @param b2 Feedforward coefficient 2 (output)
 * @param a1 Feedback coefficient 1 (output)
 * @param a2 Feedback coefficient 2 (output)
 */
template<typename T>
inline void computeAllpassCoeffs(T normFreq, T Q, T& b0, T& b1, T& b2, T& a1, T& a2)
{
    T w0 = utils::two_pi<T> * normFreq;
    T cosw0 = std::cos(w0);
    T sinw0 = std::sin(w0);
    T alpha = sinw0 / (T(2) * Q);
    T a0 = T(1) + alpha;
    b0 = (T(1) - alpha) / a0;
    b1 = (-T(2) * cosw0) / a0;
    b2 = (T(1) + alpha) / a0;
    a1 = (-T(2) * cosw0) / a0;
    a2 = (T(1) - alpha) / a0;
}

/**
 * @brief Compute notch biquad filter coefficients.
 * @param normFreq Normalized center frequency (0..0.5, where 0.5 = Nyquist)
 * @param Q Quality factor
 * @param b0 Feedforward coefficient 0 (output)
 * @param b1 Feedforward coefficient 1 (output)
 * @param b2 Feedforward coefficient 2 (output)
 * @param a1 Feedback coefficient 1 (output)
 * @param a2 Feedback coefficient 2 (output)
 */
template<typename T>
inline void computeNotchCoeffs(T normFreq, T Q, T& b0, T& b1, T& b2, T& a1, T& a2)
{
    T w0 = utils::two_pi<T> * normFreq;
    T cosw0 = std::cos(w0);
    T sinw0 = std::sin(w0);
    T alpha = sinw0 / (T(2) * Q);
    T a0 = T(1) + alpha;
    b0 = T(1) / a0;
    b1 = (-T(2) * cosw0) / a0;
    b2 = T(1) / a0;
    a1 = (-T(2) * cosw0) / a0;
    a2 = (T(1) - alpha) / a0;
}

/**
 * @brief Compute peaking EQ biquad filter coefficients.
 * @param normFreq Normalized center frequency (0..0.5, where 0.5 = Nyquist)
 * @param Q Quality factor
 * @param gainLinear Linear gain (not dB)
 * @param b0 Feedforward coefficient 0 (output)
 * @param b1 Feedforward coefficient 1 (output)
 * @param b2 Feedforward coefficient 2 (output)
 * @param a1 Feedback coefficient 1 (output)
 * @param a2 Feedback coefficient 2 (output)
 */
template<typename T>
inline void computePeakCoeffs(T normFreq, T Q, T gainLinear,
                               T& b0, T& b1, T& b2, T& a1, T& a2)
{
    // For peaking EQ, A should be sqrt(gainLinear), not gainLinear
    T A = std::sqrt(gainLinear);
    T w0 = utils::two_pi<T> * normFreq;
    T cosw0 = std::cos(w0);
    T sinw0 = std::sin(w0);
    T alpha = sinw0 / (T(2) * Q);
    // Standard peaking EQ formulas from Audio EQ Cookbook
    T a0 = T(1) + alpha / A;
    b0 = (T(1) + alpha * A) / a0;
    b1 = (-T(2) * cosw0) / a0;
    b2 = (T(1) - alpha * A) / a0;
    a1 = (-T(2) * cosw0) / a0;
    a2 = (T(1) - alpha / A) / a0;
}

/**
 * @brief Compute low shelf biquad filter coefficients.
 * @param normFreq Normalized transition frequency (0..0.5, where 0.5 = Nyquist)
 * @param Q Quality factor (shelf slope)
 * @param gainLinear Linear gain (not dB)
 * @param b0 Feedforward coefficient 0 (output)
 * @param b1 Feedforward coefficient 1 (output)
 * @param b2 Feedforward coefficient 2 (output)
 * @param a1 Feedback coefficient 1 (output)
 * @param a2 Feedback coefficient 2 (output)
 */
template<typename T>
inline void computeLowshelfCoeffs(T normFreq, T Q, T gainLinear,
                                   T& b0, T& b1, T& b2, T& a1, T& a2)
{
    T A = std::sqrt(gainLinear);
    T w0 = utils::two_pi<T> * normFreq;
    T cosw0 = std::cos(w0);
    T sinw0 = std::sin(w0);
    T alpha = sinw0 / (T(2) * Q);
    T sqrtA = std::sqrt(A);
    T a0 = (A + T(1)) + (A - T(1)) * cosw0 + T(2) * sqrtA * alpha;
    b0 = (A * ((A + T(1)) - (A - T(1)) * cosw0 + T(2) * sqrtA * alpha)) / a0;
    b1 = (T(2) * A * ((A - T(1)) - (A + T(1)) * cosw0)) / a0;
    b2 = (A * ((A + T(1)) - (A - T(1)) * cosw0 - T(2) * sqrtA * alpha)) / a0;
    a1 = (-T(2) * ((A - T(1)) + (A + T(1)) * cosw0)) / a0;
    a2 = ((A + T(1)) + (A - T(1)) * cosw0 - T(2) * sqrtA * alpha) / a0;
}

/**
 * @brief Compute high shelf biquad filter coefficients.
 * @param normFreq Normalized transition frequency (0..0.5, where 0.5 = Nyquist)
 * @param Q Quality factor (shelf slope)
 * @param gainLinear Linear gain (not dB)
 * @param b0 Feedforward coefficient 0 (output)
 * @param b1 Feedforward coefficient 1 (output)
 * @param b2 Feedforward coefficient 2 (output)
 * @param a1 Feedback coefficient 1 (output)
 * @param a2 Feedback coefficient 2 (output)
 */
template<typename T>
inline void computeHighshelfCoeffs(T normFreq, T Q, T gainLinear,
                                    T& b0, T& b1, T& b2, T& a1, T& a2)
{
    T A = std::sqrt(gainLinear);
    T w0 = utils::two_pi<T> * normFreq;
    T cosw0 = std::cos(w0);
    T sinw0 = std::sin(w0);
    T alpha = sinw0 / (T(2) * Q);
    T sqrtA = std::sqrt(A);
    T a0 = (A + T(1)) - (A - T(1)) * cosw0 + T(2) * sqrtA * alpha;
    b0 = (A * ((A + T(1)) + (A - T(1)) * cosw0 + T(2) * sqrtA * alpha)) / a0;
    b1 = (-T(2) * A * ((A - T(1)) + (A + T(1)) * cosw0)) / a0;
    b2 = (A * ((A + T(1)) + (A - T(1)) * cosw0 - T(2) * sqrtA * alpha)) / a0;
    a1 = (T(2) * ((A - T(1)) - (A + T(1)) * cosw0)) / a0;
    a2 = ((A + T(1)) - (A - T(1)) * cosw0 - T(2) * sqrtA * alpha) / a0;
}

} // namespace jonssonic::filters::detail
