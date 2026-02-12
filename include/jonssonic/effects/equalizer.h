// Jonssonic - A C++ audio DSP library
// Equalizer effect class header file
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/utils/detail/config_utils.h"
#include <jonssonic/core/filters/biquad_filter.h>

namespace jnsc::effects {
/**
 * @brief Three-band variable Q equalizer with highpass, low and high mid peaking, and highshelf filters.
 * @param T Sample data type (e.g., float, double)
 */
template <typename T>
class Equalizer {
    /**
     * @brief Tunable constants for the equalizer effect
     * @param VARIABLE_Q_WEIGHT Weight factor for mid bands with variable Q computation.
     * @param BASE_Q Base Q factor for mid bands.
     * @param HIGH_SHELF_CUTOFF Cutoff frequency for the highshelf filter
     */
    static constexpr T VARIABLE_Q_WEIGHT = T(0.2);
    static constexpr T BASE_Q = T(1.4);
    static constexpr size_t HIGH_SHELF_CUTOFF = T(5000);

  public:
    /// Default constructor.
    Equalizer() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     */
    Equalizer(size_t newNumChannels, size_t newMaxBlockSize, T newSampleRate) {
        prepare(newNumChannels, newMaxBlockSize, newSampleRate);
    }
    /// Default destructor.
    ~Equalizer() = default;

    /// No copy nor move semantics
    Equalizer(const Equalizer&) = delete;
    Equalizer& operator=(const Equalizer&) = delete;
    Equalizer(Equalizer&&) = delete;
    Equalizer& operator=(Equalizer&&) = delete;

    /**
     * @brief Prepare the Equalizer effect for processing.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     * @param maxDelayMs Maximum Equalizer in milliseconds
     */
    void prepare(size_t newNumChannels, size_t newMaxBlockSize, T newSampleRate) {

        // Prepare biquad chain
        eq.prepare(newNumChannels, newSampleRate, 4); // 4 sections: LowCut, LowMid, HighMid, HighShelf
        eq.section(0).setResponse(BiquadFilter<T>::Response::Highpass);  // LowCut
        eq.section(1).setResponse(BiquadFilter<T>::Response::Peak);      // LowMid
        eq.section(2).setResponse(BiquadFilter<T>::Response::Peak);      // HighMid
        eq.section(3).setResponse(BiquadFilter<T>::Response::Highshelf); // HighShelf

        // Fixed parameters
        eq.section(3).setFrequency(Frequency<T>::Hertz(HIGH_SHELF_CUTOFF)); // HighShelf fixed freq
    }

    void reset() { eq.reset(); }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input buffer (numChannels x numSamples)
     * @param output Output buffer (numChannels x numSamples)
     * @param numSamples Number of samples to process
     */
    void processBlock(const T* const* input, T* const* output, size_t numSamples) {
        eq.processBlock(input, output, numSamples);
    }

    // SETTERS FOR PARAMETERS
    /**
     * @brief Set low cut frequency in Hz.
     * @param newFreqHz New low cut frequency in Hertz.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setLowCutFreq(T newFreqHz, bool /*skipSmoothing*/) {
        eq.section(0).setFrequency(Frequency<T>::Hertz(newFreqHz));
    }

    /**
     * @brief Set low mid gain in dB.
     * @param newGainDb New low mid gain in decibels.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setLowMidGainDb(T newGainDb, bool /*skipSmoothing*/) {
        eq.section(1).setGain(Gain<T>::Decibels(newGainDb));
        T newQ = computeVariableQ(newGainDb);
        eq.section(1).setQ(newQ);
    }

    /**
     * @brief Set high mid gain in dB.
     * @param newGainDb New high mid gain in decibels.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setHighMidGainDb(T newGainDb, bool /*skipSmoothing*/) {
        eq.section(2).setGain(Gain<T>::Decibels(newGainDb));
        T newQ = computeVariableQ(newGainDb);
        eq.section(2).setQ(newQ);
    }

    /**
     * @brief Set high shelf gain in dB.
     * @param newGainDb New high shelf gain in decibels.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setHighShelfGainDb(T newGainDb, bool /*skipSmoothing*/) {
        eq.section(3).setGain(Gain<T>::Decibels(newGainDb));
    }

    /**
     * @brief Set low mid frequency in Hz.
     * @param newFreqHz New low mid frequency in Hertz.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setLowMidFreq(T newFreqHz, bool /*skipSmoothing*/) {
        eq.section(1).setFrequency(Frequency<T>::Hertz(newFreqHz));
    }
    /**
     * @brief Set high mid frequency in Hz.
     * @param newFreqHz New high mid frequency in Hertz.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setHighMidFreq(T newFreqHz, bool /*skipSmoothing*/) {
        eq.section(2).setFrequency(Frequency<T>::Hertz(newFreqHz));
    }

    /// Get number of channels.
    size_t getNumChannels() const { return eq.getNumChannels(); }

    /// Get sample rate.
    T getSampleRate() const { return eq.getSampleRate(); }

  private:
    BiquadFilter<T> eq;

    /**
     * @brief Compute variable Q factor based on gain in dB.
     * @param gainDb Gain in decibels
     * @return Computed Q factor
     */
    T computeVariableQ(T gainDb) {
        if (gainDb < 0)
            return BASE_Q * (T(1) + VARIABLE_Q_WEIGHT * std::abs(gainDb));
        else
            return BASE_Q / (T(1) + VARIABLE_Q_WEIGHT * gainDb);
    }
};

} // namespace jnsc::effects