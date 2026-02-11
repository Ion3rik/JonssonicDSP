// Jonssonic - A Modular Realtime C++ Audio DSP Library
// First-order filter coefficient computation functions
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/core/common/quantities.h"
#include "jonssonic/core/filters/detail/filter_limits.h"
#include "jonssonic/utils/detail/config_utils.h"
#include <cmath>
#include <jonssonic/utils/math_utils.h>

namespace jnsc::detail {

template <typename T>
class BilinearOnePoleDesign {
  public:
    /// Enumeration of the supported filter responses
    enum class Response { Lowpass, Highpass, Allpass, Lowshelf, Highshelf };

    /// Default constructor
    BilinearOnePoleDesign() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newSampleRate Sample rate in Hz
     */
    BilinearOnePoleDesign(T newSampleRate) { prepare(newSampleRate); }

    /// Default destructor
    ~BilinearOnePoleDesign() = default;

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
        w0 = FilterLimits<T>::clampFrequency(newFreq, fs).toRadians(fs);
        updateCoeffs();
    }

    /**
     * @brief Set the gain for the filter.
     * @param newGain Gain struct.
     * @note Updates coefficients based on the current parameters and response type.
     * @note Only applicable for shelving response types.
     */
    void setGain(Gain<T> newGain) {
        g = FilterLimits<T>::clampGain(newGain).toLinear();
        updateCoeffs();
    }

    /// Get B0 feedforward coefficient
    T getB0() const { return b0; }
    /// Get B1 feedforward coefficient
    T getB1() const { return b1; }
    /// Get A1 feedback coefficient
    T getA1() const { return a1; }
    /// Get current sample rate
    T getSampleRate() const { return fs; }
    /// Get current filter response type (for testing purposes)
    const Response& getResponse() const { return response; }
    /// Get current gain (for testing purposes)
    Gain<T> getGain() const { return Gain<T>::Linear(g); }
    /// Get current normalized frequency (for testing purposes)
    Frequency<T> getFrequency() const { return Frequency<T>::Radians(w0); }

  private:
    // Parameters
    T fs = 0;
    Response response;
    T w0, g;

    // Coefficients
    T b0, b1, a1;

    // Update coefficients based on current parameters and response
    void updateCoeffs() {
        switch (response) {
        case Response::Lowpass:
            makeLowpass();
            break;
        case Response::Highpass:
            makeHighpass();
            break;
        case Response::Allpass:
            makeAllpass();
            break;
        case Response::Lowshelf:
            makeLowShelf();
            break;
        case Response::Highshelf:
            makeHighShelf();
            break;
        default:
            // Handle unsupported types or add implementations
            break;
        }
    }

    // Helper functions to calculate coefficients for different response types
    inline void makeLowpass() {
        T k = std::tan(T(0.5) * w0);
        T a0 = T(1) / (T(1) + k);
        b0 = k * a0;
        b1 = k * a0;
        a1 = (T(1) - k) * a0;
    }

    void makeHighpass() {
        T k = std::tan(T(0.5) * w0);
        T a0 = T(1) / (T(1) + k);
        b0 = a0;
        b1 = -a0;
        a1 = (T(1) - k) * a0;
    }

    void makeAllpass() {
        T k = std::tan(T(0.5) * w0);
        T coeff = (T(1) - k) / (T(1) + k);
        b0 = coeff;
        b1 = T(1);
        a1 = coeff;
    }

    void makeLowShelf() {
        T k = std::tan(T(0.5) * w0);
        T sqrt_g = std::sqrt(g);
        T a0 = k + sqrt_g;
        b0 = (g * k + sqrt_g) / a0;
        b1 = (g * k - sqrt_g) / a0;
        a1 = (k - sqrt_g) / a0;
    }

    void makeHighShelf() {
        T k = std::tan(T(0.5) * w0);
        T sqrt_g = std::sqrt(g);
        T a0 = sqrt_g * k + T(1);
        b0 = (sqrt_g * k + g) / a0;
        b1 = (sqrt_g * k - g) / a0;
        a1 = (sqrt_g * k - T(1)) / a0;
    }
};

} // namespace jnsc::detail
