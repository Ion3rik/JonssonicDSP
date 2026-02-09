// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Filter topology structs
// SPDX-License-Identifier: MIT

#pragma once
#include "detail/biquad_design.h"
#include "detail/df1_engine.h"

/**
 * @brief Filter topology structs combine filter engines (e.g., Direct Form II Transposed) and designers (e.g.,
 * BiquadD). This allows for flexible combinations of design methods and processing topologies.
 *
 */

namespace jnsc {

/**
 * @brief Direct Form I biquad filter topology struct.
 * @tparam T Sample data type (e.g., float, double)
 */
template <typename T>
struct BiquadDF1 {
    /// Type alias for the engine and design components of this topology
    using Engine = detail::DF1Engine<T>;
    using Design = detail::BiquadDesign<T>;

    // Engine and design instances
    detail::DF1Engine<T> engine;
    detail::BiquadDesign<T> design;

    // Function to apply design coefficients to the engine for a specific channel and section
    void applyDesignToEngine(size_t ch, size_t section) {
        engine.setCoeffs(ch, section, design.getB0(), design.getB1(), design.getB2(), design.getA1(), design.getA2());
    }
};

/// Typealias for easy access to commonly used filter topologies
template <typename T>
using BiquadDF1Filter = Filter<T, BiquadDF1<T>>;
} // namespace jnsc
