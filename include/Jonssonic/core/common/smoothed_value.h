// Jonssonic - A Modular Realtime C++ Audio DSP Library
// Smoothed value class for parameter and control signal smoothing
// SPDX-License-Identifier: MIT

#pragma once
#include <jonssonic/utils/detail/config_utils.h>
#include <cmath>
#include <algorithm>
#include <vector>
#include <cassert>


namespace jonssonic::core::common {
/// Smoothing algorithm types 
enum class SmootherType {
    None,
    OnePole,
    Linear
};

/**
 * @brief SmoothedValue class template for smoothing parameter changes.
 * @tparam T Data type (e.g., float, double).
 * @tparam Type Smoothing algorithm type.
 * @tparam Order Order of the smoothing filter.
 * @note Order is only applicable for OnePole smoother.
 */
template<typename T, SmootherType Type = SmootherType::OnePole, size_t Order = 1>
class SmoothedValue;
constexpr int SmoothedValueMaxOrder = 8; // Maximum allowed order for cascaded smoothing filters

/// Type aliases for common smoother types
template<typename T>
using OnePoleSmoother = SmoothedValue<T, SmootherType::OnePole, 1>;

template<typename T>
using LinearSmoother = SmoothedValue<T, SmootherType::Linear, 1>;


// =============================================================
// None specialization (passthrough)
// =============================================================
/**
 * @brief No smoothing, passthrough implementation.
 */
template<typename T, size_t Order>
class SmoothedValue<T, SmootherType::None, Order> {

public:
    /// Default constructor.
    SmoothedValue() = default;
    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels.
     * @param newSampleRate Sample rate in Hz (not used).
     */
    SmoothedValue(size_t newNumChannels, T newSampleRate) {
        prepare(newNumChannels, newSampleRate);
    }
    /// Default destructor.
    ~SmoothedValue() = default;

    /// No copy nor move semantics.
    SmoothedValue(const SmoothedValue&) = delete;
    SmoothedValue& operator=(const SmoothedValue&) = delete;
    SmoothedValue(SmoothedValue&&) = delete;
    SmoothedValue& operator=(SmoothedValue&&) = delete;

    /**
     * Prepare for a given number of channels.
     * @param newNumChannels Number of channels
     */
    void prepare(size_t newNumChannels, T) {
        value.assign(newNumChannels, T(0));
    }

    /// Reset the smoothed value (sets all channels to zero).
    void reset() {
        for (auto& v : value) v = T(0);
    }

    /// Process (passthrough no smoothing)
    T process(size_t ch, T target) {
        value[ch] = target;
        return value[ch];
    }

    /// Apply smoothed value to buffer (passthrough no smoothing)
    void applyToBuffer(T* const* buffer, size_t numSamples) {
        for (size_t ch = 0; ch < value.size(); ++ch)
            for (size_t n = 0; n < numSamples; ++n)
                buffer[ch][n] *= value[ch];
    }

    /// Set smoothing time in milliseconds (no effect for None)
    void setTimeMs(T) { }

    /// Set smoothing time in samples (no effect for None)
    void setTimeSamples(size_t) { }

    /// Set all channels to the same target value (no smoothing, just set value)
    void setTarget(T newValue, bool skipSmoothing = false) {
        for (auto& v : value) v = newValue;
    }

    /// Set target value for specific channel
    void setTarget(size_t ch, T newValue, bool skipSmoothing = false) {
        value[ch] = newValue;
    }

    /// Get next value for a channel (no smoothing, just return value)
    T getNextValue(size_t ch) {
        return value[ch];
    }

    // Get current value for a channel
    T getCurrentValue(size_t ch) const {
        return value[ch];
    }

    /// Get target value for a channel (same as current for None)
    T getTargetValue(size_t ch) const {
        return value[ch];
    }

private:
    std::vector<T> value;

};
// =============================================================
// OnePole specialization (arbitrary order)
// =============================================================
/**
 * @brief One-pole (exponential) smoothing implementation, arbitrary order.
 */
template<typename T, size_t Order>
class SmoothedValue<T, SmootherType::OnePole, Order> {
    static_assert(Order >= 1 && Order <= SmoothedValueMaxOrder, "Order must be between 1 and SmoothedValueMaxOrder (8)");
public:
    /// Default constructor
    SmoothedValue() = default;
    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     */    
    SmoothedValue(size_t newNumChannels, T newSampleRate) {
        prepare(newNumChannels, newSampleRate);
    }

    /// Default destructor.
    ~SmoothedValue() = default;

    /// No copy nor move semantics.
    SmoothedValue(const SmoothedValue&) = delete;
    const SmoothedValue& operator=(const SmoothedValue&) = delete;
    SmoothedValue(SmoothedValue&&) = delete;
    const SmoothedValue& operator=(SmoothedValue&&) = delete;

    /**
     * @brief Prepare the smoother for processing.
     * @param newNumChannels Number of channels.
     * @param newSampleRate Sample rate in Hz.
     */
    void prepare(size_t newNumChannels, T newSampleRate) {
        sampleRate = utils::detail::clampSampleRate(newSampleRate);
        numChannels = utils::detail::clampChannels(newNumChannels);
        current.assign(numChannels, T(0));
        target.assign(numChannels, T(0));
        stage.resize(numChannels);
        for (auto& s : stage)
            s.fill(T(0));
        togglePrepared = true;
    }

    void setTimeMs(T newTimeMs) {
        timeMs = newTimeMs;
        updateSmoothingParams();
    }

    void setTimeSamples(size_t timeSamples) {
        assert(sampleRate > T(0));
        timeMs = (T(timeSamples) / sampleRate) * T(1000);
        updateSmoothingParams();
    }

    void reset() {
        for (auto& s : stage)
            s.fill(T(0));
        for (size_t ch = 0; ch < current.size(); ++ch) {
            current[ch] = target[ch] = T(0);
        }
    }

    // Process (set target and get next value)
    T process(size_t ch, T target) {
        setTarget(ch, target);
        return getNextValue(ch);
    }

    // Apply smoothed value to buffer
    void applyToBuffer(T* const* buffer, size_t numSamples) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t n = 0; n < numSamples; ++n) {
                buffer[ch][n] *= getNextValue(ch);
            }
        }
    }

    // Set all channels to the same target value
    void setTarget(T value, bool skipSmoothing = false) {
        if (skipSmoothing) {
            for (size_t ch = 0; ch < numChannels; ++ch) {
                current[ch] = value;
                target[ch] = value;
                for (int i = 0; i < Order; ++i) {
                    stage[ch][i] = value;
                }
            }
        } else {
            for (auto& t : target)
                t = value;
        }
    }

    /**
     * @brief Set target value for a specific channel.
     * @param ch Channel index
     * @param value Target value
     * @param skipSmoothing If true, sets current value directly to target
     */
    void setTarget(size_t ch, T value, bool skipSmoothing = false) {
        if (skipSmoothing) {
            current[ch] = value;
            target[ch] = value;
            for (int i = 0; i < Order; ++i) {
                stage[ch][i] = value;
            }
        } else {
            target[ch] = value;
        }
    }

    /// Get next smoothed value for a channel and advance the state
    T getNextValue(size_t ch) {
        T input = target[ch];
        for (int i = 0; i < Order; ++i) {
            stage[ch][i] += coeff * (input - stage[ch][i]);
            input = stage[ch][i];
        }
        current[ch] = stage[ch][Order-1];
        return current[ch];
    }

    /// Get current value for a channel
    T getCurrentValue(size_t ch) const {
         return current[ch];
    }

    /// Get target value for a channel
    T getTargetValue(size_t ch) const {
        return target[ch];
    }

private:
    bool togglePrepared = false;
    T sampleRate = 44100;
    size_t numChannels = 0;
    std::vector<T> current;
    std::vector<T> target;
    T timeMs = 10;
    T coeff = 0;
    std::vector<std::array<T, Order>> stage; // stage[channel][order]

    void updateSmoothingParams() {
        // Early exit if not prepared
        if (!togglePrepared) 
            return;
        // Calculate coeff for one-pole smoothing
        T tau = timeMs * 0.001;
        coeff = 1 - std::exp(-1.0 / (tau * sampleRate));
    }
};


// =============================================================
// Linear specialization
// =============================================================
/**
 * @brief Linear ramp smoothing implementation.
 */
template<typename T, size_t Order>
class SmoothedValue<T, SmootherType::Linear, Order> {
public:
    /// Default constructor
    SmoothedValue() = default;
    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     */    
    SmoothedValue(size_t newNumChannels, T newSampleRate) {
        prepare(newNumChannels, newSampleRate);
    }

    /// Default destructor
    ~SmoothedValue() = default;

    /// No copy semantics nor move semantics
    SmoothedValue(const SmoothedValue&) = delete;
    const SmoothedValue& operator=(const SmoothedValue&) = delete;
    SmoothedValue(SmoothedValue&&) = delete;
    const SmoothedValue& operator=(SmoothedValue&&) = delete;

    /**
     * @brief Prepare the smoother for processing.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     */
    void prepare(size_t newNumChannels, T newSampleRate) {
        sampleRate = utils::detail::clampSampleRate(newSampleRate);
        numChannels = utils::detail::clampChannels(newNumChannels);
        current.assign(numChannels, T(0));
        target.assign(numChannels, T(0));
        rampStep.assign(numChannels, T(0));
        rampSamples = 0;
    }

    /// Set smoothing time in milliseconds
    void setTimeMs(T newTimeMs) {
        timeMs = newTimeMs;
        updateSmoothingParams();
    }

    /// Set smoothing time in samples
    void setTimeSamples(size_t timeSamples) {
        assert(sampleRate > T(0));
        timeMs = (T(timeSamples) / sampleRate) * T(1000);
        updateSmoothingParams();
    }

    /// Reset the smoothed value
    void reset(T value = T(0)) {
        std::fill(current.begin(), current.end(), value);
        std::fill(target.begin(), target.end(), value);
        std::fill(rampStep.begin(), rampStep.end(), T(0));
        rampSamples = 0;
    }

    /**
     * @brief Process (set target and get next value).
     * @param ch Channel index
     * @param target Target value
     * @return Next smoothed value
     */
    T process(size_t ch, T target) {
        setTarget(ch, target);
        return getNextValue(ch);
    }

    /**
     * @brief Apply the smoothed value to a buffer.
     * @param buffer Audio buffer (array of pointers to channel data)
     * @param numSamples Number of samples per channel
     */
    void applyToBuffer(T* const* buffer, size_t numSamples) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t n = 0; n < numSamples; ++n) {
                buffer[ch][n] *= getNextValue(ch);
            }
        }
    }

    /**
     * @brief Set all channels to the same target value.
     * @param value Target value
     * @param skipSmoothing If true, sets current value directly to target
     */
    void setTarget(T value, bool skipSmoothing = false) {
        if (skipSmoothing) {
            for (size_t ch = 0; ch < numChannels; ++ch) {
                current[ch] = value;
                target[ch] = value;
                rampStep[ch] = T(0);
            }
        } else {
            for (size_t ch = 0; ch < target.size(); ++ch) {
                setTarget(ch, value, false);
            }
        }
    }

    /**
     * @brief Set target value for a specific channel.
     * @param ch Channel index
     * @param value Target value
     * @param skipSmoothing If true, sets current value directly to target
     */
    void setTarget(size_t ch, T value, bool skipSmoothing = false) {
        if (skipSmoothing) {
            current[ch] = value;
            target[ch] = value;
            rampStep[ch] = T(0);
        } else {
            target[ch] = value;
            rampStep[ch] = (target[ch] - current[ch]) / static_cast<T>(rampSamples > 0 ? rampSamples : 1);
        }
    }

    /// Get next value for a channel
    T getNextValue(size_t ch) {
        if (rampSamples > 0) {
            current[ch] += rampStep[ch];
            --rampSamples;
        } else {
            current[ch] = target[ch];
        }
        return current[ch];
    }

    /// Get current value for a channel
    T getCurrentValue(size_t ch) const {
         return current[ch];
    }

    // Get target value for a channel
    T getTargetValue(size_t ch) const {
        return target[ch];
    }

private:

    T sampleRate = 44100;
    size_t numChannels = 0;
    T timeMs = 10;
    std::vector<T> current;
    std::vector<T> target;
    std::vector<T> rampStep;
    size_t rampSamples = 0;

    void updateSmoothingParams() {
        rampSamples = std::max<size_t>(1, static_cast<size_t>(timeMs * 0.001 * sampleRate));
        for (size_t ch = 0; ch < numChannels; ++ch) {
            rampStep[ch] = (target[ch] - current[ch]) / static_cast<T>(rampSamples);
        }
    }
};

} // namespace jonssonic::common