// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Structures for representing quantities with units
// SPDX-License-Identifier: MIT
#pragma once
#include "jonssonic/utils/detail/config_utils.h"
#include <cassert>
#include <jonssonic/utils/math_utils.h>
#include <limits>

namespace jonssonic::core::common {

/**
 * @brief This file contains structures for representing quantities with units,
 * such as time, frequency, gain, etc. These will be used in setters and getters
 * throughout the library to improve code clarity and type safety and avoid multiple
 * overloads of functions for different units.
 */

/// Time unit enumeration
enum class TimeUnit { Samples, Milliseconds, Seconds };

/// Time quantity structure
template <typename T> struct Time {
  private:
    Time(T v, TimeUnit u) : value(v), unit(u) {}

  public:
    T value;
    TimeUnit unit;

    /// Factory methods for creating Time instances with specific units
    static Time Samples(T v) { return Time(std::max(v, T(0)), TimeUnit::Samples); }
    static Time Milliseconds(T v) { return Time(std::max(v, T(0)), TimeUnit::Milliseconds); }
    static Time Seconds(T v) { return Time(std::max(v, T(0)), TimeUnit::Seconds); }

    /// Convert time to samples given a sample rate
    T toSamples(T sampleRate) const {
        switch (unit) {
        case TimeUnit::Samples:
            return value;
        case TimeUnit::Milliseconds:
            return value * sampleRate * T(0.001);
        case TimeUnit::Seconds:
            return value * sampleRate;
        default:
            assert(false && "Unknown TimeUnit");
            return 0;
        }
    }
    /// Convert time to seconds given a sample rate
    T toSeconds(T sampleRate) const {
        assert(sampleRate > T(0) && "Sample rate must be positive");
        switch (unit) {
        case TimeUnit::Samples:
            return value * T(1) / sampleRate;
        case TimeUnit::Milliseconds:
            return value * T(0.001);
        case TimeUnit::Seconds:
            return value;
        default:
            assert(false && "Unknown TimeUnit");
            return T(0);
        }
    }

    /// Convert time to milliseconds given a sample rate
    T toMilliseconds(T sampleRate) const {
        assert(sampleRate > T(0) && "Sample rate must be positive");
        switch (unit) {
        case TimeUnit::Samples:
            return value * T(1000) * T(1) / sampleRate;
        case TimeUnit::Milliseconds:
            return value;
        case TimeUnit::Seconds:
            return value * T(1000);
        default:
            assert(false && "Unknown TimeUnit");
            return T(0);
        }
    }
};

/// Frequency unit enumeration
enum class FrequencyUnit { Hertz, Kilohertz, Normalized };

/// Frequency quantity structure
template <typename T> struct Frequency {
  private:
    Frequency(T v, FrequencyUnit u) : value(v), unit(u) {}

  public:
    T value;
    FrequencyUnit unit;

    /// Factory methods for creating Frequency instances with specific units
    static Frequency Hertz(T v) { return Frequency(std::max(v, T(0)), FrequencyUnit::Hertz); }
    static Frequency Kilohertz(T v) {
        return Frequency(std::max(v, T(0)), FrequencyUnit::Kilohertz);
    }
    static Frequency Normalized(T v) {
        return Frequency(std::max(v, T(0)), FrequencyUnit::Normalized);
    }

    /// Convert frequency to Hertz given a sample rate
    T toHertz(T sampleRate) const {
        switch (unit) {
        case FrequencyUnit::Hertz:
            return value;
        case FrequencyUnit::Kilohertz:
            return value * T(1000);
        case FrequencyUnit::Normalized:
            return value * sampleRate;
        default:
            assert(false && "Unknown FrequencyUnit");
            return T(0);
        }
    }
    /// Convert frequency to normalized (0..0.5) given a sample rate
    T toNormalized(T sampleRate) const {
        assert(sampleRate > T(0) && "Sample rate must be positive");
        switch (unit) {
        case FrequencyUnit::Hertz:
            return value * T(1) / sampleRate;
        case FrequencyUnit::Kilohertz:
            return value * T(1000) * T(1) / sampleRate;
        case FrequencyUnit::Normalized:
            return value;
        default:
            assert(false && "Unknown FrequencyUnit");
            return T(0);
        }
    }
    /// Convert frequency to Kilohertz given a sample rate
    T toKilohertz(T sampleRate) const {
        switch (unit) {
        case FrequencyUnit::Hertz:
            return value * T(0.001);
        case FrequencyUnit::Kilohertz:
            return value;
        case FrequencyUnit::Normalized:
            return value * sampleRate * T(0.001);
        default:
            assert(false && "Unknown FrequencyUnit");
            return T(0);
        }
    }
};

/// Gain unit enumeration
enum class GainUnit { Linear, Decibels };

/// Gain quantity structure
template <typename T> struct Gain {
  private:
    Gain(T v, GainUnit u) : value(v), unit(u) {}

  public:
    T value;
    GainUnit unit;

    /// Factory methods for creating Gain instances with specific units
    static Gain Linear(T v) {
        T sign = (v < T(0)) ? T(-1) : T(1);
        T mag = std::max(std::abs(v), std::numeric_limits<T>::epsilon());
        return Gain(sign * mag, GainUnit::Linear);
    }
    static Gain Decibels(T v) { return Gain(v, GainUnit::Decibels); }

    /// Convert gain to linear scale
    T toLinear() const {
        switch (unit) {
        case GainUnit::Linear:
            return value;
        case GainUnit::Decibels:
            return utils::dB2Mag(value);
        default:
            assert(false && "Unknown GainUnit");
            return T(1);
        }
    }
    /**
     * @brief Convert gain to decibels
     * @return Gain in decibels
     * @note Uses absolute value for linear to avoid log of negative numbers.
     *       This is relevant for phase inversion scenarios.
     */

    T toDecibels() const {
        switch (unit) {
        case GainUnit::Linear:
            return utils::mag2dB(std::abs(value));
        case GainUnit::Decibels:
            return value;
        default:
            assert(false && "Unknown GainUnit");
            return T(0);
        }
    }

    /// Unary minus operator for phase inversion or negative gain
    Gain operator-() const { return Gain(-value, unit); }
};

} // namespace jonssonic::core::common

/// Convenient type aliases directly in the jonssonic namespace
namespace jonssonic {
template <typename T> using Time = core::common::Time<T>;

template <typename T> using Frequency = core::common::Frequency<T>;

template <typename T> using Gain = core::common::Gain<T>;
} // namespace jonssonic

/// User-defined literals for ergonomic construction of quantities (float)
namespace jonssonic::literals {
// Time literals
inline Time<float> operator""_ms(long double v) {
    return Time<float>::Milliseconds(static_cast<float>(v));
}
inline Time<float> operator""_s(long double v) {
    return Time<float>::Seconds(static_cast<float>(v));
}
inline Time<float> operator""_samples(long double v) {
    return Time<float>::Samples(static_cast<float>(v));
}

// Frequency literals
inline Frequency<float> operator""_hz(long double v) {
    return Frequency<float>::Hertz(static_cast<float>(v));
}
inline Frequency<float> operator""_khz(long double v) {
    return Frequency<float>::Kilohertz(static_cast<float>(v));
}
inline Frequency<float> operator""_norm(long double v) {
    return Frequency<float>::Normalized(static_cast<float>(v));
}

// Gain literals
inline Gain<float> operator""_db(long double v) {
    return Gain<float>::Decibels(static_cast<float>(v));
}
inline Gain<float> operator""_lin(long double v) {
    return Gain<float>::Linear(static_cast<float>(v));
}
} // namespace jonssonic::literals