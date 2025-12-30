// Jonssonic - A Modular Realtime C++ Audio DSP Library
// Filter type enumerations are defined here
// SPDX-License-Identifier: MIT

#pragma once

namespace jonssonic::filters
{  

/// First-order filter types
enum class FirstOrderType {
    Lowpass,
    Highpass,
    Allpass,
    Lowshelf,
    Highshelf
};   

/// Biquad filter types
enum class BiquadType {
    Lowpass,
    Highpass,
    Bandpass,
    Allpass,
    Notch,
    Peak,
    Lowshelf,
    Highshelf
};

/// Damping filter types
enum class DampingType {
    OnePole,
    Shelving 
};

} // namespace jonssonic::filters