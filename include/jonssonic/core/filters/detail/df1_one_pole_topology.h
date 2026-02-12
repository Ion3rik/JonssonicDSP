// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// DF1OnePoleTopology header file
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/utils/detail/config_utils.h"
#include <cassert>
#include <jonssonic/core/common/audio_buffer.h>
#include <vector>

namespace jnsc::detail {
/**
 * @brief DF1OnePoleTopology filter class implementing a multi-channel, multi-section first-order
 * filter in Direct Form I.
 * @param T Sample data type (e.g., float, double)
 */
template <typename T>
class DF1OnePoleTopology {
  public:
    /**
     * @brief Constexprs for coefficient and state variable counts per section.
     * @param COEFFS_PER_SECTION Number of coefficients per first-order section (b0, b1, a1)
     * @param STATE_VARS_PER_SECTION Number of state variables per first-order section (x1, y1)
     */
    static constexpr size_t COEFFS_PER_SECTION = 3;     // b0, b1, a1
    static constexpr size_t STATE_VARS_PER_SECTION = 2; // x1, y1

    /// Default constructor
    DF1OnePoleTopology() = default;
    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels
     * @param newNumSections Number of first-order sections
     */
    DF1OnePoleTopology(size_t newNumChannels, size_t newNumSections) { prepare(newNumChannels, newNumSections); }

    /// Default destructor
    ~DF1OnePoleTopology() = default;

    /// No copy nor move semantics
    DF1OnePoleTopology(const DF1OnePoleTopology&) = delete;
    DF1OnePoleTopology& operator=(const DF1OnePoleTopology&) = delete;
    DF1OnePoleTopology(DF1OnePoleTopology&&) = delete;
    DF1OnePoleTopology& operator=(DF1OnePoleTopology&&) = delete;

    /**
     * @brief Prepare the filter for processing.
     * @param newNumChannels Number of channels
     * @param newNumSections Number of first-order sections
     */
    void prepare(size_t newNumChannels, size_t newNumSections) {
        // Clamp channels and sections to allowed limits
        numChannels = utils::detail::clampChannels(newNumChannels);
        numSections = FilterLimits<T>::clampSections(newNumSections);
        // Allocate coefficient and state buffers
        coeffs.resize(numChannels, numSections * COEFFS_PER_SECTION);
        state.resize(numChannels, numSections * STATE_VARS_PER_SECTION);
        togglePrepared = true;
    }

    /// Reset the filter state
    void reset() { state.clear(); }

    /**
     * @brief Process a single sample for a given channel and section.
     * @param ch Channel index.
     * @param section Section index.
     * @param input Input sample.
     * @return Output sample.
     * @note Must call @ref prepare before processing.
     */
    T processSample(size_t ch, size_t section, T input) {
        // Compute base indices for coefficients and state variables
        size_t coeffBase = section * COEFFS_PER_SECTION;
        size_t stateBase = section * STATE_VARS_PER_SECTION;

        // Fetch coefficients
        T b0 = coeffs[ch][coeffBase + 0];
        T b1 = coeffs[ch][coeffBase + 1];
        T a1 = coeffs[ch][coeffBase + 2];

        // Fetch state variables
        T x1 = state[ch][stateBase + 0];
        T y1 = state[ch][stateBase + 1];

        // Compute output sample (Direct Form I)
        T output = b0 * input + b1 * x1 - a1 * y1;

        // Update state variables
        state[ch][stateBase + 0] = input;  // x1 = input
        state[ch][stateBase + 1] = output; // y1 = output

        return output;
    }

    /**
     * @brief Set the coefficients for a specific channel and section.
     * @param ch Channel index.
     * @param section Section index.
     * @param b0 Feedforward coefficient 0.
     * @param b1 Feedforward coefficient 1.
     * @param a1 Feedback coefficient 1.
     * @note Must call @ref prepare before setting coefficients.
     */
    void setCoeffs(size_t ch, size_t section, T b0, T b1, T a1) {
        // Early exit if not prepared
        if (!togglePrepared)
            return;
        assert(section < numSections && "Section index out of bounds");
        assert(ch < numChannels && "Channel index out of bounds");
        size_t baseIdx = section * COEFFS_PER_SECTION;
        coeffs[ch][baseIdx + 0] = b0;
        coeffs[ch][baseIdx + 1] = b1;
        coeffs[ch][baseIdx + 2] = a1;
    }

    /// Get number of prepared channels
    size_t getNumChannels() const { return numChannels; }
    /// Get number of prepared sections
    size_t getNumSections() const { return numSections; }
    /// Check if the filter is prepared
    bool isPrepared() const { return togglePrepared; }
    /// Get state buffer (for testing purposes)
    const AudioBuffer<T>& getState() const { return state; }
    /// Get coefficient buffer (for testing purposes)
    const AudioBuffer<T>& getCoeffs() const { return coeffs; }

  private:
    bool togglePrepared = false;
    size_t numChannels = 0;
    size_t numSections = 0;
    // Coefficient layout:
    // Channels: audio channels
    // Sections: first-order sections
    // Coefficients per section and channel: [b0, b1, a1]
    // Accessing coeffs for channel ch, section s:
    //   b0 = coeffs[ch][s*3 + 0];
    //   b1 = coeffs[ch][s*3 + 1];
    //   a1 = coeffs[ch][s*3 + 2];
    AudioBuffer<T> coeffs;

    // State buffer layout:
    // Channels: audio channels
    // Samples per channel: numSections * 2 (for x1, y1 per section)
    // Writing to channel ch, section s:
    //   state[ch][s*2 + 0] = x1;
    //   state[ch][s*2 + 1] = y1;
    // Reading from channel ch, section s:
    //   x1 = state[ch][s*2 + 0];
    //   y1 = state[ch][s*2 + 1];
    AudioBuffer<T> state;
};

} // namespace jnsc::detail