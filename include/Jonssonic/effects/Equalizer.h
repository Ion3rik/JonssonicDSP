// Jonssonic - A C++ audio DSP library
// Equalizer effect class header file
// SPDX-License-Identifier: MIT

#pragma once
#include "../core/filters/BiquadChain.h"
#include "../core/nonlinear/WaveShaper.h"

namespace Jonssonic {
/**
 * @brief Variable Q Equalizer with highpass, high and low mid peaks, and highshelf filters.
 * @param T Sample data type (e.g., float, double)
 */
template<typename T>
class Equalizer {
public:
    // Tunable constants
    static constexpr T VARIABLE_Q_WEIGHT = T(0.1); // Weighting factor for variable Q calculation (higher = more Q change per dB)
    static constexpr T BASE_Q = T(1.4);              // Base Q factor for mid bands
    static constexpr size_t HIGH_SHELF_CUTOFF = T(5000);   // High shelf fixed cutoff frequency in Hz

    // Constructor and Destructor
    Equalizer() = default;
    Equalizer(size_t newNumChannels, T newSampleRate) {
        prepare(newNumChannels, newSampleRate);
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
    void prepare(size_t newNumChannels, T newSampleRate) {

        // Store global parameters
        numChannels = newNumChannels;
        sampleRate = newSampleRate;

        // Prepare Filter Chain
        equalizer.prepare(newNumChannels, 4, newSampleRate); // 4 sections: LowCut, LowMid, HighMid, HighShelf
        equalizer.setType(0, BiquadType::Highpass); // LowCut
        equalizer.setType(1, BiquadType::Peak);     // LowMid
        equalizer.setType(2, BiquadType::Peak);     // HighMid
        equalizer.setType(3, BiquadType::Highshelf); // HighShelf

        // Fixed parameters
        equalizer.setFreq(3, HIGH_SHELF_CUTOFF); // HighShelf fixed freq

        togglePrepared = true;
    }

    void clear() {
        equalizer.clear();
    }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input buffer (numChannels x numSamples)
     * @param output Output buffer (numChannels x numSamples)
     * @param numSamples Number of samples to process
     */
    void processBlock(const T* const* input, T* const* output, size_t numSamples)
    {
        // Process EQ
        equalizer.processBlock(input, output, numSamples);

        // Apply soft clipping if enabled
        if (toggleSoftClipper)
            softClipper.processBlock(output, output, numChannels, numSamples);
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

    void setSoftClipper(bool enabled) {
        toggleSoftClipper = enabled;
    }

    // GETTERS FOR GLOBAL PARAMETERS
    size_t getNumChannels() const {
        return numChannels;
    }
    T getSampleRate() const {
        return sampleRate;
    }
    bool isPrepared() const {
        return togglePrepared;
    }

private:
    
    // GLOBAL PARAMETERS
    size_t numChannels = 0;
    T sampleRate = T(44100);
    bool togglePrepared = false;
    bool toggleSoftClipper = false;

    // PROCESSORS
    BiquadChain<T> equalizer;
    WaveShaper<T,WaveShaperType::Tanh> softClipper;


    /**
     * @brief Compute variable Q factor based on gain in dB.
     * @param gainDb Gain in decibels
     * @return Computed Q factor
     */
    T computeVariableQ(T gainDb)
    {
        if (gainDb < 0)
            return BASE_Q * (T(1) + VARIABLE_Q_WEIGHT * std::abs(gainDb));
        else
            return BASE_Q / (T(1) + VARIABLE_Q_WEIGHT * gainDb);
    }

};

} // namespace Jonssonic