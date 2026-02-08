// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// DF1Engine header file
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/utils/detail/config_utils.h"
#include <jonssonic/core/common/audio_buffer.h>

namespace jnsc::detail {
/**
 * @brief DF1Engine filter class implementing a multi-channel, multi-section biquad filter in Direct Form I.
 * @param T Sample data type (e.g., float, double)
 */
template <typename T>
class DF1Engine {
  public:
    /// Constexprs for coefficient and state variable counts
    static constexpr size_t COEFFS_PER_SECTION = 5;     // b0, b1, b2, a1, a2
    static constexpr size_t STATE_VARS_PER_SECTION = 4; // x1, x2, y1, y2

    /// Default constructor
    DF1Engine() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels
     * @param newNumSections Number of second-order sections
     */
    DF1Engine(size_t newNumChannels, size_t newNumSections) { prepare(newNumChannels, newNumSections); }

    /// Default destructor
    ~DF1Engine() = default;

    /// No copy nor move semantics
    DF1Engine(const DF1Engine&) = delete;
    DF1Engine& operator=(const DF1Engine&) = delete;
    DF1Engine(DF1Engine&&) = delete;
    DF1Engine& operator=(DF1Engine&&) = delete;

    /**
     * @brief Prepare the filter for processing.
     * @param newNumChannels Number of channels
     * @param newNumSections Number of second-order sections
     */
    void prepare(size_t newNumChannels, size_t newNumSections) {
        // Clamp channels and sections to allowed limits
        numChannels = utils::detail::clampChannels(newNumChannels);
        numSections = detail::clampSections(newNumSections);

        // Allocate coefficient and state buffers
        coeffs.resize(numChannels, numSections * COEFFS_PER_SECTION);
        state.resize(numChannels, numSections * STATE_VARS_PER_SECTION);

        // Mark as prepared
        togglePrepared = true;
    }

    /// Reset the filter state
    void reset() { state.clear(); }

    /**
     * @brief Process a single sample for a given channel.
     * @param ch Channel index
     * @param input Input sample
     * @return Output sample
     * @note Must call @ref prepare before processing.
     */
    T processSample(size_t ch, T input) {
        for (size_t s = 0; s < numSections; ++s) {
            size_t coeffBase = s * COEFFS_PER_SECTION;
            size_t stateBase = s * STATE_VARS_PER_SECTION;

            // Fetch coefficients
            T b0 = coeffs[ch][coeffBase + 0];
            T b1 = coeffs[ch][coeffBase + 1];
            T b2 = coeffs[ch][coeffBase + 2];
            T a1 = coeffs[ch][coeffBase + 3];
            T a2 = coeffs[ch][coeffBase + 4];
            // Fetch state variables
            T x1 = state[ch][stateBase + 0];
            T x2 = state[ch][stateBase + 1];
            T y1 = state[ch][stateBase + 2];
            T y2 = state[ch][stateBase + 3];

            // Compute output sample (Direct Form I)
            T output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;

            // Update state variables
            state[ch][stateBase + 1] = x1;     // x2 = x1
            state[ch][stateBase + 0] = input;  // x1 = input
            state[ch][stateBase + 3] = y1;     // y2 = y1
            state[ch][stateBase + 2] = output; // y1 = output

            // Input for next section
            input = output;
        }

        return input;
    }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input sample pointers (one per channel)
     * @param output Output sample pointers (one per channel)
     * @param numSamples Number of samples to process
     * @note Must call @ref prepare before processing.
     */
    void processBlock(const T* const* input, T* const* output, size_t numSamples) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t n = 0; n < numSamples; ++n) {
                output[ch][n] = processSample(ch, input[ch][n]);
            }
        }
    }

    /**
     * @brief Set the same coefficients for all channels for a specific section.
     * @param section Section index
     * @param b0 Feedforward coefficient 0
     * @param b1 Feedforward coefficient 1
     * @param b2 Feedforward coefficient 2
     * @param a1 Feedback coefficient 1
     * @param a2 Feedback coefficient 2
     * @note Must call @ref prepare before setting coefficients.
     */
    void setSectionCoeffs(size_t section, T b0, T b1, T b2, T a1, T a2) {
        // Early exit if not prepared
        if (!togglePrepared)
            return;
        assert(section < numSections && "Section index out of bounds");

        // Update coefficients for all channels for this section
        size_t baseIdx = section * COEFFS_PER_SECTION;
        for (size_t ch = 0; ch < numChannels; ++ch) {
            coeffs[ch][baseIdx + 0] = b0;
            coeffs[ch][baseIdx + 1] = b1;
            coeffs[ch][baseIdx + 2] = b2;
            coeffs[ch][baseIdx + 3] = a1;
            coeffs[ch][baseIdx + 4] = a2;
        }
    }

    /**
     * @brief Set coefficients for a specific channel and section.
     * @param ch Channel index
     * @param section Section index
     * @param b0 Feedforward coefficient 0
     * @param b1 Feedforward coefficient 1
     * @param b2 Feedforward coefficient 2
     * @param a1 Feedback coefficient 1
     * @param a2 Feedback coefficient 2
     * @note Must call @ref prepare before setting coefficients.
     */
    void setChannelSectionCoeffs(size_t ch, size_t section, T b0, T b1, T b2, T a1, T a2) {
        // Early exit if not prepared
        if (!togglePrepared)
            return;
        assert(section < numSections && "Section index out of bounds");
        assert(ch < numChannels && "Channel index out of bounds");

        size_t baseIdx = section * COEFFS_PER_SECTION;
        coeffs[ch][baseIdx + 0] = b0;
        coeffs[ch][baseIdx + 1] = b1;
        coeffs[ch][baseIdx + 2] = b2;
        coeffs[ch][baseIdx + 3] = a1;
        coeffs[ch][baseIdx + 4] = a2;
    }

    /// Get number of prepared channels
    size_t getNumChannels() const { return numChannels; }
    /// Get number of prepared sections
    size_t getNumSections() const { return numSections; }
    /// Check if the filter is prepared
    bool isPrepared() const { return togglePrepared; }

  private:
    size_t numChannels = 0;
    size_t numSections = 0;
    bool togglePrepared = false;

    // Coefficient layout:
    // Channels: audio channels
    // Sections: second-order sections
    // Coefficients per section: [b0, b1, b2, a1, a2]
    // Accessing coeffs for channel ch, section s:
    //   b0 = coeffs[ch][s*5 + 0];
    //   b1 = coeffs[ch][s*5 + 1];
    //   b2 = coeffs[ch][s*5 + 2];
    //   a1 = coeffs[ch][s*5 + 3];
    //   a2 = coeffs[ch][s*5 + 4];
    AudioBuffer<T> coeffs;

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
} // namespace jnsc::detail