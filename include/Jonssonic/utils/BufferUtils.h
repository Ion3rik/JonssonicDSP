// Jonssonic - A C++ audio DSP library
// Buffer utilities header file
// SPDX-License-Identifier: MIT
#pragma once
#include <cstring> 

namespace Jonssonic {

    //==============================================================================
    // FUNCTIONS
    //==============================================================================

    /**
     * @brief Maps input channels to output channels for raw audio buffers.
     *        Supports both upmixing and downmixing.
     *        Dow
     * @tparam T Sample type (e.g., float, double)
     * @param input Array of pointers to input channel data (const T* const*)
     * @param output Array of pointers to output channel data (T* const*)
     * @param numInputChannels Number of input channels
     * @param numOutputChannels Number of output channels
     * @param numSamples Number of samples per channel
     */
    template<typename T>
    inline void mapChannels(const T* const* input, T* const* output, size_t numInputChannels, size_t numOutputChannels, size_t numSamples)
    {
        for (int outCh = 0; outCh < numOutputChannels; ++outCh)
        {
            if (numInputChannels == numOutputChannels)
            {
                // Direct copy
                std::memcpy(output[outCh], input[outCh], sizeof(T) * numSamples);
            }
            else if (numInputChannels > numOutputChannels)
            {
                // Downmix: average groups of input channels
                int groupSize = numInputChannels / numOutputChannels;
                for (int n = 0; n < numSamples; ++n)
                {
                    T sum = 0;
                    for (int i = 0; i < groupSize; ++i)
                        sum += input[outCh * groupSize + i][n];
                    output[outCh][n] = sum / groupSize;
                }
            }
            else
            {
                // Upmix: wrap or duplicate
                int inCh = outCh % numInputChannels;
                std::memcpy(output[outCh], input[inCh], sizeof(T) * numSamples);
            }
        }
    }


    /**
     * @brief Planar to interleaved buffer conversion: planarBuffer [ch][samp] -> interleavedBuffer [samp][ch]
     */
    template<typename T>
    void planarToInterleaved(const T* const* planarBuffer, T* const* interleavedBuffer, size_t numChannels, size_t numSamples) {
        for (size_t n = 0; n < numSamples; ++n) // outer loop over samples for better cache performance for writing
            for (size_t ch = 0; ch < numChannels; ++ch)
                interleavedBuffer[n][ch] = planarBuffer[ch][n];
    }

    /**
     * @brief Interleaved to planar buffer conversion: interleavedBuffer [samp][ch] -> planarBuffer [ch][samp]
     */
    template<typename T>
    void interleavedToPlanar(const T* const* interleavedBuffer, T* const* planarBuffer, size_t numChannels, size_t numSamples) {
        for (size_t ch = 0; ch < numChannels; ++ch) // outer loop over channels for better cache performance for writing
            for (size_t n = 0; n < numSamples; ++n)
                planarBuffer[ch][n] = interleavedBuffer[n][ch];
    }

    /**
     * @brief Apply fixed gain to a buffer.
     * @param buffer Array of pointers to channel data
     * @param numChannels Number of channels
     * @param numSamples Number of samples per channel
     * @param gain Gain factor to apply
     */

    template<typename T>
    void applyGain(T* const* buffer, size_t numChannels, size_t numSamples, T gain) {
        for (size_t ch = 0; ch < numChannels; ++ch)
            for (size_t n = 0; n < numSamples; ++n)
                buffer[ch][n] *= gain;
    }

    /**
     * @brief copyToBuffer - Copy data from source buffer to destination buffer
     * @param src Source buffer (array of channel pointers)
     * @param dest Destination buffer (array of channel pointers)
     * @param numChannels Number of channels
     * @param numSamples Number of samples per channel
     */

    template<typename T>
    void copyToBuffer(const T* const* src, T* const* dest, size_t numChannels, size_t numSamples) {
        for (size_t ch = 0; ch < numChannels; ++ch)
            std::memcpy(dest[ch], src[ch], numSamples * sizeof(T));
    }
} // namespace Jonssonic