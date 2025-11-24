// Jonssonic - A C++ audio DSP library
// Smoothed value class for parameter and control signal smoothing
// SPDX-License-Identifier: MIT


constexpr int SmoothedValueMaxOrder = 8; // Maximum allowed order for cascaded smoothing filters

#pragma once
#include <cmath>
#include <algorithm>

namespace Jonssonic {
/**
 * @brief Types of smoothing algorithms.
 */
enum class SmootherType {
    None,
    OnePole,
    Linear
};

// Forward declaration for template specializations
template<typename T, SmootherType Type, size_t Order>
class SmoothedValue;

/**
 *  @brief Type aliases for common smoothers
 */
template<typename T, size_t Order = 1>
using OnePoleSmoother = SmoothedValue<T, SmootherType::OnePole, Order>;

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
    SmoothedValue() = default;
    ~SmoothedValue() = default;

    // Resize for number of channels
    void prepare(size_t newNumChannels, T /*newTimeMs*/,T /*newSampleRate*/) {
        value.resize(newNumChannels, T(0));
    }

    // Reset values to zero
    void reset() {
        for (auto& v : value) v = T(0);
    }

    // Process (passthrough)
    T process(T target, size_t ch) {
        value[ch] = target;
        return value[ch];
    }

    // Set all channels to the same value
    SmoothedValue& operator=(const T& newValue) {
        for (auto& v : value) v = newValue;
        return *this;
    }

    // Get channel value (read/write)
    T& operator[](size_t ch) { return value[ch]; }
    const T& operator[](size_t ch) const { return value[ch]; }

    // Pass through methods to provide consistent interface
    // Set all channels to the same target value (no smoothing, just set value)
    void setTarget(T newValue) {
        for (auto& v : value) v = newValue;
    }

    // Set target value for specific channel
    void setTarget(T newValue, size_t ch) {
        value[ch] = newValue;
    }

    // Get next value for a channel (no smoothing, just return value)
    T getNextValue(size_t ch) {
        return value[ch];
    }

    // Get current value for a channel
    T getCurrentValue(size_t ch) const {
        return value[ch];
    }

    // Get target value for a channel (same as current for None)
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
    // default constructor and destructor
    SmoothedValue() = default;
    ~SmoothedValue() = default;

    // no copy semantics nor move semantics
    SmoothedValue(const SmoothedValue&) = delete;
    const SmoothedValue& operator=(const SmoothedValue&) = delete;
    SmoothedValue(SmoothedValue&&) = delete;
    const SmoothedValue& operator=(SmoothedValue&&) = delete;

    void prepare(size_t newNumChannels, T newTimeMs,T newSampleRate) {
        sampleRate = newSampleRate;
        timeMs = newTimeMs;
        current.resize(newNumChannels, T(0));
        target.resize(newNumChannels, T(0));
        stage.resize(newNumChannels);
        for (auto& s : stage)
            s.fill(T(0));
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
    T process(T target, size_t ch) {
        setTarget(target, ch);
        return getNextValue(ch);
    }

    // Force set current value on all channels
    SmoothedValue& operator=(const T& newValue) {
        for (auto& c : current) c = newValue;
        for (auto& t : target) t = newValue;
        return *this;
    }

    // Set all channels to the same target value
    void setTarget(T value) {
        for (auto& t : target)
            t = value;
    }

    // Set target value for specific channel
    void setTarget(T value, size_t ch) {
        target[ch] = value;
    }

    // Get next smoothed value for a channel
    T getNextValue(size_t ch) {
        T input = target[ch];
        for (int i = 0; i < Order; ++i) {
            stage[ch][i] += alpha * (input - stage[ch][i]);
            input = stage[ch][i];
        }
        current[ch] = stage[ch][Order-1];
        return current[ch];
    }

    // Get current value for a channel
    T getCurrentValue(size_t ch) const {
         return current[ch];
    }

    // Get target value for a channel
    T getTargetValue(size_t ch) const {
        return target[ch];
    }

private:
    void updateSmoothingParams() {
        T tau = timeMs * 0.001;
        alpha = 1 - std::exp(-1.0 / (tau * sampleRate));
    }

    T sampleRate = 44100;
    std::vector<T> current;
    std::vector<T> target;
    T timeMs = 0;
    T alpha = 0;
    std::vector<std::array<T, Order>> stage; // stage[channel][order]
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
    // default constructor and destructor
    SmoothedValue() = default;
    ~SmoothedValue() = default;

    // no copy semantics nor move semantics
    SmoothedValue(const SmoothedValue&) = delete;
    const SmoothedValue& operator=(const SmoothedValue&) = delete;
    SmoothedValue(SmoothedValue&&) = delete;
    const SmoothedValue& operator=(SmoothedValue&&) = delete;

    // Prepare for a given number of channels, sample rate, and ramp time
    void prepare(size_t newNumChannels, T newTimeMs, T newSampleRate) {
        sampleRate = newSampleRate;
        timeMs = newTimeMs;
        current.resize(newNumChannels, T(0));
        target.resize(newNumChannels, T(0));
        rampStep.resize(newNumChannels, T(0));
        rampSamples = 0;
        updateSmoothingParams();
    }

    void reset(T value = T(0)) {
        std::fill(current.begin(), current.end(), value);
        std::fill(target.begin(), target.end(), value);
        std::fill(rampStep.begin(), rampStep.end(), T(0));
        rampSamples = 0;
    }

    // Process (set target and get next value)
    T process(T target, size_t ch) {
        setTarget(target, ch);
        return getNextValue(ch);
    }

    // Force set current value on all channels
    SmoothedValue& operator=(const T& newValue) {
        for (auto& c : current) c = newValue;
        for (auto& t : target) t = newValue;
        return *this;
    }

    // Force set current value on specific channel
    SmoothedValue& operator=(const std::pair<T, size_t>& valueCh) {
        current[valueCh.second] = valueCh.first;
        return *this;
    }

    // Set all channels to the same target value
    void setTarget(T value) {
        for (size_t ch = 0; ch < target.size(); ++ch) {
            setTarget(value, ch);
        }
    }

    // Set target value for a specific channel
    void setTarget(T value, size_t ch) {
        target[ch] = value;
        rampStep[ch] = (target[ch] - current[ch]) / static_cast<T>(rampSamples > 0 ? rampSamples : 1);
    }

    // Get next value for a channel
    T getNextValue(size_t ch) {
        if (rampSamples > 0) {
            current[ch] += rampStep[ch];
            --rampSamples;
        } else {
            current[ch] = target[ch];
        }
        return current[ch];
    }

    // Get current value for a channel
    T getCurrentValue(size_t ch) const {
         return current[ch];
    }

    // Get target value for a channel
    T getTargetValue(size_t ch) const {
        return target[ch];
    }

private:
    void updateSmoothingParams() {
        rampSamples = std::max<size_t>(1, static_cast<size_t>(timeMs * 0.001 * sampleRate));
        for (size_t ch = 0; ch < rampStep.size(); ++ch) {
            rampStep[ch] = (target[ch] - current[ch]) / static_cast<T>(rampSamples);
        }
    }

    T sampleRate = 44100;
    T timeMs = 0;
    std::vector<T> current;
    std::vector<T> target;
    std::vector<T> rampStep;
    size_t rampSamples = 0;
};

} // namespace Jonssonic