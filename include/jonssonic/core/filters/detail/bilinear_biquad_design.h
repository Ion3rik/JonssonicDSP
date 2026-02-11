// Jonssonic - A Modular Realtime C++ Audio DSP Library
// Bilinear transform-based biquad filter design class for calculating filter coefficients from parameters.
// SPDX-License-Identifier: MIT

#pragma once

#include "jonssonic/core/common/quantities.h"
#include "jonssonic/core/filters/detail/filter_limits.h"
#include "jonssonic/utils/detail/config_utils.h"
#include <cmath>
#include <jonssonic/utils/math_utils.h>

namespace jnsc::detail {

template <typename T>
class BilinearBiquadDesign {
  public:
    /// Enumeration of the supported filter responses
    enum class Response { Lowpass, Highpass, Bandpass, Allpass, Notch, Peak, Lowshelf, Highshelf };

    /// Default constructor
    BilinearBiquadDesign() = default;

    /// Default destructor
    ~BilinearBiquadDesign() = default;

    /**
     * @brief Prepare the design for a specific sample rate.
     * @param newSampleRate Sample rate in Hz
     * @note Must be called before calling @ref setFrequency.
     * @note Clamps the sample rate to a valid range to avoid unstable behavior.
     */
    void prepare(T newSampleRate) {
        // Store clamped sample rate.
        fs = utils::detail::clampSampleRate(newSampleRate);

        // Reset parameters to defaults
        setResponse(Response::Lowpass);
        setFrequency(Frequency<T>::Hertz(T(1000)));
        setGain(Gain<T>::Linear(T(1)));
        setQ(T(0.707)); // Butterworth Q for a maximally flat response
    }

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
     * @brief Set the filter frequency.
     * @param newFreq Frequency struct.
     * @note Updates coefficients based on the current parameters and response type.
     */
    void setFrequency(Frequency<T> newFreq) {
        assert(fs > T(0) && "Sample rate must be set and positive before setting frequency");
        // Early exit if sample rate is not valid
        if (fs <= T(0))
            return;

        // Clamp frequency to valid range and convert to radians
        w0 = FilterLimits<T>::clampFrequency(newFreq, fs).toRadians(fs);

        // Call coefficient update to reflect the new frequency in the filter design
        updateCoeffs();
    }

    /**
     * @brief Set the gain for the filter.
     * @param newGain Gain struct.
     * @note Updates coefficients based on the current parameters and response type.
     * @note Only applicable for peaking and shelving response types.
     */
    void setGain(Gain<T> newGain) {
        g = FilterLimits<T>::clampGain(newGain).toLinear();
        updateCoeffs();
    }

    /**
     * @brief Set quality factor (Q) for the filter.
     * @param newQ Quality factor.
     * @note Updates coefficients based on the current parameters and response type.
     */
    void setQ(T newQ) {
        Q = FilterLimits<T>::clampQ(newQ);
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
    /// Get current sample rate
    T getSampleRate() const { return fs; }
    /// Get current filter response type (for testing purposes)
    const Response& getResponse() const { return response; }
    /// Get current gain (for testing purposes)
    Gain<T> getGain() const { return Gain<T>::Linear(g); }
    /// Get current normalized frequency (for testing purposes)
    Frequency<T> getFrequency() const { return Frequency<T>::Radians(w0); }
    /// Get current Q factor (for testing purposes)
    T getQ() const { return Q; }

  private:
    // Parameters
    T fs = 0;
    Response response;
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
