// Jonssonic - A Modular Realtime C++ Audio DSP Library
// Bilinear transform-based biquad filter design class for calculating filter coefficients from parameters.
// SPDX-License-Identifier: MIT

#pragma once

#include "jonssonic/core/common/audio_buffer.h"
#include "jonssonic/core/common/quantities.h"
#include "jonssonic/core/filters/detail/filter_limits.h"
#include "jonssonic/utils/detail/config_utils.h"
#include <cmath>
#include <jonssonic/utils/math_utils.h>

namespace jnsc::detail {

/**
 * @brief BilinearBiquadDesign class that calculates biquad filter coefficients based on the bilinear transform method.
 * @tparam T Sample data type (e.g., float, double)
 */

template <typename T>
class BilinearBiquadDesign {
  public:
    /// Enumeration of the supported filter responses
    enum class Response { Lowpass, Highpass, Bandpass, Allpass, Notch, Peak, Lowshelf, Highshelf };

    /// Default constructor
    BilinearBiquadDesign() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newSampleRate Sample rate in Hz
     */
    BilinearBiquadDesign(T newSampleRate) { prepare(newSampleRate); }

    /// Default destructor
    ~BilinearBiquadDesign() = default;

    /**
     * @brief Prepare the design for a multi-channel, multi-section filter.
     * @param newNumChannels Number of channels.
     * @param newSampleRate Sample rate in Hz.
     * @param newNumSections Number of second-order sections.
     * @note Must be called before calling @ref setFrequency.
     * @note Clamps the sample rate to a valid range to avoid unstable behavior.
     */
    void prepare(size_t newNumChannels, T newSampleRate, size_t newNumSections) {
        // Store clamped sample rate.
        fs = utils::detail::clampSampleRate(newSampleRate);

        // Initialize parameter buffers
        response.resize(newNumChannels, newNumSections, Response::Lowpass);
        w0.resize(newNumChannels, newNumSections, T(utils::two_pi<T> * T(1000) / fs));
        g.resize(newNumChannels, newNumSections, T(1));
        Q.resize(newNumChannels, newNumSections, T(0.707));

        togglePrepared = true;
    }

    /**
     * @brief Compute biquad coefficients for a specific channel and section based on the current parameters.
     * @param ch Channel index.
     * @param section Section index.
     * @param b0 Output parameter for the b0 coefficient.
     * @param b1 Output parameter for the b1 coefficient.
     * @param b2 Output parameter for the b2 coefficient.
     * @param a1 Output parameter for the a1 coefficient.
     * @param a2 Output parameter for the a2 coefficient.
     */
    void computeCoeffs(size_t ch, size_t section, T& b0, T& b1, T& b2, T& a1, T& a2) {
        switch (response[ch][section]) {
        case Response::Lowpass:
            makeLowpass(ch, section, b0, b1, b2, a1, a2);
            break;
        case Response::Highpass:
            makeHighpass(ch, section, b0, b1, b2, a1, a2);
            break;
        case Response::Bandpass:
            makeBandpass(ch, section, b0, b1, b2, a1, a2);
            break;
        case Response::Allpass:
            makeAllpass(ch, section, b0, b1, b2, a1, a2);
            break;
        case Response::Notch:
            makeNotch(ch, section, b0, b1, b2, a1, a2);
            break;
        case Response::Peak:
            makePeak(ch, section, b0, b1, b2, a1, a2);
            break;
        case Response::Lowshelf:
            makeLowshelf(ch, section, b0, b1, b2, a1, a2);
            break;
        case Response::Highshelf:
            makeHighshelf(ch, section, b0, b1, b2, a1, a2);
            break;
        default:
            // Handle unsupported types or add implementations
            break;
        }
    }

    /**
     * @brief Set the filter response type for a specific channel and section.
     * @param ch Channel index.
     * @param section Section index.
     * @param newResponse Desired filter magnitude response type.
     * @note Updates coefficients based on the current parameters and the new response type.
     */
    void setResponse(size_t ch, size_t section, Response newResponse) { response[ch][section] = newResponse; }

    /**
     * @brief Set the filter frequency for a specific channel and section.
     * @param ch Channel index.
     * @param section Section index.
     * @param newFreq Frequency struct.
     * @note Updates coefficients based on the current parameters and response type.
     */
    void setFrequency(size_t ch, size_t section, Frequency<T> newFreq) {
        // Early exit if not prepared
        if (!togglePrepared)
            return;
        w0[ch][section] = FilterLimits<T>::clampFrequency(newFreq, fs).toRadians(fs);
    }

    /**
     * @brief Set the gain for a specific channel and section.
     * @param ch Channel index.
     * @param section Section index.
     * @param newGain Gain struct.
     * @note Updates coefficients based on the current parameters and response type.
     * @note Only applicable for peaking and shelving response types.
     */
    void setGain(size_t ch, size_t section, Gain<T> newGain) {
        g[ch][section] = FilterLimits<T>::clampGain(newGain).toLinear();
    }

    /**
     * @brief Set quality factor (Q) for a specific channel and section.
     * @param ch Channel index.
     * @param section Section index.
     * @param newQ Quality factor.
     * @note Updates coefficients based on the current parameters and response type.
     */
    void setQ(size_t ch, size_t section, T newQ) { Q[ch][section] = FilterLimits<T>::clampQ(newQ); }

    /// Get current sample rate
    T getSampleRate() const { return fs; }
    /// Get current filter response type (for testing purposes)
    const Response& getResponse(size_t ch = 0, size_t section = 0) const { return response[ch][section]; }
    /// Get current gain (for testing purposes)
    Gain<T> getGain(size_t ch = 0, size_t section = 0) const { return Gain<T>::Linear(g[ch][section]); }
    /// Get current normalized frequency (for testing purposes)
    Frequency<T> getFrequency(size_t ch = 0, size_t section = 0) const {
        return Frequency<T>::Radians(w0[ch][section]);
    }
    /// Get current Q factor (for testing purposes)
    T getQ(size_t ch = 0, size_t section = 0) const { return Q[ch][section]; }

  private:
    // Parameters
    bool togglePrepared = false;
    T fs = 0;
    AudioBuffer<Response> response;
    AudioBuffer<T> w0, g, Q;

    // Coefficient calculation helpers
    void makeLowpass(size_t ch, size_t section, T& b0, T& b1, T& b2, T& a1, T& a2) {
        T cosw0 = std::cos(w0[ch][section]);
        T sinw0 = std::sin(w0[ch][section]);
        T alpha = sinw0 / (T(2) * Q[ch][section]);
        T a0 = T(1) + alpha;
        b0 = ((T(1) - cosw0) / T(2)) / a0;
        b1 = (T(1) - cosw0) / a0;
        b2 = ((T(1) - cosw0) / T(2)) / a0;
        a1 = (-T(2) * cosw0) / a0;
        a2 = (T(1) - alpha) / a0;
    }

    void makeHighpass(size_t ch, size_t section, T& b0, T& b1, T& b2, T& a1, T& a2) {
        T cosw0 = std::cos(w0[ch][section]);
        T sinw0 = std::sin(w0[ch][section]);
        T alpha = sinw0 / (T(2) * Q[ch][section]);
        T a0 = T(1) + alpha;
        b0 = ((T(1) + cosw0) / T(2)) / a0;
        b1 = -(T(1) + cosw0) / a0;
        b2 = ((T(1) + cosw0) / T(2)) / a0;
        a1 = (-T(2) * cosw0) / a0;
        a2 = (T(1) - alpha) / a0;
    }
    void makeBandpass(size_t ch, size_t section, T& b0, T& b1, T& b2, T& a1, T& a2) {
        T cosw0 = std::cos(w0[ch][section]);
        T sinw0 = std::sin(w0[ch][section]);
        T alpha = sinw0 / (T(2) * Q[ch][section]);
        T a0 = T(1) + alpha;
        b0 = alpha / a0;
        b1 = T(0);
        b2 = -alpha / a0;
        a1 = (-T(2) * cosw0) / a0;
        a2 = (T(1) - alpha) / a0;
    }
    void makeAllpass(size_t ch, size_t section, T& b0, T& b1, T& b2, T& a1, T& a2) {
        T cosw0 = std::cos(w0[ch][section]);
        T sinw0 = std::sin(w0[ch][section]);
        T alpha = sinw0 / (T(2) * Q[ch][section]);
        T a0 = T(1) + alpha;
        b0 = (T(1) - alpha) / a0;
        b1 = (-T(2) * cosw0) / a0;
        b2 = (T(1) + alpha) / a0;
        a1 = (-T(2) * cosw0) / a0;
        a2 = (T(1) - alpha) / a0;
    }

    void makeNotch(size_t ch, size_t section, T& b0, T& b1, T& b2, T& a1, T& a2) {
        T cosw0 = std::cos(w0[ch][section]);
        T sinw0 = std::sin(w0[ch][section]);
        T alpha = sinw0 / (T(2) * Q[ch][section]);
        T a0 = T(1) + alpha;
        b0 = T(1) / a0;
        b1 = (-T(2) * cosw0) / a0;
        b2 = T(1) / a0;
        a1 = (-T(2) * cosw0) / a0;
        a2 = (T(1) - alpha) / a0;
    }

    void makePeak(size_t ch, size_t section, T& b0, T& b1, T& b2, T& a1, T& a2) {
        T A = std::sqrt(g[ch][section]);
        T cosw0 = std::cos(w0[ch][section]);
        T sinw0 = std::sin(w0[ch][section]);
        T alpha = sinw0 / (T(2) * Q[ch][section]);
        T a0 = T(1) + alpha / A;
        b0 = (T(1) + alpha * A) / a0;
        b1 = (-T(2) * cosw0) / a0;
        b2 = (T(1) - alpha * A) / a0;
        a1 = (-T(2) * cosw0) / a0;
        a2 = (T(1) - alpha / A) / a0;
    }

    void makeLowshelf(size_t ch, size_t section, T& b0, T& b1, T& b2, T& a1, T& a2) {
        T A = std::sqrt(g[ch][section]);
        T cosw0 = std::cos(w0[ch][section]);
        T sinw0 = std::sin(w0[ch][section]);
        T alpha = sinw0 / (T(2) * Q[ch][section]);
        T sqrtA = std::sqrt(A);
        T a0 = (A + T(1)) + (A - T(1)) * cosw0 + T(2) * sqrtA * alpha;
        b0 = (A * ((A + T(1)) - (A - T(1)) * cosw0 + T(2) * sqrtA * alpha)) / a0;
        b1 = (T(2) * A * ((A - T(1)) - (A + T(1)) * cosw0)) / a0;
        b2 = (A * ((A + T(1)) - (A - T(1)) * cosw0 - T(2) * sqrtA * alpha)) / a0;
        a1 = (-T(2) * ((A - T(1)) + (A + T(1)) * cosw0)) / a0;
        a2 = ((A + T(1)) + (A - T(1)) * cosw0 - T(2) * sqrtA * alpha) / a0;
    }

    void makeHighshelf(size_t ch, size_t section, T& b0, T& b1, T& b2, T& a1, T& a2) {
        T A = std::sqrt(g[ch][section]);
        T cosw0 = std::cos(w0[ch][section]);
        T sinw0 = std::sin(w0[ch][section]);
        T alpha = sinw0 / (T(2) * Q[ch][section]);
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
