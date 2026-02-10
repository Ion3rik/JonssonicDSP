// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Biquad Direct Form I filter engine struct header file
// SPDX-License-Identifier: MIT

#pragma once
#include "detail/biquad_design.h"
#include "detail/df1_topology.h"

/**
 * @brief Filter engine structs combine filter topologies (e.g., Direct Form II Transposed) and designers (e.g.,
 * BiquadDesign). This allows for flexible combinations of design methods and processing topologies.
 *
 */

namespace jnsc::detail {

/**
 * @brief Direct Form I biquad filter engine struct.
 * @tparam T Sample data type (e.g., float, double)
 */
template <typename T>
struct BiquadDF1 {
    /// Type alias for the engine and design components of this topology
    using Topology = detail::DF1Topology<T>;
    using Design = detail::BiquadDesign<T>;

    // Topology and design instances
    detail::DF1Topology<T> topology;
    detail::BiquadDesign<T> design;

    // Function to apply design coefficients to the topology for a specific channel and section
    void applyDesignToTopology(size_t ch, size_t section) {
        topology.setCoeffs(ch, section, design.getB0(), design.getB1(), design.getB2(), design.getA1(), design.getA2());
    }

    // Set the response type for the entire filter (applies to all channels and sections)
    void setResponse(Response newResponse) {
        design.setResponse(newResponse);
        for (size_t ch = 0; ch < topology.getNumChannels(); ++ch)
            for (size_t section = 0; section < topology.getNumSections(); ++section)
                applyDesignToTopology(ch, section);
    }

    // Set the frequency for the entire filter (applies to all channels and sections)
    void setFrequency(Frequency<T> newFreq) {
        design.setFrequency(newFreq);
        for (size_t ch = 0; ch < topology.getNumChannels(); ++ch)
            for (size_t section = 0; section < topology.getNumSections(); ++section)
                applyDesignToTopology(ch, section);
    }

    // Set the Q factor for the entire filter (applies to all channels and sections)
    void setQ(T newQ) {
        design.setQ(newQ);
        for (size_t ch = 0; ch < topology.getNumChannels(); ++ch)
            for (size_t section = 0; section < topology.getNumSections(); ++section)
                applyDesignToTopology(ch, section);
    }

    // Set the gain for the entire filter (applies to all channels and sections)
    void setGain(Gain<T> newGain) {
        design.setGain(newGain);
        for (size_t ch = 0; ch < topology.getNumChannels(); ++ch)
            for (size_t section = 0; section < topology.getNumSections(); ++section)
                applyDesignToTopology(ch, section);
    }
};

} // namespace jnsc::detail
