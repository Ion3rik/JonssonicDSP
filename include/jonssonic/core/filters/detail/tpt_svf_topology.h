// JonssonicDSP - A modular realtime C++ audio DSP library
// State Variable Filter TPT Topology class header file
// SPDX-License-Identifier: MIT
#pragma once

#include "jonssonic/utils/detail/config_utils.h"
#include <jonssonic/core/common/tpt_integrator.h>
#include <jonssonic/core/filters/detail/filter_limits.h>

namespace jnsc::detail {

/**
 * @brief State Variable Filter TPT Topology class implementing a multi-channel, multi-section state variable filter.
 * @tparam T Sample data type (e.g., float, double)
 */
template <typename T>
class SVFTPTTopology {
  public:
    /// Constexprs for state variable count per section
    static constexpr size_t STATE_VARS_PER_SECTION = 2; // z1, z2 for each of the 2 integrators in the biquad section

    /// Default constructor
    SVFTPTTopology() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels
     * @param newNumSections Number of second-order sections
     */
    SVFTPTTopology(size_t newNumChannels, size_t newNumSections) { prepare(newNumChannels, newNumSections); }

    /// Default destructor
    ~SVFTPTTopology() = default;

    // No copy or move semantics
    SVFTPTTopology(const SVFTPTTopology&) = delete;
    SVFTPTTopology& operator=(const SVFTPTTopology&) = delete;
    SVFTPTTopology(SVFTPTTopology&&) = delete;
    SVFTPTTopology& operator=(SVFTPTTopology&&) = delete;

    /**
     * @brief Prepare the filter for processing.
     * @param newNumChannels Number of channels
     * @param newNumSections Number of second-order sections
     */
    void prepare(size_t newNumChannels, size_t newNumSections) {
        // Clamp channels and sections to allowed limits
        numChannels = utils::detail::clampChannels(newNumChannels);
        numSections = FilterLimits<T>::clampSections(newNumSections);

        // Prepare the TPT integrator
        integrator.prepare(numChannels, STATE_VARS_PER_SECTION * numSections);

        // Mark as prepared
        togglePrepared = true;
    }

    /// Reset the filter state
    void reset() { integrator.reset(); }

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
     */
    void process(size_t ch, size_t section, T input, T g, T twoR, T& hp, T& bp, T& lp) {
        T g0 = twoR + g;
        T d = T(1.0) / (T(1.0) + twoR * g + g * g);

        // Get previous integrator states
        size_t stateBase = section * STATE_VARS_PER_SECTION;
        T s0 = integrator.getState()(ch, stateBase + 0);
        T s1 = integrator.getState()(ch, stateBase + 1);

        // Highpass output
        hp = (input - s1 - g0 * s0) * d;

        // Bandpass output
        bp = integrator.processSample(ch, stateBase + 0, hp, g);

        // Lowpass output
        lp = integrator.processSample(ch, stateBase + 1, bp, g);
    }

  private:
    bool togglePrepared = false;
    size_t numChannels = 0;
    size_t numSections = 0;
    TPTIntegrator<T> integrator; // TPT integrator for the state variable filter
};

} // namespace jnsc::detail
