// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Damping filter class header file
// SPDX-License-Identifier: MIT

#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <jonssonic/core/filters/biquad_filter.h>
#include <jonssonic/core/filters/one_pole_filter.h>

namespace jnsc {
/// Enumeration of supported damping filter types
enum class DampingType { OnePole, Shelf };

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
    DampingFilter(size_t newNumChannels, T newSampleRate) { prepare(newNumChannels, newSampleRate); }
    /// Default destructor.
    ~DampingFilter() = default;

    /**
     * @brief Prepare the filter for processing.
     * @param newSampleRate Sample rate in Hz.
     * @param newNumChannels Number of channels.
     */
    void prepare(size_t newNumChannels, T newSampleRate) {
        // Store config variables
        numChannels = utils::detail::clampChannels(newNumChannels);
        sampleRate = utils::detail::clampSampleRate(newSampleRate);

        // Initialize state and coefficient vectors
        z1.assign(numChannels, T(0));
        a.assign(numChannels, T(0));
        b.assign(numChannels, T(0));
        togglePrepared = true;
    }

    /**
     * @brief Set filter coefficients based on T60 at DC and Nyquist.
     * @param newT60Dc Desired decay time at DC (0 Hz)
     * @param newT60Nyq Desired decay time at Nyquist (fs/2 Hz)
     * @param newDelayTime Delay time used in the damping calculation
     * @note Coeffs are only updated if @ref prepare has been called before.
     */
    void setByT60(size_t ch, Time<T> newT60Dc, Time<T> newT60Nyq, Time<T> newDelayTime) {
        assert(ch < numChannels && "Channel index out of bounds.");
        assert(sampleRate > T(0) && "Sample rate must be set and greater than zero.");

        // Early exit if not prepared
        if (!togglePrepared)
            return;

        // Clamp T60 values
        T T60_DC = std::clamp(newT60Dc.toSeconds(sampleRate),
                              detail::DampingLimits<T>::MIN_T60,
                              detail::DampingLimits<T>::MAX_T60);
        T T60_NYQ = std::clamp(newT60Nyq.toSeconds(sampleRate),
                               detail::DampingLimits<T>::MIN_T60,
                               detail::DampingLimits<T>::MAX_T60);

        // Compute coefficients based on T60 values
        T g0 = std::pow(T(10), -3.0 * newDelayTime.toSeconds(sampleRate) / T60_DC);
        T g1 = std::pow(T(10), -3.0 * newDelayTime.toSeconds(sampleRate) / T60_NYQ);
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
// Shelf Damping Filter Specialization
// =============================================================================
/**
 * @brief DampingFilter specialization for Shelf type.
 *       Implements a biquad shelving filter parametrized
 *       by crossover frequency and T60 below and above it.
 */
template <typename T>
class DampingFilter<T, DampingType::Shelf> {
  public:
    /// Default constructor.
    DampingFilter() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels.
     * @param newSampleRate Sample rate in Hz.
     */
    DampingFilter(size_t newNumChannels, T newSampleRate) { prepare(newNumChannels, newSampleRate); }

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