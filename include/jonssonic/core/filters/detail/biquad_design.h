// Jonssonic - A Modular Realtime C++ Audio DSP Library
// Biquad filter coefficient computation functions
// Based on Robert Bristow-Johnson's Audio EQ Cookbook
// SPDX-License-Identifier: MIT

#pragma once

#include <cmath>
#include <jonssonic/utils/math_utils.h>

namespace jnsc::detail {

template <typename T>
class BiquadDesign {
  public:
    /// Enumeration of the supported filter responses
    enum class Response { Lowpass, Highpass, Bandpass, Allpass, Notch, Peak, Lowshelf, Highshelf };

    /// Default constructor
    BiquadDesign() = default;

    /// Default destructor
    ~BiquadDesign() = default;

    /**
     * @brief Set the filter response type.
     * @param newResponse Desired filter magnitude response type.
     * @note Updates coefficients based on the current parameters and the new response type.
     */
    void setResponse(Response newResponse) {
        response = newResponse;
        updateCoeffs();
    }

    /**
     * @brief Set normalized frequency (0..0.5, where 0.5 = Nyquist).
     * @param newFreq Normalized frequency.
     * @note Updates coefficients based on the current parameters and response type.
     */
    void setFrequency(T newFreq) {
        w0 = utils::two_pi<T> * newFreq;
        updateCoeffs();
    }

    /**
     * @brief Set the gain for the filter.
     * @param newGain Linear gain.
     * @note Updates coefficients based on the current parameters and response type.
     * @note Only applicable for peaking and shelving response types.
     */
    void setGain(T newGain) {
        g = newGain;
        updateCoeffs();
    }

    /**
     * @brief Set quality factor (Q) for the filter.
     * @param newQ Quality factor.
     * @note Updates coefficients based on the current parameters and response type.
     */
    void setQ(T newQ) {
        Q = newQ;
        updateCoeffs();
    }

    /// Get B0 feedforward coefficient
    T getB0() const { return b0; }
    /// Get B1 feedforward coefficient
    T getB1() const { return b1; }
    /// Get B2 feedforward coefficient
    T getB2() const { return b2; }
    /// Get A1 feedback coefficient
    T getA1() const { return a1; }
    /// Get A2 feedback coefficient
    T getA2() const { return a2; }

  private:
    // Filter response type
    Response response;
    // Parameters
    T w0, g, Q;
    // Coefficients
    T b0, b1, b2, a1, a2;

    // Update coefficients based on current parameters and response
    void updateCoeffs() {
        switch (response) {
        case Response::Lowpass:
            makeLowpass();
            break;
        case Response::Highpass:
            makeHighpass();
            break;
        case Response::Bandpass:
            makeBandpass();
            break;
        case Response::Allpass:
            makeAllpass();
            break;
        case Response::Notch:
            makeNotch();
            break;
        case Response::Peak:
            makePeak();
            break;
        case Response::Lowshelf:
            makeLowshelf();
            break;
        case Response::Highshelf:
            makeHighshelf();
            break;
        default:
            // Handle unsupported types or add implementations
            break;
        }
    }

    // Coefficient calculation helpers
    void makeLowpass() {
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

    void makeHighpass() {
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
    void makeBandpass() {
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
    void makeAllpass() {
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

    void makeNotch() {
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

    void makePeak() {
        T A = std::sqrt(g);
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

    void makeLowshelf() {
        T A = std::sqrt(g);
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

    void makeHighshelf() {
        T A = std::sqrt(g);
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
};
} // namespace jnsc::detail
