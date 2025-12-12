// Jonssonic - A C++ audio DSP library
// UtilityFilters: Common utility filter wrappers and aliases
// SPDX-License-Identifier: MIT

#pragma once
#include <vector>
#include <cmath>

namespace Jonssonic {

/**
 * @brief DC Blocker - A first-order highpass filter with very low cutoff
 * @param T Sample data type (e.g., float, double)
 */

template<typename T>
class DCBlocker {
public:
    DCBlocker() = default;


    void prepare(size_t newNumChannels, T newSampleRate) {
        numChannels = newNumChannels;
        sampleRate = newSampleRate;
        // Hardcoded cutoff frequency for DC blocking
        constexpr T cutoffHz = T(5);
        if (sampleRate > T(0)) {
            R = T(1) - (T(2) * T(M_PI) * cutoffHz / sampleRate);
            if (R < T(0)) R = T(0);
            if (R > T(0.9999)) R = T(0.9999);
        }
        x1.assign(numChannels, T(0));
        y1.assign(numChannels, T(0));
    }

    void clear() {
        std::fill(x1.begin(), x1.end(), T(0));
        std::fill(y1.begin(), y1.end(), T(0));
    }

    T processSample(size_t ch, T input) {
        if (ch >= numChannels) return input;
        T y = input - x1[ch] + R * y1[ch];
        x1[ch] = input;
        y1[ch] = y;
        return y;
    }

    void processBlock(const T* const* input, T* const* output, size_t numSamples) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t n = 0; n < numSamples; ++n) {
                output[ch][n] = processSample(ch, input[ch][n]);
            }
        }
    }

private:
    size_t numChannels = 1;
    T sampleRate = T(44100);
    T R = T(0.995);
    std::vector<T> x1, y1;
};

} // namespace Jonssonic
