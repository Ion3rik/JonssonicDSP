// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Filter topology structs
// SPDX-License-Identifier: MIT

#pragma once
#include "detail/df1_engine.h"

/**
 * @file
 * @brief Filter topology structs for biquad and first-order filters.
 * @details These structs combine filter engines and designers to define specific filter topologies (e.g., Direct Form
 * II, One-Pole, State Variable) with their associated processing and coefficient update logic.
 */

namespace jnsc::detail {

/**
 * @brief Direct Form I biquad filter topology struct.
 * @tparam T Sample data type (e.g., float, double)
 */
template <typename T>
struct DF1Biquad {
    DF1Engine<T> engine;
    DF1Design<T> design;
};
} // namespace jnsc::detail
