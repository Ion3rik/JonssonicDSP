// Jonssonic - A C++ audio DSP library
// Equalizer effect class header file
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/utils/detail/config_utils.h"
#include <jonssonic/core/common/audio_buffer.h>
#include <jonssonic/core/common/quantities.h>
#include <jonssonic/core/filters/biquad_chain.h>
#include <jonssonic/core/nonlinear/wave_shaper.h>
#include <jonssonic/core/oversampling/oversampler.h>
#include <jonssonic/utils/buffer_utils.h>

namespace jnsc::effects {
/**
 * @brief Variable Q Equalizer with highpass, high and low mid peaks, and highshelf filters.
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

        // Store config variables
        numChannels = utils::detail::clampChannels(newNumChannels);
        sampleRate = utils::detail::clampSampleRate(newSampleRate);

        // Prepare biquad chain
        eq.prepare(numChannels, 4, sampleRate); // 4 sections: LowCut, LowMid, HighMid, HighShelf
        eq.setType(0, BiquadType::Highpass);    // LowCut
        eq.setType(1, BiquadType::Peak);        // LowMid
        eq.setType(2, BiquadType::Peak);        // HighMid
        eq.setType(3, BiquadType::Highshelf);   // HighShelf

        // Fixed parameters
        eq.setFreq(3, Frequency<T>::Hertz(HIGH_SHELF_CUTOFF)); // HighShelf fixed freq
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
        eq.setFreq(0, Frequency<T>::Hertz(newFreqHz));
    }

    /**
     * @brief Set low mid gain in dB.
     * @param newGainDb New low mid gain in decibels.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setLowMidGainDb(T newGainDb, bool /*skipSmoothing*/) {
        eq.setGain(1, Gain<T>::Decibels(newGainDb));
        T newQ = computeVariableQ(newGainDb);
        eq.setQ(1, newQ);
    }

    /**
     * @brief Set high mid gain in dB.
     * @param newGainDb New high mid gain in decibels.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setHighMidGainDb(T newGainDb, bool /*skipSmoothing*/) {
        eq.setGain(2, Gain<T>::Decibels(newGainDb));
        T newQ = computeVariableQ(newGainDb);
        eq.setQ(2, newQ);
    }

    /**
     * @brief Set high shelf gain in dB.
     * @param newGainDb New high shelf gain in decibels.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setHighShelfGainDb(T newGainDb, bool /*skipSmoothing*/) {
        eq.setGain(3, Gain<T>::Decibels(newGainDb));
    }

    /**
     * @brief Set low mid frequency in Hz.
     * @param newFreqHz New low mid frequency in Hertz.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setLowMidFreq(T newFreqHz, bool /*skipSmoothing*/) {
        eq.setFreq(1, Frequency<T>::Hertz(newFreqHz));
    }
    /**
     * @brief Set high mid frequency in Hz.
     * @param newFreqHz New high mid frequency in Hertz.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setHighMidFreq(T newFreqHz, bool /*skipSmoothing*/) {
        eq.setFreq(2, Frequency<T>::Hertz(newFreqHz));
    }

    /// Get number of channels.
    size_t getNumChannels() const { return numChannels; }

    /// Get sample rate.
    T getSampleRate() const { return sampleRate; }

  private:
    // CONFIG VARIABLES
    size_t numChannels = 0;
    T sampleRate = T(44100);

    // PROCESSORS
    BiquadChain<T> eq;
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