// Jonssonic - A C++ audio DSP library
// FirstOrderFilter class header file
// SPDX-License-Identifier: MIT

#pragma once
#include "FilterTypes.h"
#include "FirstOrderCore.h"
#include "FirstOrderCoeffs.h"
#include <algorithm>

namespace Jonssonic {

/**
 * @brief First-order filter wrapper
 * @param T Sample data type (e.g., float, double)
 */
template<typename T>
class FirstOrderFilter {
public:
    // Constructor and Destructor
    FirstOrderFilter() = default;
    FirstOrderFilter(size_t newNumChannels, T newSampleRate) {
        prepare(newNumChannels, newSampleRate);
    }
    ~FirstOrderFilter() = default;

    // No copy or move semantics
    FirstOrderFilter(const FirstOrderFilter&) = delete;
    FirstOrderFilter& operator=(const FirstOrderFilter&) = delete;
    FirstOrderFilter(FirstOrderFilter&&) = delete;
    FirstOrderFilter& operator=(FirstOrderFilter&&) = delete;

    void prepare(size_t newNumChannels, T newSampleRate) {
        sampleRate = newSampleRate;
        type = FirstOrderType::Lowpass; // default to lowpass
        FirstOrderCore.prepare(newNumChannels, 1); // single section
        updateCoeffs();
    }

    void clear() {
        FirstOrderCore.clear();
    }

    T processSample(size_t ch, T input) {
        return FirstOrderCore.processSample(ch, input);
    }

    void processBlock(const T* const* input, T* const* output, size_t numSamples) {
        FirstOrderCore.processBlock(input, output, numSamples);
    }

    /**
     * @brief Set the cutoff frequency in Hz
     * @param newFreq Cutoff frequency in Hz
     */
    void setFreq(T newFreqHz) {
        assert(sampleRate > T(0) && "Sample rate must be set before setting frequency");
        // Convert Hz to normalized frequency (0..0.5)
        freqNormalized = std::clamp(newFreqHz / sampleRate, T(0), T(0.5));
        updateCoeffs();
    }

    /**
     * @brief Set the cutoff frequency as a normalized value (0..0.5, where 0.5 = Nyquist)
     *        This allows setting the cutoff before the sample rate is known. The actual frequency will be set on prepare().
     * @param normalizedFreq Normalized frequency (0..0.5)
     */
    void setFreqNormalized(T normalizedFreq) {
        freqNormalized = std::clamp(normalizedFreq, T(0), T(0.5));
        updateCoeffs();
    }

    void setType(FirstOrderType newType) {
        type = newType;
        updateCoeffs();
    }

    void setGainLinear(T newGainLin) {
        gain = std::clamp(newGainLin, T(0.001), T(10));
        updateCoeffs();
    }

    void setGainDb(T newGainDb) {
        gain = Jonssonic::dB2Mag(newGainDb);
        updateCoeffs();
    }

private:
    T sampleRate = T(44100);
    T freqNormalized = T(0.25); // default to quarter Nyquist
    T gain = T(1); // linear gain for shelf filters

    FirstOrderType type;
    FirstOrderCore<T> FirstOrderCore;

    void updateCoeffs() {
        // Skip if filter not prepared (numSections == 0)
        if (FirstOrderCore.getNumSections() == 0) return;
        
        T b0 = T(0), b1 = T(0), a1 = T(0);
        switch (type) {
            case FirstOrderType::Lowpass:
                Jonssonic::computeFirstOrderLowpassCoeffs<T>(freqNormalized, b0, b1, a1);
                FirstOrderCore.setSectionCoeffs(0, b0, b1, a1);
                break;
            case FirstOrderType::Highpass:
                Jonssonic::computeFirstOrderHighpassCoeffs<T>(freqNormalized, b0, b1, a1);
                FirstOrderCore.setSectionCoeffs(0, b0, b1, a1);
                break;
            case FirstOrderType::Allpass:
                Jonssonic::computeFirstOrderAllpassCoeffs<T>(freqNormalized, b0, b1, a1);
                FirstOrderCore.setSectionCoeffs(0, b0, b1, a1);
                break;
            case FirstOrderType::Lowshelf:
                Jonssonic::computeFirstOrderLowshelfCoeffs<T>(freqNormalized, gain, b0, b1, a1);
                FirstOrderCore.setSectionCoeffs(0, b0, b1, a1);
                break;
            case FirstOrderType::Highshelf:
                Jonssonic::computeFirstOrderHighshelfCoeffs<T>(freqNormalized, gain, b0, b1, a1);
                FirstOrderCore.setSectionCoeffs(0, b0, b1, a1);
                break;
            default:
                // Handle unsupported types or add implementations
                break;
        }
    }
};


} // namespace Jonssonic
