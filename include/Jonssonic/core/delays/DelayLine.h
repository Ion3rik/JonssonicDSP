// Jonssonic - A C++ audio DSP library
// Delay line class header file
// Author: Jon Fagerström
// Update: 18.11.2025

#pragma once
#include "../common/AudioBuffer.h"
#include "../../utils/MathUtils.h"

namespace Jonssonic
{
/**
 * @brief A delay line class for audio processing.
 * This class implements a circular buffer to create delay effects with support for multiple channels.
 */
template<typename T, typename Interpolator>
class DelayLine
{
public:
    DelayLine() = default;
    ~DelayLine() = default;

    // No copy semantics nor move semantics
    DelayLine(const DelayLine&) = delete;
    const DelayLine& operator=(const DelayLine&) = delete;
    DelayLine(DelayLine&&) = delete;
    const DelayLine& operator=(DelayLine&&) = delete;

    void prepare(size_t newNumChannels, size_t maxDelaySamples)
    {
        numChannels = newNumChannels;
        bufferSize = nextPowerOfTwo(maxDelaySamples);
        buffer.resize(numChannels, bufferSize);
        buffer.clear();
        writeIndex.resize(numChannels, 0);
    }

    void clear()
    {
        buffer.clear();
    }

    /**
     * @brief Process a single sample for all channels with fixed delay.
     * @param input Input samples (one value per channel) - array of size numChannels
     * @param output Output samples (one value per channel) - array of size numChannels
     * 
     * @note Input and output must have the same number of channels as prepared.
     */
    T processSample(T input, size_t ch)
    {
        // Write input sample to buffer
        buffer[ch][writeIndex[ch]] = input;

        // Calculate read index with wrap-around (fixed delay)
        size_t readIndex = (writeIndex[ch] + bufferSize - delaySamples) & (bufferSize - 1);

        // Increment and wrap with bitwise AND for power-of-two buffer size
        writeIndex[ch] = (writeIndex[ch] + 1) & (bufferSize - 1);

        // Read output sample directly (no interpolation needed for fixed integer delay)
        return buffer[ch][readIndex];
    }

    /**
     * @brief Process a single sample for all channels with modulated delay.
     * @param input Input samples (one value per channel) - array of size numChannels
     * @param output Output samples (one value per channel) - array of size numChannels
     * @param modulation Modulation values (one value per channel, in samples) - array of size numChannels
     * 
     * @note The modulation signal is added to the base delay time. 
     *       Modulation values should be in samples and will be clamped to valid range [0, bufferSize-1].
     * @note Input, output, and modulation must all have the same number of channels as prepared.
     */
    T processSample(T input, T modulation, size_t ch)
    {
        // Write input sample to buffer
        buffer[ch][writeIndex[ch]] = input;

        // Calculate modulated delay time (base delay + modulation)
        T modulatedDelay = static_cast<T>(delaySamples) + modulation;
        
        // Clamp modulated delay to valid range
        modulatedDelay = std::max(T(0), std::min(modulatedDelay, static_cast<T>(bufferSize - 1)));
        
        // Split into integer and fractional parts for interpolation
        size_t delayInt = static_cast<size_t>(modulatedDelay);
        T delayFrac = modulatedDelay - static_cast<T>(delayInt);
        
        // Calculate read index with wrap-around
        size_t readIndex = (writeIndex[ch] + bufferSize - delayInt) & (bufferSize - 1);

            // Increment and wrap write index
        writeIndex[ch] = (writeIndex[ch] + 1) & (bufferSize - 1);

        // Interpolate output sample
        return Interpolator::interpolate(buffer.readChannelPtr(ch), readIndex, delayFrac);
    

    }

    /**
     * @brief Process a block of samples for all channels with fixed delay.
     * @param input Input sample pointers (one per channel)
     * @param output Output sample pointers (one per channel)
     * @param numSamples Number of samples to process
     * 
     * @note Input and output must have the same number of channels as prepared.
     */
    void processBlock(const T* const* input, T* const* output, size_t numSamples)
    {
        for (size_t ch = 0; ch < numChannels; ++ch)
        {   
            for (size_t i = 0; i < numSamples; ++i)
            {
                // Write input sample to buffer
                buffer[ch][writeIndex[ch]] = input[ch][i];

                // Calculate read index with wrap-around (fixed delay)
                size_t readIndex = (writeIndex[ch] + bufferSize - delaySamples) & (bufferSize - 1);

                // Read output sample directly (no interpolation needed for fixed integer delay)
                output[ch][i] = buffer[ch][readIndex];
                
                // Increment and wrap write index
                writeIndex[ch] = (writeIndex[ch] + 1) & (bufferSize - 1);
            }
        }
    }

    /**
     * @brief Process a block of samples for all channels with modulated delay.
     * @param input Input sample pointers (one per channel)
     * @param output Output sample pointers (one per channel)
     * @param modulation Modulation signal pointers (one per channel, in samples)
     * @param numSamples Number of samples to process
     * 
     * @note The modulation signal is added to the base delay time. 
     *       Modulation values should be in samples and will be clamped to valid range [0, bufferSize-1].
     * @note Input, output, and modulation must all have the same number of channels as prepared.
     */
    void processBlock(const T* const* input, T* const* output, 
                     const T* const* modulation, size_t numSamples)
    {
        for (size_t ch = 0; ch < numChannels; ++ch)
        {   
            for (size_t i = 0; i < numSamples; ++i)
            {
                // Write input sample to buffer
                buffer[ch][writeIndex[ch]] = input[ch][i];

                // Calculate modulated delay time (base delay + modulation)
                T modulatedDelay = static_cast<T>(delaySamples) + modulation[ch][i];
                
                // Clamp modulated delay to valid range
                modulatedDelay = std::max(T(0), std::min(modulatedDelay, static_cast<T>(bufferSize - 1)));
                
                // Split into integer and fractional parts for interpolation
                size_t delayInt = static_cast<size_t>(modulatedDelay);
                T delayFrac = modulatedDelay - static_cast<T>(delayInt);
                
                // Calculate read index with wrap-around
                size_t readIndex = (writeIndex[ch] + bufferSize - delayInt) & (bufferSize - 1);

                // Interpolate output sample
                output[ch][i] = Interpolator::interpolate(buffer.readChannelPtr(ch), readIndex, delayFrac);
                
                // Increment and wrap write index
                writeIndex[ch] = (writeIndex[ch] + 1) & (bufferSize - 1);
            }
        }
    }

    void setDelay(size_t newDelaySamples)
    {
        delaySamples = std::min(newDelaySamples, bufferSize); // Clamp to buffer size
    }

private:
    AudioBuffer<T> buffer;              // Multi-channel circular buffer
    std::vector<size_t> writeIndex;     // Current write index in the buffer per channel
    size_t bufferSize;                  // Maximum delay in samples (always power of two)
    size_t delaySamples = 0;            // Current delay time in samples
    size_t numChannels;                 // Number of audio channels

};
} // namespace Jonssonic

