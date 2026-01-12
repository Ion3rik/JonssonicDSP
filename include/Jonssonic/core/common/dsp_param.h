// Jonssonic - A Modular Realtime C++ Audio DSP Library
// DspParam class with, smoothing and modulation support
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/core/common/quantities.h"
#include "jonssonic/utils/detail/config_utils.h"
#include <jonssonic/core/common/smoothed_value.h>
#include <limits>

namespace jonssonic::core::common {
/**
 * @brief DSP parameter class with smoothing and safe modulation capabilities.
 */
template <typename T, SmootherType Type = SmootherType::OnePole, int Order = 1>
class DspParam {
  public:
    /// Default constructor
    DspParam() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     */
    DspParam(size_t newNumChannels, T newSampleRate) { prepare(newNumChannels, newSampleRate); }
    /// Default destructor
    ~DspParam() = default;

    /// No copy semantics nor move semantics
    DspParam(const DspParam&) = delete;
    const DspParam& operator=(const DspParam&) = delete;
    DspParam(DspParam&&) = delete;
    const DspParam& operator=(DspParam&&) = delete;

    /**
     * @brief Prepare the parameter for processing.
     * @param newNumChannels Number of channels.
     * @param newSampleRate Sample rate in Hz.
     */
    void prepare(size_t newNumChannels, T newSampleRate) {
        smoother.prepare(newNumChannels, newSampleRate);
    }

    /// Reset the parameter smoothing state.
    void reset() { smoother.reset(); }

    /// Set smoothing time in various units via Time class (quantities.h)
    void setSmoothingTime(Time<T> newTime) { smoother.setTime(newTime); }

    /// Set parameter value bounds.
    void setBounds(T newMin, T newMax) {
        min = newMin;
        max = newMax;
    }

    /**
     * @brief Apply additive modulation: base + mod.
     * @param ch Channel index
     * @param mod Modulation value
     * @return Modulated value (clamped to bounds set in @ref setBounds).
     */
    T applyAdditiveMod(size_t ch, T mod) { return clamp(getNextValue(ch) + mod); }

    /**
     * @brief Apply multiplicative modulation: base * mod (clamped).
     * @param ch Channel index
     * @param mod Modulation value
     * @return Modulated value (clamped to bounds set in @ref setBounds).
     */
    T applyMultiplicativeMod(size_t ch, T mod) { return clamp(getNextValue(ch) * mod); }

    /**
     * @brief Set target value for all channels.
     * @param value Target value (clamped to bounds set in @ref setBounds).
     * @param skipSmoothing If true, skip smoothing and set immediately
     */
    void setTarget(T value, bool skipSmoothing = false) {
        T clampedValue = clamp(value);
        smoother.setTarget(clampedValue, skipSmoothing);
    }

    /**
     * @brief Set target value for a specific channel.
     * @param ch Channel index.
     * @param value Target value (clamped to bounds set in @ref setBounds).
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setTarget(size_t ch, T value, bool skipSmoothing = false) {
        T clampedValue = clamp(value);
        smoother.setTarget(ch, clampedValue, skipSmoothing);
    }

    /**
     * @brief Apply to buffer of samples.
     * @param buffer Audio buffer (array of pointers to channel data)
     * @param numSamples Number of samples per channel
     */
    void applyToBuffer(T* const* buffer, size_t numSamples) {
        smoother.applyToBuffer(buffer, numSamples);
    }

    /// Get next smoothed value for a channel.
    T getNextValue(size_t ch) { return smoother.getNextValue(ch); }

    /// Get current value for a channel.
    T getCurrentValue(size_t ch) const { return smoother.getCurrentValue(ch); }

    /// Get target value for a channel.
    T getTargetValue(size_t ch) const { return smoother.getTargetValue(ch); }

  private:
    SmoothedValue<T, Type, Order> smoother;
    T min = std::numeric_limits<T>::lowest();
    T max = std::numeric_limits<T>::max();

    // Clamp helper
    T clamp(T value) const { return std::clamp(value, min, max); }
};

} // namespace jonssonic::core::common