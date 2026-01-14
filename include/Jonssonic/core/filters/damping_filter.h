// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Damping filter class header file
// SPDX-License-Identifier: MIT

#pragma once

// Internal includes
#include "detail/filter_limits.h"
#include "jonssonic/utils/detail/config_utils.h"

// Public API includes
#include <jonssonic/core/common/quantities.h>
#include <jonssonic/core/filters/biquad_filter.h>
#include <jonssonic/core/filters/filter_types.h>
#include <jonssonic/core/filters/first_order_filter.h>

// Standard library includes
#include <algorithm>
#include <cassert>
#include <cmath>

namespace jnsc {

// =============================================================================
// Template Declaration
// =============================================================================
/// DampingFilter class template
template <typename T, DampingType Type = DampingType::OnePole>
class DampingFilter;

// =============================================================================
// One-Pole Damping Filter Specialization
// =============================================================================

/**
 * @brief DampingFilter specialization for One-Pole type.
 *       Implements a one-pole lowpass filter parametrized by T60 at a frequency.
 */
template <typename T>
class DampingFilter<T, DampingType::OnePole> {
  public:
    /// Default constructor.
    DampingFilter() = default;
    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels.
     * @param newSampleRate Sample rate in Hz.
     */
    DampingFilter(size_t newNumChannels, T newSampleRate) {
        prepare(newNumChannels, newSampleRate);
    }
    /// Default destructor.
    ~DampingFilter() = default;

    /**
     * @brief Prepare the filter for processing.
     * @param newSampleRate Sample rate in Hz.
     * @param newNumChannels Number of channels.
     */
    void prepare(size_t newNumChannels, T newSampleRate) {
        numChannels = utils::detail::clampChannels(newNumChannels);
        sampleRate = utils::detail::clampSampleRate(newSampleRate);
        z1.assign(numChannels, T(0));
        a.assign(numChannels, T(0));
        b.assign(numChannels, T(0));
        togglePrepared = true;
    }

    /**
     * @brief Set filter coefficients based on T60 at DC and Nyquist.
     * @param T60_DC Desired decay time (seconds) at DC (0 Hz)
     * @param T60_NYQ Desired decay time (seconds) at Nyquist (fs/2 Hz)
     * @note Coeffs are only updated if @ref prepare has been called before.
     */
    void setByT60(size_t ch, T T60_DC, T T60_NYQ, size_t delaySamples) {
        assert(ch < numChannels && "Channel index out of bounds.");
        assert(sampleRate > T(0) && "Sample rate must be set and greater than zero.");

        // Early exit if not prepared
        if (!togglePrepared)
            return;

        // Clamp T60 values
        T60_DC = std::clamp(T60_DC,
                            detail::DampingLimits<T>::MIN_T60,
                            detail::DampingLimits<T>::MAX_T60);
        T60_NYQ = std::clamp(T60_NYQ,
                             detail::DampingLimits<T>::MIN_T60,
                             detail::DampingLimits<T>::MAX_T60);

        // Compute coefficients based on T60 values
        T g0 = std::pow(T(10), -3.0 * delaySamples / (T60_DC * sampleRate));
        T g1 = std::pow(T(10), -3.0 * delaySamples / (T60_NYQ * sampleRate));
        a[ch] = (g0 + g1) / 2;
        b[ch] = (g0 - g1) / 2;
    }

    /**
     * @brief Process a single sample through the filter for a given channel.
     * @param ch Channel index
     * @param x Input sample
     * @return Filtered output sample
     * @note Must call @ref prepare before processing.
     */
    T processSample(size_t ch, T x) {
        assert(ch < numChannels && "Channel index out of bounds");
        T y = a[ch] * x + b[ch] * z1[ch];
        z1[ch] = y;
        return y;
    }

    /// Reset the filter state
    void reset() { std::fill(z1.begin(), z1.end(), T(0)); }

    /// Get number of prepared channels
    size_t getNumChannels() const { return numChannels; }

    /// Check if the filter is prepared
    bool isPrepared() const { return togglePrepared; }

  private:
    bool togglePrepared = false;
    T sampleRate = T(44100);
    size_t numChannels = 0;
    std::vector<T> a;  // feedforward coefficient
    std::vector<T> b;  // feedback coefficient
    std::vector<T> z1; // state per channel
};

// =============================================================================
// Biquad Damping Filter Specialization
// =============================================================================
/**
 * @brief DampingFilter specialization for BiquadShelf type.
 *       Implements a biquad shelving filter parametrized
 *       by crossover frequency and T60 below and above it.
 */
template <typename T>
class DampingFilter<T, DampingType::BiquadShelf> {
  public:
    /// Default constructor.
    DampingFilter() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels.
     * @param newSampleRate Sample rate in Hz.
     */
    DampingFilter(size_t newNumChannels, T newSampleRate) {
        prepare(newNumChannels, newSampleRate);
    }

    /// No copy nor move semantics
    DampingFilter(const DampingFilter&) = delete;
    DampingFilter& operator=(const DampingFilter&) = delete;
    DampingFilter(DampingFilter&&) = delete;
    DampingFilter& operator=(DampingFilter&&) = delete;

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
     * @param crossOverFreqHz Crossover frequency in Hz between low and high shelving
     * @param T60_Low Desired decay time (seconds) below crossover frequency
     * @param T60_High Desired decay time (seconds) above crossover frequency
     * @note Must call prepare() before this.
     */
    void setByT60(size_t ch, T crossOverFreqHz, T T60_Low, T T60_High, size_t delaySamples) {
        assert(ch < numChannels && "Channel index out of bounds.");
        assert(sampleRate > T(0) && "Sample rate must be set and greater than zero.");
        // Early exit if not prepared
        if (!togglePrepared)
            return;

        // Clamp crossover frequency
        crossOverFreqHz = std::clamp(crossOverFreqHz,
                                     detail::FilterLimits<T>::MIN_FREQ_NORM * sampleRate,
                                     detail::FilterLimits<T>::MAX_FREQ_NORM * sampleRate);
        // Clamp T60 values
        T60_Low = std::clamp(T60_Low,
                             detail::DampingLimits<T>::MIN_T60,
                             detail::DampingLimits<T>::MAX_T60);

        T60_High = std::clamp(T60_High,
                              detail::DampingLimits<T>::MIN_T60,
                              detail::DampingLimits<T>::MAX_T60);

        // Set crossover frequency
        shelf.setFreq(crossOverFreqHz);

        // Compute gains based on T60
        T gLow = std::pow(T(10), -3.0 * delaySamples / (T60_Low * sampleRate));
        T gHigh = std::pow(T(10), -3.0 * delaySamples / (T60_High * sampleRate));

        // Switch shelving type based on gain relationship
        gHigh < gLow
            ? shelf.setType(BiquadType::Highshelf) // damp high frequencies if gHigh < gBase
            : shelf.setType(BiquadType::Lowshelf); // damp low frequencies if gLow < gBase

        // Base gain = max of the two gains i.e. least damping
        gBase[ch] = std::max(gLow, gHigh);

        // Compute shelf gain relative to base gain
        T shelfGain = std::min(gLow, gHigh) / gBase[ch];
        shelf.setGainLinear(shelfGain);
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

// =============================================================================
// First-Order Damping Filter Specialization
// =============================================================================
/**
 * @brief DampingFilter specialization for FirstOrderShelf type.
 *       Implements a first-order shelving filter parametrized
 *       by crossover frequency and T60 below and above it.
 */
template <typename T>
class DampingFilter<T, DampingType::FirstOrderShelf> {
  public:
    /// Default constructor.
    DampingFilter() = default;

} // namespace Jonssonic
