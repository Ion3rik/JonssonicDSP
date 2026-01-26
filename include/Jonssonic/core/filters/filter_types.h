// Jonssonic - A Modular Realtime C++ Audio DSP Library
// Filter type enumerations are defined here
// SPDX-License-Identifier: MIT

#pragma once

namespace jnsc {

/// First-order filter types
enum class FirstOrderType { Lowpass, Highpass, Allpass, Lowshelf, Highshelf };

/// Biquad filter types
enum class BiquadType { Lowpass, Highpass, Bandpass, Allpass, Notch, Peak, Lowshelf, Highshelf };

/// Damping filter types
enum class DampingType { Bypass, OnePole, BiquadShelf, FirstOrderShelf };

} // namespace jnsc