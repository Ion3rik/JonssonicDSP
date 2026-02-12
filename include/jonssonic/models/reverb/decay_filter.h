// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Damping filter class header file
// SPDX-License-Identifier: MIT

#pragma once

#include "jonssonic/models/reverb/detail/one_pole_decay.h"
#include "jonssonic/models/reverb/detail/shelf_decay.h"
#include <algorithm>
#include <cassert>
#include <cmath>

namespace jnsc::models {
/**
 * @brief Type aliases for decay filter types
 * @tparam T Sample data type (e.g., float, double)
 * @param Shelf1Decay First-order shelving decay filter
 * @param Shelf2Decay Second-order shelving decay filter
 */

template <typename T>
using OnePoleDecay = detail::OnePoleDecay<T>;

template <typename T>
using Shelf1Decay = ShelfDecay<T, OnePoleFilter<T>>;

template <typename T>
using Shelf2Decay = ShelfDecay<T, BiquadFilter<T>>;

/**
 * @brief DecayFilter implements a multichannel decay filter that is parametrized via T60 decay times.
 * @tparam T Sample data type (e.g., float, double)
 * @tparam DecayType Type of decay filter (e.g., OnePole, Shelf
 */
template <typename T, typename DecayType>
class DecayFilter {
  public:
    /// Default constructor.
    DecayFilter() = default;
    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels.
     * @param newSampleRate Sample rate in Hz.
     */
    DecayFilter(size_t newNumChannels, T newSampleRate) { prepare(newNumChannels, newSampleRate); }

    /// Default destructor.
    ~DecayFilter() = default;

    /// Reset the filter state
    void reset() { filter.reset(); }

    /**
     * @brief Prepare the filter for processing.
     * @param newSampleRate Sample rate in Hz.
     * @param newNumChannels Number of channels.
     */
    void prepare(size_t newNumChannels, T newSampleRate) {
        // Store config variables
        numChannels = utils::detail::clampChannels(newNumChannels);
        sampleRate = utils::detail::clampSampleRate(newSampleRate);
    }

    /**
     * @brief Process a single sample through the filter for a given channel.
     * @param ch Channel index
     * @param input Input sample
     * @return Filtered output sample
     * @note Must call @ref prepare before processing.
     */
    T processSample(size_t ch, T input) { filter.processSample(ch, input); }

    /// Get number of prepared channels
    size_t getNumChannels() const { return numChannels; }

    /// Check if the filter is prepared
    bool isPrepared() const { return filter.isPrepared(); }

    /// Get the underlying filter object (e.g., for setting coefficients)
    DecayType& engine() { return filter; }

  private:
    // Config variables
    T sampleRate = T(44100);
    size_t numChannels = 0;

    // Filter object
    DecayType filter;
};

// =============================================================================
// Shelf Damping Filter Specialization
// =============================================================================
/**
 * @brief DecayFilter specialization for Shelf type.
 *       Implements a biquad shelving filter parametrized
 *       by crossover frequency and T60 below and above it.
 */
template <typename T>
class DecayFilter<T, DecayType::Shelf> {
  public:
    /// Default constructor.
    DecayFilter() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels.
     * @param newSampleRate Sample rate in Hz.
     */
    DecayFilter(size_t newNumChannels, T newSampleRate) { prepare(newNumChannels, newSampleRate); }

    /// No copy nor move semantics
    DecayFilter(const DecayFilter&) = delete;
    DecayFilter& operator=(const DecayFilter&) = delete;
    DecayFilter(DecayFilter&&) = delete;
    DecayFilter& operator=(DecayFilter&&) = delete;

    /**
     * @brief Prepare the filter for processing.
     * @param newSampleRate Sample rate in Hz
     * @param newNumChannels Number of channels
     * @note Must be called before processing.
     */
    void prepare(size_t newNumChannels, T newSampleRate) {
        numChannels = utils::detail::clampChannels(newNumChannels);
        sampleRate = utils::detail::clampSampleRate(newSampleRate);
        shelf.prepare(newNumChannels, newSampleRate, BiquadType::Highshelf);
        gBase.resize(numChannels, T(0));
        togglePrepared = true;
    }

    /**
     * @brief Set filter coefficients based on T60 at low and high frequencies.
     * @param ch Channel index
     * @param crossOverFreq Crossover frequency between low and high shelving
     * @param newT60Low Desired decay time below crossover frequency
     * @param newT60High Desired decay time above crossover frequency
     * @param newDelayTime Delay time used in the damping calculation
     * @note Must call prepare() before this.
     */
    void
    setByT60(size_t ch, Frequency<T> newCrossOverFreq, Time<T> newT60Low, Time<T> newT60High, Time<T> newDelayTime) {
        assert(ch < numChannels && "Channel index out of bounds.");
        assert(sampleRate > T(0) && "Sample rate must be set and greater than zero.");
        // Early exit if not prepared
        if (!togglePrepared)
            return;

        // Clamp crossover frequency
        T crossOverFreqHz = std::clamp(newCrossOverFreq.toHertz(sampleRate),
                                       detail::FilterLimits<T>::MIN_FREQ_NORM * sampleRate,
                                       detail::FilterLimits<T>::MAX_FREQ_NORM * sampleRate);
        // Clamp T60 values
        T T60_Low = std::clamp(newT60Low.toSeconds(sampleRate),
                               detail::DampingLimits<T>::MIN_T60_SEC,
                               detail::DampingLimits<T>::MAX_T60_SEC);

        T T60_High = std::clamp(newT60High.toSeconds(sampleRate),
                                detail::DampingLimits<T>::MIN_T60_SEC,
                                detail::DampingLimits<T>::MAX_T60_SEC);

        // Set crossover frequency
        shelf.setFreq(newCrossOverFreq);

        // Compute gains based on T60
        T gLow = std::pow(T(10), -3.0 * newDelayTime.toSeconds(sampleRate) / T60_Low);
        T gHigh = std::pow(T(10), -3.0 * newDelayTime.toSeconds(sampleRate) / T60_High);
        // Switch shelving type based on gain relationship
        gHigh < gLow ? shelf.setType(BiquadType::Highshelf) // damp high frequencies if gHigh < gBase
                     : shelf.setType(BiquadType::Lowshelf); // damp low frequencies if gLow < gBase

        // Base gain = max of the two gains i.e. least damping
        gBase[ch] = std::max(gLow, gHigh);

        // Compute shelf gain relative to base gain
        T shelfGain = std::min(gLow, gHigh) / gBase[ch];
        shelf.setGain(Gain<T>::Linear(shelfGain));
    }

    /**
     * @brief Process a single sample through the filter for a given channel.
     * @param ch Channel index
     * @param x Input sample
     * @return Filtered output sample
     */
    T processSample(size_t ch, T x) { return shelf.processSample(ch, gBase[ch] * x); }

    /// Reset the filter state
    void reset() { shelf.reset(); }

    /// Get number of prepared channels
    size_t getNumChannels() const { return numChannels; }

    /// Check if the filter is prepared
    bool isPrepared() const { return togglePrepared; }

  private:
    bool togglePrepared = false;
    T sampleRate = T(44100);
    size_t numChannels = 0;
    std::vector<T> gBase;
    BiquadFilter<T> shelf;
};

} // namespace jnsc