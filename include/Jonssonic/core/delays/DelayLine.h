// Jonssonic - A C++ audio DSP library
// Delay line class header file
// Author: Jon Fagerström
// Update: 18.11.2025

#pragma once
#include "../common/MultiChannelBuffer.h"
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
    DelayLine()
        : buffer(),
          writeIndex(0),
          bufferSize(0),
          numChannels(0)
    {}
    ~DelayLine() = default;

    // No copy semantics nor move semantics
    DelayLine(const DelayLine&) = delete;
    const DelayLine& operator=(const DelayLine&) = delete;
    DelayLine(DelayLine&&) = delete;
    const DelayLine& operator=(DelayLine&&) = delete;

    void setDelay(size_t newDelaySamples)
    {
        delaySamples = std::min(newDelaySamples, bufferSize); // Clamp to buffer size
    }

    void prepare(size_t newNumChannels, size_t maxDelaySamples)
    {
        numChannels = newNumChannels;
        bufferSize = nextPowerOfTwo(maxDelaySamples);
        buffer.resize(numChannels, bufferSize);
        buffer.clear();
        writeIndex = 0;
    }

    /**
     * @brief Process a single sample for all channels with fixed delay.
     * @param input Input sample pointers (one per input channel)
     * @param output Output sample pointers (one per output channel)
     * @param numInputChannels Number of input channels
     * 
     * @note Channel handling:
     *       - If numInputChannels < delay line channels: input[0] is duplicated to remaining channels
     *       - If numInputChannels > delay line channels: extra input channels are ignored
     *       - Output always matches the delay line's channel count
     */
    void processSample(const T* const* input, T* const* output, size_t numInputChannels)
    {
        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            const T* inChannel = input[ch < numInputChannels ? ch : 0]; // Handle fewer input channels than delay line channels
            T* outChannel = output[ch]; // Get output channel pointer

            // Write input sample to buffer
            buffer.getChannelPointer(ch)[writeIndex] = inChannel[0];

            // Calculate read index with wrap-around (fixed delay)
            size_t readIndex = (writeIndex + bufferSize - delaySamples) & (bufferSize - 1);

            // Read output sample directly (no interpolation needed for fixed integer delay)
            outChannel[0] = buffer.getChannelPointer(ch)[readIndex];
        }

        // Increment and wrap with bitwise AND for power-of-two buffer size
        writeIndex = (writeIndex + 1) & (bufferSize - 1);
    }

    /**
     * @brief Process a single sample for all channels with modulated delay.
     * @param input Input sample pointers (one per input channel)
     * @param output Output sample pointers (one per output channel)
     * @param modulation Modulation signal pointers (one per channel, in samples)
     * @param numInputChannels Number of input channels
     * 
     * @note The modulation signal is added to the base delay time. 
     *       Modulation values should be in samples and will be clamped to valid range [0, bufferSize-1].
     * 
     * @note Channel handling:
     *       - If numInputChannels < delay line channels: input[0] and modulation[0] are duplicated
     *       - If numInputChannels > delay line channels: extra input channels are ignored
     *       - Output always matches the delay line's channel count
     */
    void processSample(const T* const* input, T* const* output, 
                      const T* const* modulation, size_t numInputChannels)
    {
        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            const T* inChannel = input[ch < numInputChannels ? ch : 0];
            T* outChannel = output[ch];
            const T* modChannel = modulation[ch < numInputChannels ? ch : 0];

            // Write input sample to buffer
            buffer.getChannelPointer(ch)[writeIndex] = inChannel[0];

            // Calculate modulated delay time (base delay + modulation)
            T modulatedDelay = static_cast<T>(delaySamples) + modChannel[0];
            
            // Clamp modulated delay to valid range
            modulatedDelay = std::max(T(0), std::min(modulatedDelay, static_cast<T>(bufferSize - 1)));
            
            // Split into integer and fractional parts for interpolation
            size_t delayInt = static_cast<size_t>(modulatedDelay);
            T delayFrac = modulatedDelay - static_cast<T>(delayInt);
            
            // Calculate read index with wrap-around
            size_t readIndex = (writeIndex + bufferSize - delayInt) & (bufferSize - 1);

            // Interpolate output sample
            outChannel[0] = Interpolator::interpolate(buffer.getChannelPointer(ch), readIndex, delayFrac);
        }

        // Increment and wrap write index
        writeIndex = (writeIndex + 1) & (bufferSize - 1);
    }

    /**
     * @brief Process a block of samples for all channels with fixed delay.
     * @param input Input sample pointers (one per input channel)
     * @param output Output sample pointers (one per output channel)
     * @param numInputChannels Number of input channels
     * @param numSamples Number of samples to process
     * 
     * @note Channel handling:
     *       - If numInputChannels < delay line channels: input[0] is duplicated to remaining channels
     *       - If numInputChannels > delay line channels: extra input channels are ignored
     *       - Output always matches the delay line's channel count
     */
    void processBlock(const T* const* input, T* const* output, size_t numInputChannels, size_t numSamples)
    {
        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            const T* inChannel = input[ch < numInputChannels ? ch : 0];
            T* outChannel = output[ch];
            T* delayBuffer = buffer.getChannelPointer(ch);
            
            size_t workingWriteIndex = writeIndex;  // Local copy for this channel
            
            for (size_t i = 0; i < numSamples; ++i)
            {
                // Write input sample to buffer
                delayBuffer[workingWriteIndex] = inChannel[i];

                // Calculate read index with wrap-around (fixed delay)
                size_t readIndex = (workingWriteIndex + bufferSize - delaySamples) & (bufferSize - 1);

                // Read output sample directly (no interpolation needed for fixed integer delay)
                outChannel[i] = delayBuffer[readIndex];
                
                // Increment and wrap write index
                workingWriteIndex = (workingWriteIndex + 1) & (bufferSize - 1);
            }
        }
        
        // Update global write index after processing all channels
        writeIndex = (writeIndex + numSamples) & (bufferSize - 1);
    }

    /**
     * @brief Process a block of samples for all channels with modulated delay.
     * @param input Input sample pointers (one per input channel)
     * @param output Output sample pointers (one per output channel)
     * @param modulation Modulation signal pointers (one per channel, in samples)
     * @param numInputChannels Number of input channels
     * @param numSamples Number of samples to process
     * 
     * @note The modulation signal is added to the base delay time. 
     *       Modulation values should be in samples and will be clamped to valid range [0, bufferSize-1].
     * 
     * @note Channel handling:
     *       - If numInputChannels < delay line channels: input[0] and modulation[0] are duplicated
     *       - If numInputChannels > delay line channels: extra input channels are ignored
     *       - Output always matches the delay line's channel count
     */
    void processBlock(const T* const* input, T* const* output, 
                     const T* const* modulation, size_t numInputChannels, size_t numSamples)
    {
        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            const T* inChannel = input[ch < numInputChannels ? ch : 0];
            T* outChannel = output[ch];
            const T* modChannel = modulation[ch < numInputChannels ? ch : 0];
            T* delayBuffer = buffer.getChannelPointer(ch);
            
            size_t workingWriteIndex = writeIndex;  // Local copy for this channel
            
            for (size_t i = 0; i < numSamples; ++i)
            {
                // Write input sample to buffer
                delayBuffer[workingWriteIndex] = inChannel[i];

                // Calculate modulated delay time (base delay + modulation)
                T modulatedDelay = static_cast<T>(delaySamples) + modChannel[i];
                
                // Clamp modulated delay to valid range
                modulatedDelay = std::max(T(0), std::min(modulatedDelay, static_cast<T>(bufferSize - 1)));
                
                // Split into integer and fractional parts for interpolation
                size_t delayInt = static_cast<size_t>(modulatedDelay);
                T delayFrac = modulatedDelay - static_cast<T>(delayInt);
                
                // Calculate read index with wrap-around
                size_t readIndex = (workingWriteIndex + bufferSize - delayInt) & (bufferSize - 1);

                // Interpolate output sample
                outChannel[i] = Interpolator::interpolate(delayBuffer, readIndex, delayFrac);
                
                // Increment and wrap write index
                workingWriteIndex = (workingWriteIndex + 1) & (bufferSize - 1);
            }
        }
        
        // Update global write index after processing all channels
        writeIndex = (writeIndex + numSamples) & (bufferSize - 1);
    }

private:
    MultiChannelBuffer<T> buffer;  // Multi-channel circular buffer
    size_t writeIndex;             // Current write index in the buffer
    size_t bufferSize;             // Maximum delay in samples (always power of two)
    size_t delaySamples = 0;       // Current delay time in samples
    size_t numChannels;            // Number of audio channels

};
} // namespace Jonssonic

