// Jonssonic - A C++ audio DSP library
// Buffer utilities header file
// SPDX-License-Identifier: MIT
#pragma once
#include <cstring> 

namespace Jonssonic {

        /**
         * @brief Maps input channels to output channels for raw audio buffers.
         * Jonssonic DSP processors map N -> N channels. 
         * This utility can be used at the call site to duplicate or map channels as needed.
         *
         * @tparam T Sample type (e.g., float, double)
         * @param input Array of pointers to input channel data (const T* const*)
         * @param output Array of pointers to output channel data (T* const*)
         * @param numInputChannels Number of input channels
         * @param numOutputChannels Number of output channels
         * @param numSamples Number of samples per channel
         */
        template<typename T>
        inline void mapChannels(const T* const* input, T* const* output, int numInputChannels, int numOutputChannels, int numSamples)
        {
            for (int channel = 0; channel < numOutputChannels; ++channel)
            {
                int inputChannel = channel < numInputChannels ? channel : numInputChannels - 1;
                std::memcpy(output[channel], input[inputChannel], sizeof(T) * numSamples);
            }
        }
}