// Jonssonic - A C++ audio DSP library
// FirstOrderCore header file
// SPDX-License-Identifier: MIT

#pragma once
#include "../common/AudioBuffer.h"
#include <vector>
#include <cassert>

namespace Jonssonic {
/**
 * @brief FirstOrderCore filter class implementing a multi-channel, multi-section first-order filter.
 * @param T Sample data type (e.g., float, double)
 */
template<typename T>
class FirstOrderCore {
public:
    static constexpr size_t COEFFS_PER_SECTION = 3; // b0, b1, a1
    static constexpr size_t STATE_VARS_PER_SECTION = 2; // x1, y1

    // Constructor and Destructor
    FirstOrderCore() = default;
    FirstOrderCore(size_t newNumChannels, size_t newNumSections) {
        prepare(newNumChannels, newNumSections);
    }
    ~FirstOrderCore() = default;

    // No copy or move semantics
    FirstOrderCore(const FirstOrderCore&) = delete;
    FirstOrderCore& operator=(const FirstOrderCore&) = delete;
    FirstOrderCore(FirstOrderCore&&) = delete;
    FirstOrderCore& operator=(FirstOrderCore&&) = delete;

    void prepare(size_t newNumChannels, size_t newNumSections) {
        numChannels = newNumChannels;
        numSections = newNumSections;
        coeffs.resize(numSections * COEFFS_PER_SECTION, T(0));
        state.resize(numChannels, numSections * STATE_VARS_PER_SECTION);
    }

    void clear() {
        state.clear();
    }

    T processSample(size_t ch, T input) {
        assert(ch < numChannels && "Channel index out of bounds");
        for (size_t s = 0; s < numSections; ++s) {
            size_t coeffBase = s * COEFFS_PER_SECTION;
            size_t stateBase = s * STATE_VARS_PER_SECTION;

            // Fetch coefficients
            T b0 = coeffs[coeffBase + 0];
            T b1 = coeffs[coeffBase + 1];
            T a1 = coeffs[coeffBase + 2];

            // Fetch state variables
            T x1 = state[ch][stateBase + 0];
            T y1 = state[ch][stateBase + 1];

            // Direct Form I
            T output = b0 * input + b1 * x1 - a1 * y1;

            // Update state variables
            state[ch][stateBase + 0] = input;  // x1 = input
            state[ch][stateBase + 1] = output; // y1 = output

            input = output;
        }
        return input;
    }

    void processBlock(const T* const* input, T* const* output, size_t numSamples) {
        assert(numChannels > 0 && "Filter not prepared");
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t n = 0; n < numSamples; ++n) {
                output[ch][n] = processSample(ch, input[ch][n]);
            }
        }
    }

    void setSectionCoeffs(size_t section, T b0, T b1, T a1) {
        assert(section < numSections && "Section index out of bounds");
        size_t baseIdx = section * COEFFS_PER_SECTION;
        coeffs[baseIdx + 0] = b0;
        coeffs[baseIdx + 1] = b1;
        coeffs[baseIdx + 2] = a1;
    }

    size_t getNumChannels() const { return numChannels; }
    size_t getNumSections() const { return numSections; }

private:
    size_t numChannels = 0;
    size_t numSections = 0;
    std::vector<T> coeffs; // [b0, b1, a1] per section
    AudioBuffer<T> state;  // [x1, y1] per section, per channel
};

} // namespace Jonssonic
