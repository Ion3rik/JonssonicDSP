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
 * @param OnePoleDecay Simple one-pole decay filter.
 * @param Shelf1Decay First-order shelving decay filter
 * @param Shelf2Decay Second-order shelving decay filter
 */

template <typename T>
using OnePoleDecay = detail::OnePoleDecay<T>;

template <typename T>
using Shelf1Decay = detail::ShelfDecay<T, OnePoleFilter<T>>;

template <typename T>
using Shelf2Decay = detail::ShelfDecay<T, BiquadFilter<T>>;

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

        // Prepare the filter engine
        filter.prepare(numChannels, sampleRate);
    }

    /**
     * @brief Process a single sample through the filter for a given channel.
     * @param ch Channel index
     * @param input Input sample
     * @return Filtered output sample
     * @note Must call @ref prepare before processing.
     */
    T processSample(size_t ch, T input) { return filter.processSample(ch, input); }

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

} // namespace jnsc::models