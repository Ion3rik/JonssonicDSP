// Jonssonic - A C++ audio DSP library
// Filter types header file
// Filter type enumerations are defined here
// SPDX-License-Identifier: MIT

#pragma once

namespace Jonssonic
{  

/**
 * @brief First-order filter types
 */
enum class FirstOrderType {
    Lowpass,
    Highpass,
    Allpass,
    Lowshelf,
    Highshelf
};   

/**
 * @brief Biquad Filter Types
 */
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

/**
 * @brief Damping filter types
 */
enum class DampingType {
    OnePole,
    Shelving 
};

} // namespace Jonssonic