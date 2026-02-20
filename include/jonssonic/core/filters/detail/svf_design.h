// Jonssonic - A Modular Realtime C++ Audio DSP Library
// State variable filter design class for storing parameters of multi-channel, multi-section filters.
// SPDX-License-Identifier: MIT

#pragma once

#include "jonssonic/core/common/audio_buffer.h"
#include "jonssonic/core/common/dsp_param.h"
#include "jonssonic/core/common/quantities.h"
#include "jonssonic/core/filters/detail/filter_limits.h"
#include "jonssonic/utils/detail/config_utils.h"
#include <cmath>
#include <jonssonic/utils/math_utils.h>

namespace jnsc::detail {

/**
 * @brief SVFDesign class that stores parameters of multi-channel, multi-section state variable filters.
 * @tparam T Sample data type (e.g., float, double)
 */

template <typename T>
class SVFDesign {
  public:
    /// Enumeration of the supported filter responses
    enum class Response { Lowpass, Highpass, Bandpass, numTypes };

    /// Default constructor
    SVFDesign() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels.
     * @param newSampleRate Sample rate in Hz.
     * @param newNumSections Number of second-order sections.
     * @note Clamps the sample rate, number of channels, and sections to valid ranges to avoid unstable behavior.
     */
    SVFDesign(size_t newNumChannels, T newSampleRate, size_t newNumSections) {
        prepare(newNumChannels, newSampleRate, newNumSections);
    }

    /// Default destructor
    ~SVFDesign() = default;

    /**
     * @brief Prepare the design for a multi-channel, multi-section filter.
     * @param newNumChannels Number of channels.
     * @param newSampleRate Sample rate in Hz.
     * @param newNumSections Number of second-order sections.
     * @note Must be called before calling @ref setFrequency.
     * @note Clamps the sample rate, number of channels, and sections to valid ranges to avoid unstable behavior.
     */
    void prepare(size_t newNumChannels, T newSampleRate, size_t newNumSections) {
        // Clamp sample rate, number of channels, and sections to allowed limits
        fs = utils::detail::clampSampleRate(newSampleRate);
        numChannels = utils::detail::clampChannels(newNumChannels);
        numSections = FilterLimits<T>::clampSections(newNumSections);

        // Initialize parameters
        responseOneHot.resize(numChannels, numSections * static_cast<size_t>(Response::numTypes));
        twoR.prepare(numChannels * numSections, fs);
        g.prepare(numChannels * numSections, fs);

        // Mark as prepared
        togglePrepared = true;

        // Set default frequency and Q for all sections and channels
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t section = 0; section < numSections; ++section) {
                setResponse(ch, section, Response::Lowpass);          // Default to lowpass response
                setFrequency(ch, section, Frequency<T>::Hertz(1000)); // Default to 1 kHz
                setQ(ch, section, T(0.707));                          // Default to Butterworth Q
            }
        }
    }

    /**
     * @brief Set the control parameter smoothing time for all parameters in the design.
     * @param newTime Smoothing time struct.
     */
    void setControlSmoothingTime(Time<T> newTime) {
        twoR.setSmoothingTime(newTime);
        g.setSmoothingTime(newTime);
    }

    /**
     * @brief Set the filter response type for a specific channel and section.
     * @param ch Channel index.
     * @param section Section index.
     * @param newResponse Desired filter magnitude response type.
     * @note Updates coefficients based on the current parameters and the new response type.
     */
    void setResponse(size_t ch, size_t section, Response newResponse) {
        assert(ch < numChannels);
        assert(section < numSections);
        // Set one-hot encoding for the response type in the responseOneHot buffer
        size_t numResponseTypes = static_cast<size_t>(Response::numTypes);
        size_t baseIndex = section * numResponseTypes;
        // Set one-hot encoding for the response type in the responseOneHot buffer
        for (size_t i = 0; i < numResponseTypes; ++i)
            responseOneHot[ch][baseIndex + i] = (i == static_cast<size_t>(newResponse)) ? 1.0f : 0.0f;
    }

    /**
     * @brief Set the filter frequency for a specific channel and section.
     * @param ch Channel index.
     * @param section Section index.
     * @param newFreq Frequency struct.
     * @note Maps frequency to the g parameter used in the TPT SVF topology, where g = tan(w0/2).
     */
    void setFrequency(size_t ch, size_t section, Frequency<T> newFreq) {
        // Early exit if not prepared
        if (!togglePrepared)
            return;
        // Clamp frequency and convert to radians.
        T w0 = FilterLimits<T>::clampFrequency(newFreq, fs).toRadians(fs);

        // Map frequency to the g parameter used in the TPT SVF topology, where g = tan(w0/2).
        g.setTarget(index(ch, section), std::tan(w0 * T(0.5)));
    }

    /**
     * @brief Set quality factor (Q) for a specific channel and section.
     * @param ch Channel index.
     * @param section Section index.
     * @param newQ Quality factor.
     * @note Maps Q to store the twoR parameter used in the TPT SVF topology, where twoR = 1/Q.
     */
    void setQ(size_t ch, size_t section, T newQ) {
        twoR.setTarget(index(ch, section), T(1) / FilterLimits<T>::clampQ(newQ));
    }

    /// Get next g value for a specific channel and section.
    T getNextG(size_t ch, size_t section) { return g.getNextValue(index(ch, section)); }

    /// Get next twoR value for a specific channel and section.
    T getNextTwoR(size_t ch, size_t section) { return twoR.getNextValue(index(ch, section)); }

    /// Get the response mask for a specific channel and section.
    const T* getResponseMask(size_t ch, size_t section) const {
        size_t numResponseTypes = static_cast<size_t>(Response::numTypes);
        size_t baseIndex = section * numResponseTypes;
        return &responseOneHot[ch][baseIndex];
    }

    /// Get current sample rate
    T getSampleRate() const { return fs; }

    /// Get current filter response type (for testing purposes)
    Response getResponse(size_t ch = 0, size_t section = 0) const {
        size_t numResponseTypes = static_cast<size_t>(Response::numTypes);
        size_t baseIndex = section * numResponseTypes;
        const auto& oneHot = responseOneHot[ch];
        Response response;
        for (size_t i = 0; i < numResponseTypes; ++i) {
            if (std::abs(oneHot[baseIndex + i] - 1.0f) < 1e-6f) {
                response = static_cast<Response>(i);
                break;
            }
        }
        return response;
    }
    /// Get current frequency (for testing purposes)
    Frequency<T> getFrequency(size_t ch = 0, size_t section = 0) const {
        T w0 = T(2) * std::atan(g.getTargetValue(index(ch, section)));
        return Frequency<T>::Radians(w0);
    }
    /// Get current Q factor (for testing purposes)
    T getQ(size_t ch = 0, size_t section = 0) const { return T(1) / twoR.getTargetValue(index(ch, section)); }

  private:
    // Parameters
    bool togglePrepared = false;
    size_t numChannels = 0;
    size_t numSections = 0;
    T fs = 0;
    AudioBuffer<T> responseOneHot; // Buffer to store one-hot encoded response types for each channel and section
    DspParam<T> twoR, g; // Onepole smoothed parameters for the TPT SVF topology (twoR = 2*R, g = tan(pi*fc/fs))

    // Helper function to index the 1D parameter buffers as 2D (channel, section)
    inline size_t index(size_t ch, size_t section) const { return ch * numSections + section; }
};

} // namespace jnsc::detail