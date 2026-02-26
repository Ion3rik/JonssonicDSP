// JonssonicDSP - A modular realtime C++ audio DSP library
// TPT State Variable Filter Topology class header file
// SPDX-License-Identifier: MIT
#pragma once

#include "jonssonic/utils/detail/config_utils.h"
#include <jonssonic/core/common/audio_buffer.h>
#include <jonssonic/core/filters/detail/filter_limits.h>

namespace jnsc::detail {

/**
 * @brief TPT State Variable Filter Topology class implementing a multi-channel, multi-section state variable filter.
 * @tparam T Sample data type (e.g., float, double)
 */
template <typename T>
class TPTSVFTopology {
  public:
    /// Constexprs for state variable count per section
    static constexpr size_t STATE_VARS_PER_SECTION = 2; // z1, z2 for each of the 2 integrators in the biquad section

    /// Default constructor
    TPTSVFTopology() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels
     * @param newNumSections Number of second-order sections
     */
    TPTSVFTopology(size_t newNumChannels, size_t newNumSections) { prepare(newNumChannels, newNumSections); }

    /// Default destructor
    ~TPTSVFTopology() = default;

    // No copy or move semantics
    TPTSVFTopology(const TPTSVFTopology&) = delete;
    TPTSVFTopology& operator=(const TPTSVFTopology&) = delete;
    TPTSVFTopology(TPTSVFTopology&&) = delete;
    TPTSVFTopology& operator=(TPTSVFTopology&&) = delete;

    /**
     * @brief Prepare the filter for processing.
     * @param newNumChannels Number of channels
     * @param newNumSections Number of second-order sections
     */
    void prepare(size_t newNumChannels, size_t newNumSections) {
        // Clamp channels and sections to allowed limits
        numChannels = utils::detail::clampChannels(newNumChannels);
        numSections = FilterLimits<T>::clampSections(newNumSections);

        // Resize state buffer to hold state variables for all channels and sections
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
     * @param g Integration coefficient for the TPT integrator (tan(pi * fc / fs))
     * @param twoR Damping factor for the state variable filter (2 * R, where R is the filter's damping ratio)
     * @param hp Output variable for the highpass output.
     * @param bp Output variable for the bandpass output.
     * @param lp Output variable for the lowpass output.
     * @param ap Output variable for the allpass output.
     */
    void processSample(size_t ch, size_t section, T input, T g, T twoR, T& hp, T& bp, T& lp) {
        T g0 = twoR + g;
        T d = T(1.0) / (T(1.0) + twoR * g + g * g);

        // Get previous integrator states
        size_t stateBase = section * STATE_VARS_PER_SECTION;
        T z0 = state[ch][stateBase + 0];
        T z1 = state[ch][stateBase + 1];

        // Highpass output
        hp = (input - z1 - g0 * z0) * d;

        // Bandpass output
        T v0 = g * hp;
        bp = v0 + z0;

        // Lowpass output
        T v1 = g * bp;
        lp = v1 + z1;

        // Update state variables for next sample
        state[ch][section * STATE_VARS_PER_SECTION + 0] = bp + v0;
        state[ch][section * STATE_VARS_PER_SECTION + 1] = lp + v1;
    }

    /// Get the state variable for a specific channel and section.
    T getState(size_t ch, size_t section, size_t stateVarIndex) const {
        size_t stateBase = section * STATE_VARS_PER_SECTION;
        return state[ch][stateBase + stateVarIndex];
    }
    /// Get number of prepared channels
    size_t getNumChannels() const { return numChannels; }
    /// Get number of prepared sections
    size_t getNumSections() const { return numSections; }
    /// Check if the topology is prepared
    bool isPrepared() const { return togglePrepared; }

  private:
    bool togglePrepared = false;
    size_t numChannels = 0;
    size_t numSections = 0;
    AudioBuffer<T> state;
};

} // namespace jnsc::detail
