// Jonssonic - A C++ audio DSP library
// Biquad filter coefficient computation functions
// Based on Robert Bristow-Johnson's Audio EQ Cookbook
// SPDX-License-Identifier: MIT

#pragma once
#include <cmath>
#include "../../utils/MathUtils.h"

namespace Jonssonic
{

/**
 * @brief Compute lowpass biquad filter coefficients.
 * @param freq Cutoff frequency in Hz
 * @param Q Quality factor
 * @param sampleRate Sample rate in Hz
 * @param b0 Feedforward coefficient 0 (output)
 * @param b1 Feedforward coefficient 1 (output)
 * @param b2 Feedforward coefficient 2 (output)
 * @param a1 Feedback coefficient 1 (output)
 * @param a2 Feedback coefficient 2 (output)
 */
template<typename T>
inline void computeLowpassCoeffs(T freq, T Q, T sampleRate, 
                                  T& b0, T& b1, T& b2, T& a1, T& a2)
{
    T w0 = two_pi<T> * freq / sampleRate;
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
 * @param freq Cutoff frequency in Hz
 * @param Q Quality factor
 * @param sampleRate Sample rate in Hz
 * @param b0 Feedforward coefficient 0 (output)
 * @param b1 Feedforward coefficient 1 (output)
 * @param b2 Feedforward coefficient 2 (output)
 * @param a1 Feedback coefficient 1 (output)
 * @param a2 Feedback coefficient 2 (output)
 */
template<typename T>
inline void computeHighpassCoeffs(T freq, T Q, T sampleRate,
                                   T& b0, T& b1, T& b2, T& a1, T& a2)
{
    T w0 = two_pi<T> * freq / sampleRate;
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
 * @param freq Center frequency in Hz
 * @param Q Quality factor
 * @param sampleRate Sample rate in Hz
 * @param b0 Feedforward coefficient 0 (output)
 * @param b1 Feedforward coefficient 1 (output)
 * @param b2 Feedforward coefficient 2 (output)
 * @param a1 Feedback coefficient 1 (output)
 * @param a2 Feedback coefficient 2 (output)
 */
template<typename T>
inline void computeBandpassCoeffs(T freq, T Q, T sampleRate,
                                   T& b0, T& b1, T& b2, T& a1, T& a2)
{
    T w0 = two_pi<T> * freq / sampleRate;
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
 * @param freq Center frequency in Hz
 * @param Q Quality factor
 * @param sampleRate Sample rate in Hz
 * @param b0 Feedforward coefficient 0 (output)
 * @param b1 Feedforward coefficient 1 (output)
 * @param b2 Feedforward coefficient 2 (output)
 * @param a1 Feedback coefficient 1 (output)
 * @param a2 Feedback coefficient 2 (output)
 */

template<typename T>
inline void computeAllpassCoeffs(T freq, T Q, T sampleRate,
                                  T& b0, T& b1, T& b2, T& a1, T& a2)
{
    T w0 = two_pi<T> * freq / sampleRate;
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
 * @param freq Center frequency in Hz
 * @param Q Quality factor
 * @param sampleRate Sample rate in Hz
 * @param b0 Feedforward coefficient 0 (output)
 * @param b1 Feedforward coefficient 1 (output)
 * @param b2 Feedforward coefficient 2 (output)
 * @param a1 Feedback coefficient 1 (output)
 * @param a2 Feedback coefficient 2 (output)
 */
template<typename T>
inline void computeNotchCoeffs(T freq, T Q, T sampleRate,
                                T& b0, T& b1, T& b2, T& a1, T& a2)
{
    T w0 = two_pi<T> * freq / sampleRate;
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
 * @param freq Center frequency in Hz
 * @param Q Quality factor
 * @param gainLinear Linear gain (not dB)
 * @param sampleRate Sample rate in Hz
 * @param b0 Feedforward coefficient 0 (output)
 * @param b1 Feedforward coefficient 1 (output)
 * @param b2 Feedforward coefficient 2 (output)
 * @param a1 Feedback coefficient 1 (output)
 * @param a2 Feedback coefficient 2 (output)
 */
template<typename T>
inline void computePeakCoeffs(T freq, T Q, T gainLinear, T sampleRate,
                               T& b0, T& b1, T& b2, T& a1, T& a2)
{
    T A = gainLinear;
    T w0 = two_pi<T> * freq / sampleRate;
    T cosw0 = std::cos(w0);
    T sinw0 = std::sin(w0);
    T alpha = sinw0 / (T(2) * Q);
    
    T a0 = T(1) + alpha / A;
    b0 = (T(1) + alpha * A) / a0;
    b1 = (-T(2) * cosw0) / a0;
    b2 = (T(1) - alpha * A) / a0;
    a1 = (-T(2) * cosw0) / a0;
    a2 = (T(1) - alpha / A) / a0;
}

/**
 * @brief Compute low shelf biquad filter coefficients.
 * @param freq Transition frequency in Hz
 * @param Q Quality factor (shelf slope)
 * @param gainLinear Linear gain (not dB)
 * @param sampleRate Sample rate in Hz
 * @param b0 Feedforward coefficient 0 (output)
 * @param b1 Feedforward coefficient 1 (output)
 * @param b2 Feedforward coefficient 2 (output)
 * @param a1 Feedback coefficient 1 (output)
 * @param a2 Feedback coefficient 2 (output)
 */
template<typename T>
inline void computeLowshelfCoeffs(T freq, T Q, T gainLinear, T sampleRate,
                                   T& b0, T& b1, T& b2, T& a1, T& a2)
{
    T A = gainLinear;
    T w0 = two_pi<T> * freq / sampleRate;
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
 * @param freq Transition frequency in Hz
 * @param Q Quality factor (shelf slope)
 * @param gainLinear Linear gain (not dB)
 * @param sampleRate Sample rate in Hz
 * @param b0 Feedforward coefficient 0 (output)
 * @param b1 Feedforward coefficient 1 (output)
 * @param b2 Feedforward coefficient 2 (output)
 * @param a1 Feedback coefficient 1 (output)
 * @param a2 Feedback coefficient 2 (output)
 */
template<typename T>
inline void computeHighshelfCoeffs(T freq, T Q, T gainLinear, T sampleRate,
                                    T& b0, T& b1, T& b2, T& a1, T& a2)
{
    T A = gainLinear;
    T w0 = two_pi<T> * freq / sampleRate;
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

} // namespace Jonssonic
