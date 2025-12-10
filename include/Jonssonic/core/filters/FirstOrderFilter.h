// Jonssonic - A C++ audio DSP library
// FirstOrderFilter class header file
// SPDX-License-Identifier: MIT

#pragma once
#include "FirstOrderCore.h"
#include "FirstOrderCoeffs.h"
#include <algorithm>

namespace Jonssonic {

/**
 * @brief First-order filter types
 */
enum class FirstOrderType {
    Lowpass,
    Highpass,
    Allpass,
    Lowshelf,
    Highshelf
};

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
        freq = sampleRate / T(4); // default to quarter Nyquist
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

    void setFreq(T newFreq) {
        freq = std::clamp(newFreq, T(10), sampleRate / T(2));
        updateCoeffs();
    }

    void setType(FirstOrderType newType) {
        type = newType;
        updateCoeffs();
    }

private:
    T sampleRate = T(44100);
    T freq;
    T gain = T(1); // linear gain for shelf filters
    FirstOrderType type;
    FirstOrderCore<T> FirstOrderCore;

public:
    void setGainLinear(T newGainLin) {
        gain = std::clamp(newGainLin, T(0.001), T(10));
        updateCoeffs();
    }
    void setGainDb(T newGainDb) {
        gain = Jonssonic::dB2Mag(newGainDb);
        updateCoeffs();
    }

private:
    void updateCoeffs() {
        T b0 = T(0), b1 = T(0), a1 = T(0);
        switch (type) {
            case FirstOrderType::Lowpass:
                Jonssonic::computeFirstOrderLowpassCoeffs<T>(freq, sampleRate, b0, b1, a1);
                FirstOrderCore.setSectionCoeffs(0, b0, b1, a1);
                break;
            case FirstOrderType::Highpass:
                Jonssonic::computeFirstOrderHighpassCoeffs<T>(freq, sampleRate, b0, b1, a1);
                FirstOrderCore.setSectionCoeffs(0, b0, b1, a1);
                break;
            case FirstOrderType::Allpass:
                Jonssonic::computeFirstOrderAllpassCoeffs<T>(freq, sampleRate, b0, b1, a1);
                FirstOrderCore.setSectionCoeffs(0, b0, b1, a1);
                break;
            case FirstOrderType::Lowshelf:
                Jonssonic::computeFirstOrderLowshelfCoeffs<T>(freq, gain, sampleRate, b0, b1, a1);
                FirstOrderCore.setSectionCoeffs(0, b0, b1, a1);
                break;
            case FirstOrderType::Highshelf:
                Jonssonic::computeFirstOrderHighshelfCoeffs<T>(freq, gain, sampleRate, b0, b1, a1);
                FirstOrderCore.setSectionCoeffs(0, b0, b1, a1);
                break;
            default:
                // Handle unsupported types or add implementations
                break;
        }
    }
};

} // namespace Jonssonic
