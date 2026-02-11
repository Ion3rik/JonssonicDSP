// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// DF2TBiquadTopology header file
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/utils/detail/config_utils.h"
#include <jonssonic/core/common/audio_buffer.h>
#include <jonssonic/core/filters/detail/filter_limits.h>

namespace jnsc::detail {
/**
 * @brief DF2TBiquadTopology filter class implementing a multi-channel, multi-section biquad filter in Direct Form II
 * Transposed.
 * @param T Sample data type (e.g., float, double)
 */
template <typename T>
class DF2TBiquadTopology {
  public:
    /// Constexprs for coefficient and state variable counts
    static constexpr size_t COEFFS_PER_SECTION = 5;     // b0, b1, b2, a1, a2
    static constexpr size_t STATE_VARS_PER_SECTION = 2; // s1, s2

    /// Default constructor
    DF2TBiquadTopology() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels
     * @param newNumSections Number of second-order sections
     */
    DF2TBiquadTopology(size_t newNumChannels, size_t newNumSections) { prepare(newNumChannels, newNumSections); }

    /// Default destructor
    ~DF2TBiquadTopology() = default;

    /// No copy nor move semantics
    DF2TBiquadTopology(const DF2TBiquadTopology&) = delete;
    DF2TBiquadTopology& operator=(const DF2TBiquadTopology&) = delete;
    DF2TBiquadTopology(DF2TBiquadTopology&&) = delete;
    DF2TBiquadTopology& operator=(DF2TBiquadTopology&&) = delete;

    /**
     * @brief Prepare the filter for processing.
     * @param newNumChannels Number of channels
     * @param newNumSections Number of second-order sections
     */
    void prepare(size_t newNumChannels, size_t newNumSections) {
        // Clamp channels and sections to allowed limits
        numChannels = utils::detail::clampChannels(newNumChannels);
        numSections = FilterLimits<T>::clampSections(newNumSections);

        // Allocate coefficient and state buffers
        coeffs.resize(numChannels, numSections * COEFFS_PER_SECTION);
        state.resize(numChannels, numSections * STATE_VARS_PER_SECTION);

        // Mark as prepared
        togglePrepared = true;
    }

    /// Reset the filter state
    void reset() { state.clear(); }

    /**
     * @brief Process a single sample for a specific channel and section.
     * @param ch Channel index.
     * @param section Section index.
     * @param input Input sample.
     * @return Output sample.
     * @note Must call @ref prepare before processing.
     */
    T processSample(size_t ch, size_t section, T input) {
        // Calculate base indices for coefficients and state variables
        size_t coeffBase = section * COEFFS_PER_SECTION;
        size_t stateBase = section * STATE_VARS_PER_SECTION;

        // Fetch coefficients
        T b0 = coeffs[ch][coeffBase + 0];
        T b1 = coeffs[ch][coeffBase + 1];
        T b2 = coeffs[ch][coeffBase + 2];
        T a1 = coeffs[ch][coeffBase + 3];
        T a2 = coeffs[ch][coeffBase + 4];

        // Fetch state variables
        T s1 = state[ch][stateBase + 0];
        T s2 = state[ch][stateBase + 1];

        // Compute output sample (Direct Form II Transposed)
        T output = b0 * input + s1;

        // Update state variables
        state[ch][stateBase + 0] = b1 * input - a1 * output + s2;
        state[ch][stateBase + 1] = b2 * input - a2 * output;

        return output;
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
    void setCoeffs(size_t ch, size_t section, T b0, T b1, T b2, T a1, T a2) {
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
    /// Get state buffer (for testing purposes)
    const AudioBuffer<T>& getState() const { return state; }
    /// Get coefficient buffer (for testing purposes)
    const AudioBuffer<T>& getCoeffs() const { return coeffs; }

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
    // Samples per channel: numSections * 2 (for s1, s2 per section)
    // Writing to channel ch, section s:
    //   state[ch][s*2 + 0] = s1;
    //   state[ch][s*2 + 1] = s2;
    // Reading from channel ch, section s:
    //   s1 = state[ch][s*2 + 0];
    //   s2 = state[ch][s*2 + 1];
    AudioBuffer<T> state;
};
} // namespace jnsc::detail