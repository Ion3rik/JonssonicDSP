// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Structures for representing quantities with units
// SPDX-License-Identifier: MIT
#pragma once

namespace jonssonic::common {

/**
 * @brief This file contains structures for representing quantities with units,
 * such as time, frequency, gain, etc. These will be used in setters and getters
 * throughout the library to improve code clarity and type safety and avoid multiple
 * overloads of functions for different units.
 */


/// Time unit enumeration
enum class TimeUnit { Samples, Milliseconds, Seconds };

/// Time quantity structure
template<typename T>
struct Time {
	T value;
	TimeUnit unit;

    /// Factory methods for creating Time instances with specific units
	static Time Samples(T v)      { return {v, TimeUnit::Samples}; }
	static Time Milliseconds(T v) { return {v, TimeUnit::Milliseconds}; }
	static Time Seconds(T v)      { return {v, TimeUnit::Seconds}; }
};

/// Frequency unit enumeration
enum class FrequencyUnit { Hertz, Kilohertz, Normalized };

/// Frequency quantity structure
template<typename T>
struct Frequency {
    T value;
    FrequencyUnit unit;

    /// Factory methods for creating Frequency instances with specific units
    static Frequency Hertz(T v)        { return {v, FrequencyUnit::Hertz}; }
    static Frequency Kilohertz(T v)    { return {v, FrequencyUnit::Kilohertz}; }
    static Frequency Normalized(T v)   { return {v, FrequencyUnit::Normalized}; }
};

/// Gain unit enumeration
enum class GainUnit { Linear, Decibels };

/// Gain quantity structure
template<typename T>
struct Gain {
    T value;
    GainUnit unit;

    /// Factory methods for creating Gain instances with specific units
    static Gain Linear(T v)     { return {v, GainUnit::Linear}; }
    static Gain Decibels(T v)   { return {v, GainUnit::Decibels}; }
};
} // namespace jonssonic::common
