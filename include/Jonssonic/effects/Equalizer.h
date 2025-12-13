// Jonssonic - A C++ audio DSP library
// Equalizer effect class header file
// SPDX-License-Identifier: MIT

#pragma once
#include "../core/filters/BiquadChain.h"

namespace Jonssonic {
/**
 * @brief Variable Q Equalizer with highpass, high and low mid peaks, and highshelf filters.
 * @param T Sample data type (e.g., float, double)
 */
template<typename T>
class Equalizer {
public:
    // Tunable constants
    static constexpr T VARIABLE_Q_WEIGHT = T(2.0); // Weighting factor for variable Q calculation

    // Constructor and Destructor
    Equalizer() = default;
    Equalizer(size_t newNumChannels, T newSampleRate, T maxDelayMs) {
        prepare(newNumChannels, newSampleRate, maxDelayMs);
    }
    ~Equalizer() = default;

    // No copy or move semantics
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
    void prepare(size_t newNumChannels, T newSampleRate, T maxDelayMs) {

        // Store global parameters
        numChannels = newNumChannels;
        sampleRate = newSampleRate;

        // Prepare Filter Chain
        equalizer.prepare(newNumChannels, 4, newSampleRate); // 4 sections: LowCut, LowMid, HighMid, HighShelf
        equalizer.setType(0, BiquadType::Highpass); // LowCut
        equalizer.setType(1, BiquadType::Peak);     // LowMid
        equalizer.setType(2, BiquadType::Peak);     // HighMid
        equalizer.setType(3, BiquadType::Highshelf); // HighShelf
    }

    void clear() {

    }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input buffer (numChannels x numSamples)
     * @param output Output buffer (numChannels x numSamples)
     * @param numSamples Number of samples to process
     */
    void processBlock(const T* const* input, T* const* output, size_t numSamples)
    {
        equalizer.processBlock(input, output, numSamples);
    }

    // SETTERS FOR PARAMETERS
    void setLowCutFreq(T newFreqHz, bool skipSmoothing) {
        equalizer.setFreq(0, newFreqHz);
        
    }
    void setLowMidGainDb(T newGainDb, bool skipSmoothing) {
        equalizer.setGainDb(1, newGainDb);
        T newQ = computeVariableQ(newGainDb);
        equalizer.setQ(1, newQ);
        
    }
    void setHighMidGainDb(T newGainDb, bool skipSmoothing) {
        equalizer.setGainDb(2, newGainDb);
        T newQ = computeVariableQ(newGainDb);
        equalizer.setQ(2, newQ);
    }
    void setHighShelfGainDb(T newGainDb, bool skipSmoothing) {
        equalizer.setGainDb(3, newGainDb);
    }
    void setLowMidFreq(T newFreqHz, bool skipSmoothing) {
        equalizer.setFreq(1, newFreqHz);
    }
    void setHighMidFreq(T newFreqHz, bool skipSmoothing) {
        equalizer.setFreq(2, newFreqHz);
    }

    // GETTERS FOR GLOBAL PARAMETERS
    size_t getNumChannels() const {
        return numChannels;
    }
    T getSampleRate() const {
        return sampleRate;
    }

private:
    
    // GLOBAL PARAMETERS
    size_t numChannels = 0;
    T sampleRate = T(44100);

    // PROCESSORS
    BiquadChain<T> equalizer;

    /**
     * @brief Compute variable Q factor based on gain in dB.
     * @param gainDb Gain in decibels
     * @return Computed Q factor
     */
    T computeVariableQ(T gainDb)
    {
        // VARIABLE_Q_WEIGHT is a tunable constant, e.g., 0.1
        T baseQ = T(0.707);
        if (gainDb < 0)
            return baseQ * (T(1) + VARIABLE_Q_WEIGHT * std::abs(gainDb));
        else
            return baseQ / (T(1) + VARIABLE_Q_WEIGHT * gainDb);
    }

};

} // namespace Jonssonic