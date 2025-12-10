// Jonssonic - A C++ audio DSP library
// UtilityFilters: Common utility filter wrappers and aliases
// SPDX-License-Identifier: MIT

#pragma once
#include "FirstOrderFilter.h"

namespace Jonssonic {

class UtilityFilters {
public:
    /**
     * @brief Create a DC blocker (first-order highpass with very low cutoff)
     * @param numChannels Number of channels
     * @param sampleRate Sample rate in Hz
     * @param cutoffHz Cutoff frequency in Hz (default: 20 Hz)
     */
    template<typename T>
    static FirstOrderFilter<T> DCBlocker() {
        FirstOrderFilter<T> filter(); // construct a first-order filter
        filter.setType(FirstOrderType::Highpass); // select highpass
        filter.setFreqNormalized(T(0.0005)); // set very low cutoff
        return filter;
    }
};

} // namespace Jonssonic
