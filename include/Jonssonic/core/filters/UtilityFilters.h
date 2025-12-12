// Jonssonic - A C++ audio DSP library
// UtilityFilters: Common utility filter wrappers and aliases
// SPDX-License-Identifier: MIT

#pragma once
#include "FirstOrderFilter.h"

namespace Jonssonic {

/**
 * @brief DC Blocker - A first-order highpass filter with very low cutoff
 * @param T Sample data type (e.g., float, double)
 */
template<typename T>
class DCBlocker {
public:
    DCBlocker() = default;
    
    void prepare(size_t newNumChannels, T newSampleRate) {
        filter.prepare(newNumChannels, newSampleRate);
        filter.setType(FirstOrderType::Highpass);
        filter.setFreqNormalized(T(0.0005)); // Very low cutoff for DC blocking
    }
    
    void clear() {
        filter.clear();
    }
    
    T processSample(size_t ch, T input) {
        return filter.processSample(ch, input);
    }
    
    void processBlock(const T* const* input, T* const* output, size_t numSamples) {
        filter.processBlock(input, output, numSamples);
    }

private:
    FirstOrderFilter<T> filter;
};

} // namespace Jonssonic
