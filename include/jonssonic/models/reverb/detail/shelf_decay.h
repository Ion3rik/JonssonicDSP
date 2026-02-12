// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Damping filter class header file
// SPDX-License-Identifier: MIT

#pragma once

#include "jonssonic/models/reverb/detail/decay_limits.h"
#include <jonssonic/core/filters/biquad_filter.h>
#include <jonssonic/core/filters/one_pole_filter.h>

namespace jnsc::detail {

/**
 * @brief ShelfDecay implements a first or second order shelving filter that is parametrized via T60 decay times at low
 * and high frequencies.
 * @tparam T Sample data type (e.g., float, double).
 * @tparam FilterType Type of shelving filter (OnePoleFilter or BiquadFilter).
 */
template <typename T, typename FilterType>
class ShelfDecay {
  public:
    /// Default constructor.
    ShelfDecay() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels.
     * @param newSampleRate Sample rate in Hz.
     */
    ShelfDecay(size_t newNumChannels, T newSampleRate) { prepare(newNumChannels, newSampleRate); }

    /// No copy nor move semantics
    ShelfDecay(const ShelfDecay&) = delete;
    ShelfDecay& operator=(const ShelfDecay&) = delete;
    ShelfDecay(ShelfDecay&&) = delete;
    ShelfDecay& operator=(ShelfDecay&&) = delete;

    /// Reset the filter state
    void reset() { shelf.reset(); }

    /**
     * @brief Prepare the filter for processing.
     * @param newNumChannels Number of channels.
     * @param newSampleRate Sample rate in Hz.
     * @note Must be called before processing.
     */
    void prepare(size_t newNumChannels, T newSampleRate) {
        // Clamp and store config variables
        numChannels = utils::detail::clampChannels(newNumChannels);
        sampleRate = utils::detail::clampSampleRate(newSampleRate);

        // Prepare the shelf filter and initialize gain buffer
        shelf.prepare(newNumChannels, newSampleRate);
        shelf.setResponse(Response::Lowshelf);
        gBase.resize(numChannels, T(0));

        // Mark as prepared
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
    void setDecayTimes(
        size_t ch, Frequency<T> newCrossOverFreq, Time<T> newT60Low, Time<T> newT60High, Time<T> newDelayTime) {
        assert(ch < numChannels && "Channel index out of bounds.");
        assert(sampleRate > T(0) && "Sample rate must be set and greater than zero.");
        // Early exit if not prepared
        if (!togglePrepared)
            return;

        // Set crossover frequency
        shelf.setFreq(newCrossOverFreq);

        // Clamp T60 values and convert to seconds for coefficient calculations
        T T60_Low = DecayLimits<T>::clampTime(newT60Low, sampleRate).toSeconds(sampleRate);
        T T60_High = DecayLimits<T>::clampTime(newT60High, sampleRate).toSeconds(sampleRate);

        // Compute gains based on T60
        T gLow = std::pow(T(10), -3.0 * newDelayTime.toSeconds(sampleRate) / T60_Low);
        T gHigh = std::pow(T(10), -3.0 * newDelayTime.toSeconds(sampleRate) / T60_High);

        // Switch shelving type based on gain relationship
        gHigh < gLow ? shelf.channel(ch).setResponse(Response::Highshelf) // damp high frequencies if gHigh < gBase
                     : shelf.channel(ch).setResponse(Response::Lowshelf); // damp low frequencies if gLow < gBase

        // Base gain = max of the two gains i.e. least damping
        gBase[ch] = std::max(gLow, gHigh);

        // Compute shelf gain relative to base gain
        T shelfGain = std::min(gLow, gHigh) / gBase[ch];
        shelf.channel(ch).setGain(shelfGain);
    }

    /**
     * @brief Process a single sample through the filter for a given channel.
     * @param ch Channel index
     * @param x Input sample
     * @return Filtered output sample
     */
    T processSample(size_t ch, T x) { return shelf.processSample(ch, gBase[ch] * x); }

    /// Get number of prepared channels
    size_t getNumChannels() const { return numChannels; }

    /// Check if the filter is prepared
    bool isPrepared() const { return togglePrepared; }

  private:
    bool togglePrepared = false;
    T sampleRate = T(44100);
    size_t numChannels = 0;
    std::vector<T> gBase;
    FilterType shelf;
};

} // namespace jnsc::detail