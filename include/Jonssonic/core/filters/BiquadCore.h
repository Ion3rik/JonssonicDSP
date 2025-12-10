// Jonssonic - A C++ audio DSP library
// BiquadCore header file
// SPDX-License-Identifier: MIT

#pragma once
#include "../common/AudioBuffer.h"

namespace Jonssonic
{
/**
 * @brief BiquadCore filter class implementing a multi-channel, multi-section biquad filter.
 * @param T Sample data type (e.g., float, double)
 */
template<typename T>
class BiquadCore
{
public:
    // Constants
    static constexpr size_t COEFFS_PER_SECTION = 5; // b0, b1, b2, a1, a2
    static constexpr size_t STATE_VARS_PER_SECTION = 4; // x1, x2, y1, y2

    // Constructor and Destructor
    BiquadCore() = default; // Default constructor
    BiquadCore(size_t newNumChannels, size_t newNumSections) // Parameterized constructor (for the faint hearted)
    {
        prepare(newNumChannels, newNumSections);
    }
    ~BiquadCore() = default;

    // No copy or move semantics
    BiquadCore(const BiquadCore&) = delete;
    BiquadCore& operator=(const BiquadCore&) = delete;
    BiquadCore(BiquadCore&&) = delete;
    BiquadCore& operator=(BiquadCore&&) = delete;

    /**
     * @brief Prepare the SOS filter for processing.
     * @param newNumChannels Number of channels
     * @param newNumSections Number of second-order sections
     */
    void prepare(size_t newNumChannels, size_t newNumSections)
    {
        numChannels = newNumChannels;
        numSections = newNumSections;
        coeffs.resize(numSections * COEFFS_PER_SECTION, T(0)); // 5 coefficients per section
        state.resize(numChannels, numSections * STATE_VARS_PER_SECTION); // 4 state variables per section
    }

    void clear()
    {
        state.clear();
    }

    /**
     * @brief Process a single sample for a given channel.
     * @param ch Channel index
     * @param input Input sample
     * @return Output sample
     */
    T processSample(size_t ch, T input)
    {   
        assert(ch < numChannels && "Channel index out of bounds");
        for (size_t s = 0; s < numSections; ++s)
        {
            size_t coeffBase = s * COEFFS_PER_SECTION;
            size_t stateBase = s * STATE_VARS_PER_SECTION;

            // Fetch coefficients
            T b0 = coeffs[coeffBase + 0];
            T b1 = coeffs[coeffBase + 1];
            T b2 = coeffs[coeffBase + 2];
            T a1 = coeffs[coeffBase + 3];
            T a2 = coeffs[coeffBase + 4];

            // Fetch state variables
            T x1 = state[ch][stateBase + 0];
            T x2 = state[ch][stateBase + 1];
            T y1 = state[ch][stateBase + 2];
            T y2 = state[ch][stateBase + 3];

            // Compute output sample (Direct Form I)
            T output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;

            // Update state variables
            state[ch][stateBase + 1] = x1;  // x2 = x1
            state[ch][stateBase + 0] = input;  // x1 = input
            state[ch][stateBase + 3] = y1;  // y2 = y1
            state[ch][stateBase + 2] = output;  // y1 = output

            // Output of this section becomes input to next section
            input = output;
        }

        return input;
    }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input sample pointers (one per channel)
     * @param output Output sample pointers (one per channel)
     * @param numSamples Number of samples to process
     */
    void processBlock(const T* const* input, T* const* output, size_t numSamples)
    {
        assert(numChannels > 0 && "Filter not prepared");
        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            for (size_t n = 0; n < numSamples; ++n)
            {
                output[ch][n] = processSample(ch, input[ch][n]);
            }
        }
    }

    void setSectionCoeffs(size_t section, T b0, T b1, T b2, T a1, T a2)
    {
        assert(section < numSections && "Section index out of bounds");
        size_t baseIdx = section * COEFFS_PER_SECTION;
        coeffs[baseIdx + 0] = b0;
        coeffs[baseIdx + 1] = b1;
        coeffs[baseIdx + 2] = b2;
        coeffs[baseIdx + 3] = a1;
        coeffs[baseIdx + 4] = a2;
    }

    size_t getNumChannels() const { return numChannels; }
    size_t getNumSections() const { return numSections; }

private:
    size_t numChannels = 0;
    size_t numSections = 0;

    // Coefficient layout:
    // Sections: second-order sections
    // Coefficients per section: [b0, b1, b2, a1, a2]
    // Accessing coeffs for section s:
    //   b0 = coeffs[s*5 + 0];
    //   b1 = coeffs[s*5 + 1];
    //   b2 = coeffs[s*5 + 2];
    //   a1 = coeffs[s*5 + 3];
    //   a2 = coeffs[s*5 + 4];
    std::vector<T> coeffs;

    // State buffer layout:
    // Channels: audio channels
    // Samples per channel: numSections * 4 (for x1, x2, y1, y2 per section)
    // Writing to channel ch, section s:
    //   state[ch][s*4 + 0] = x1;
    //   state[ch][s*4 + 1] = x2;
    //   state[ch][s*4 + 2] = y1;
    //   state[ch][s*4 + 3] = y2;
    // Reading from channel ch, section s:
    //   x1 = state[ch][s*4 + 0];
    //   x2 = state[ch][s*4 + 1];
    //   y1 = state[ch][s*4 + 2];
    //   y2 = state[ch][s*4 + 3];
    AudioBuffer<T> state;
};
} // namespace Jonssonic