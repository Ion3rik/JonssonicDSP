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


// =============================================================
// Shared data struct for composition
// =============================================================
/** 
 * @brief Data structure to hold shared parameters and state for smoothed values.
 */
template<typename T>
struct SmoothedValueData {
    SmoothedValueData(T sampleRate = 44100, T timeMs = 10)
        : sampleRate(sampleRate), timeMs(timeMs), current(0), target(0) {}

    T sampleRate;
    T timeMs;
    T current;
    T target;
};

// Forward declaration for template specializations
template<typename T, SmootherType Type, int Order = 1>
class SmoothedValue;


// =============================================================
// None specialization (passthrough)
// =============================================================
/**
 * @brief No smoothing, passthrough implementation.
 */
template<typename T, int Order>
class SmoothedValue<T, SmootherType::None, Order> {
    static_assert(Order == 1, "None smoothing only supports Order == 1");
public:
    SmoothedValue() = default;
    void prepare(T, T) {}
    void setSampleRate(T) {}
    void setTimeMs(T) {}
    void reset(T value = T(0)) { current = target = value; }
    void setTarget(T value) { target = value; }
    T getNextValue() { current = target; return current; }
    T getCurrentValue() const { return current; }
    T getTargetValue() const { return target; }
private:
    T current = T(0);
    T target = T(0);
};
// =============================================================
// OnePole specialization (arbitrary order)
// =============================================================
/**
 * @brief One-pole (exponential) smoothing implementation, arbitrary order.
 */
template<typename T, int Order>
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

    void prepare(T sampleRate, T timeMs) {
        data.sampleRate = sampleRate;
        data.timeMs = timeMs;
        updateSmoothingParams();
    }

    void setSampleRate(T newSampleRate) {
        data.sampleRate = static_cast<T>(newSampleRate);
        updateSmoothingParams();
    }

    void setTimeMs(T newTimeMs) {
        data.timeMs = static_cast<T>(newTimeMs);
        updateSmoothingParams();
    }

    void reset(T value = T(0)) {
        for (int i = 0; i < Order; ++i)
            stage[i] = static_cast<T>(value);
        data.current = data.target = static_cast<T>(value);
    }

    void setTarget(T value) {
        data.target = static_cast<T>(value);
    }

    T getNextValue() {
        T input = data.target;
        for (int i = 0; i < Order; ++i) {
            stage[i] += alpha * (input - stage[i]);
            input = stage[i];
        }
        data.current = stage[Order-1];
        return data.current;
    }

    T getCurrentValue() const { return data.current; }
    T getTargetValue() const { return data.target; }

private:
    void updateSmoothingParams() {
        T tau = data.timeMs * 0.001;
        alpha = 1 - std::exp(-1.0 / (tau * data.sampleRate));
    }

    SmoothedValueData<T> data;
    T alpha = 0;
    T stage[Order] = {};
};


// =============================================================
// Linear specialization
// =============================================================
/**
 * @brief Linear ramp smoothing implementation.
 */
template<typename T, int Order>
class SmoothedValue<T, SmootherType::Linear, Order> {
    static_assert(Order == 1, "Linear smoothing only supports Order == 1");
public:
    // default constructor and destructor
    SmoothedValue() = default;
    ~SmoothedValue() = default;

    // no copy semantics nor move semantics
    SmoothedValue(const SmoothedValue&) = delete;
    const SmoothedValue& operator=(const SmoothedValue&) = delete;
    SmoothedValue(SmoothedValue&&) = delete;
    const SmoothedValue& operator=(SmoothedValue&&) = delete;

    void prepare(T sampleRate, T timeMs) {
        data.sampleRate = sampleRate;
        data.timeMs = timeMs;
        updateSmoothingParams();
    }

    void setSampleRate(T newSampleRate) {
        data.sampleRate = static_cast<T>(newSampleRate);
        updateSmoothingParams();
    }

    void setTimeMs(T newTimeMs) {
        data.timeMs = static_cast<T>(newTimeMs);
        updateSmoothingParams();
    }

    void reset(T value = T(0)) {
        data.current = data.target = static_cast<T>(value);
        rampStep = T(0);
        rampSamples = 0;
    }

    void setTarget(T value) {
        data.target = static_cast<T>(value);
        rampStep = (data.target - data.current) / rampSamples;
    }

    T getNextValue() {
        if (rampSamples > 0) {
            data.current += rampStep;
            --rampSamples;
        } else {
            data.current = data.target;
        }
        return data.current;
    }

    T getCurrentValue() const { return data.current; }
    T getTargetValue() const { return data.target; }

private:
    void updateSmoothingParams() {
        rampSamples = std::max(1, static_cast<int>(data.timeMs * 0.001 * data.sampleRate));
    }

    SmoothedValueData<T> data;
    T rampStep = 0;
    int rampSamples = 0;
};

} // namespace Jonssonic