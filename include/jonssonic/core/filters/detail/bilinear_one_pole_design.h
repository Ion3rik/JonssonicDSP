// Jonssonic - A Modular Realtime C++ Audio DSP Library
// First-order filter coefficient computation functions
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/core/filters/detail/filter_limits.h"
#include "jonssonic/utils/detail/config_utils.h"
#include <cmath>
#include <jonssonic/core/common/audio_buffer.h>
#include <jonssonic/core/common/quantities.h>
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
     * @param newNumChannels Number of channels.
     * @param newSampleRate Sample rate in Hz.
     * @param newNumSections Number of first-order sections.
     */
    BilinearOnePoleDesign(size_t newNumChannels, T newSampleRate, size_t newNumSections) {
        prepare(newNumChannels, newSampleRate, newNumSections);
    }

    /// Default destructor
    ~BilinearOnePoleDesign() = default;

    /**
     * @brief Prepare the design for a specific sample rate.
     * @param newNumChannels Number of channels.
     * @param newSampleRate Sample rate in Hz.
     * @param newNumSections Number of first-order sections.
     * @note Must be called before calling @ref setFrequency.
     * @note Clamps the sample rate, number of channels, and sections to valid ranges to avoid unstable behavior.
     */
    void prepare(size_t newNumChannels, T newSampleRate, size_t newNumSections) {
        fs = newSampleRate;
        // Initialize parameter buffers
        response.resize(newNumChannels, newNumSections, Response::Lowpass);
        w0.resize(newNumChannels, newNumSections, T(utils::two_pi<T> * T(1000) / fs));
        g.resize(newNumChannels, newNumSections, T(1));

        // Mark as prepared
        togglePrepared = true;
    }

    /**
     * @brief Compute the filter coefficients based on the current per channel and section parameters.
     * @param ch Channel index for multi-channel designs
     * @param section Section index for multi-section designs
     * @param b0 Reference to store the computed B0 coefficient
     * @param b1 Reference to store the computed B1 coefficient
     * @param a1 Reference to store the computed A1 coefficient
     * @note Coefficients are computed based on the current response type, frequency, and gain parameters.
     */
    void computeCoeffs(size_t ch, size_t section, T& b0, T& b1, T& a1) {
        switch (response[ch][section]) {
        case Response::Lowpass:
            makeLowpass(ch, section, b0, b1, a1);
            break;
        case Response::Highpass:
            makeHighpass(ch, section, b0, b1, a1);
            break;
        case Response::Allpass:
            makeAllpass(ch, section, b0, b1, a1);
            break;
        case Response::Lowshelf:
            makeLowShelf(ch, section, b0, b1, a1);
            break;
        case Response::Highshelf:
            makeHighShelf(ch, section, b0, b1, a1);
            break;
        default:
            // Handle unsupported types or add implementations
            break;
        }
    }

    /**
     * @brief Set the filter response type.
     * @param ch Channel index.
     * @param section Section index.
     * @param newResponse Desired filter magnitude response type.
     */
    void setResponse(size_t ch, size_t section, Response newResponse) { response[ch][section] = newResponse; }

    /**
     * @brief Set the filter frequency.
     * @param ch Channel index.
     * @param section Section index.
     * @param newFreq Frequency struct.
     */
    void setFrequency(size_t ch, size_t section, Frequency<T> newFreq) {
        if (!togglePrepared)
            return;
        w0[ch][section] = FilterLimits<T>::clampFrequency(newFreq, fs).toRadians(fs);
    }

    /**
     * @brief Set the gain for the filter.
     * @param ch Channel index.
     * @param section Section index.
     * @param newGain Gain struct.
     * @note Only applicable for shelving response types.
     */
    void setGain(size_t ch, size_t section, Gain<T> newGain) {
        g[ch][section] = FilterLimits<T>::clampGain(newGain).toLinear();
    }

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
    /// Check if the design is prepared
    bool isPrepared() const { return togglePrepared; }

  private:
    // Parameters
    bool togglePrepared = false;
    T fs = 0;
    AudioBuffer<Response> response;
    AudioBuffer<T> w0, g;

    // Helper functions to calculate coefficients for different response types
    inline void makeLowpass(size_t ch, size_t section, T& b0, T& b1, T& a1) {
        T k = std::tan(T(0.5) * w0[ch][section]);
        T a0 = T(1) / (T(1) + k);
        b0 = k * a0;
        b1 = k * a0;
        a1 = (T(1) - k) * a0;
    }

    void makeHighpass(size_t ch, size_t section, T& b0, T& b1, T& a1) {
        T k = std::tan(T(0.5) * w0[ch][section]);
        T a0 = T(1) / (T(1) + k);
        b0 = a0;
        b1 = -a0;
        a1 = (T(1) - k) * a0;
    }

    void makeAllpass(size_t ch, size_t section, T& b0, T& b1, T& a1) {
        T k = std::tan(T(0.5) * w0[ch][section]);
        T coeff = (T(1) - k) / (T(1) + k);
        b0 = coeff;
        b1 = T(1);
        a1 = coeff;
    }

    void makeLowShelf(size_t ch, size_t section, T& b0, T& b1, T& a1) {
        T k = std::tan(T(0.5) * w0[ch][section]);
        T sqrt_g = std::sqrt(g[ch][section]);
        T a0 = k + sqrt_g;
        b0 = (g[ch][section] * k + sqrt_g) / a0;
        b1 = (g[ch][section] * k - sqrt_g) / a0;
        a1 = (k - sqrt_g) / a0;
    }

    void makeHighShelf(size_t ch, size_t section, T& b0, T& b1, T& a1) {
        T k = std::tan(T(0.5) * w0[ch][section]);
        T sqrt_g = std::sqrt(g[ch][section]);
        T a0 = sqrt_g * k + T(1);
        b0 = (sqrt_g * k + g[ch][section]) / a0;
        b1 = (sqrt_g * k - g[ch][section]) / a0;
        a1 = (sqrt_g * k - T(1)) / a0;
    }
};

} // namespace jnsc::detail
