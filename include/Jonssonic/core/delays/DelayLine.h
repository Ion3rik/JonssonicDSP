// Jonssonic - A C++ audio DSP library
// Delay line class header file
// Author: Jon Fagerström
// Update: 18.11.2025

#pragma once
#include "../common/AudioBuffer.h"
#include "../../utils/MathUtils.h"
#include "../common/Interpolators.h"
#include "../common/DspParam.h"


namespace Jonssonic
{
/**
 * @brief A multichannel delay line class for audio processing with fractional delay support
 * @param T Sample data type (e.g., float, double)
 * @param Interpolator Interpolator type for fractional delay support (default: LinearInterpolator)
 * @param SmootherType Type of smoother for delay time modulation (default: OnePole)
 * @param SmootherOrder Order of the smoother for delay time modulation (default: 1)
 * @param SmoothingTimeMs Smoothing time in milliseconds for delay time changes (default: 10ms)
 */
template<
    typename T,
    typename Interpolator = LinearInterpolator<T>,
    SmootherType SmootherType = SmootherType::OnePole,
    int SmootherOrder = 1,
    int SmoothingTimeMs = 10
>
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

    void prepare(size_t newNumChannels, T newSampleRate, T newMaxDelayMs)
    {
        numChannels = newNumChannels;
        sampleRate = newSampleRate;
        size_t maxDelaySamples = msToSamples(newMaxDelayMs, sampleRate); // convert ms to samples
        bufferSize = nextPowerOfTwo(maxDelaySamples); // ensure power-of-two size for efficient wrap-around
        buffer.resize(numChannels, bufferSize); // resize buffer to number of channels and buffer size
        buffer.clear(); // initialize buffer to zero
        writeIndex.assign(numChannels, 0); // always reset write indices to zero
        delaySamples.prepare(numChannels, newSampleRate, SmoothingTimeMs); // prepare smoother for delay time
        delaySamples.setBounds(T(0), static_cast<T>(maxDelaySamples)); // set bounds to actual max delay requested
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

        // Calculate read index and fractional part for interpolation
        auto [readIndex, delayFrac] = computeReadIndexAndFrac(delaySamples.getNextValue(ch), writeIndex[ch]);

        // Increment and wrap with bitwise AND for power-of-two buffer size
        writeIndex[ch] = (writeIndex[ch] + 1) & (bufferSize - 1);

        // Read output sample with interpolation
        return Interpolator::interpolateBackward(buffer.readChannelPtr(ch), readIndex, delayFrac, bufferSize);
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

        // Calculate modulated delay time with internal clamping (base delay + modulation)
        T modulatedDelay = delaySamples.applyAdditiveMod(modulation, ch);
        
        // Split into integer and fractional parts for interpolation
        auto [readIndex, delayFrac] = computeReadIndexAndFrac(modulatedDelay, writeIndex[ch]);

        // Increment and wrap write index
        writeIndex[ch] = (writeIndex[ch] + 1) & (bufferSize - 1);

        // Interpolate output sample
        return Interpolator::interpolateBackward(buffer.readChannelPtr(ch), readIndex, delayFrac, bufferSize);
    

    }

    /**
     * @brief Process a block of samples for all channels.
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

                // Calculate read and fractional part for interpolation
                auto [readIndex, delayFrac] = computeReadIndexAndFrac(delaySamples.getNextValue(ch), writeIndex[ch]);

                // Read output sample with interpolation
                output[ch][i] = Interpolator::interpolateBackward(buffer.readChannelPtr(ch), readIndex, delayFrac, bufferSize);
                
                // Increment and wrap write index with bitwise AND for power-of-two buffer size
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

                // Calculate modulated delay time with internal clamping (base delay + modulation)
                T modulatedDelay = delaySamples.applyAdditiveMod(modulation[ch][i], ch);
                
                // Calculate read index and fractional part for interpolation
                auto [readIndex, delayFrac] = computeReadIndexAndFrac(modulatedDelay, writeIndex[ch]);

                // Interpolate output sample
                output[ch][i] = Interpolator::interpolateBackward(buffer.readChannelPtr(ch), readIndex, delayFrac, bufferSize);
                
                // Increment and wrap write index
                writeIndex[ch] = (writeIndex[ch] + 1) & (bufferSize - 1);
            }
        }
    }

    /**
     * @brief Set a constant delay time in milliseconds for all channels.
     * @param newDelayMs Delay time in milliseconds
     */
    void setDelayMs(T newDelayMs, bool skipSmoothing = false)
    {
        T newDelaySamples = msToSamples(newDelayMs, sampleRate); // convert ms to samples  
        delaySamples.setTarget(newDelaySamples, skipSmoothing);
    }

    /**
     * @brief Set a constant delay time in milliseconds for a specific channel.
     * @param ch Channel index
     * @param newDelayMs Delay time in milliseconds
     */
    void setDelayMs(size_t ch, T newDelayMs, bool skipSmoothing = false)
    {
        T newDelaySamples = msToSamples(newDelayMs, sampleRate); // convert ms to samples
        delaySamples.setTarget(ch, newDelaySamples, skipSmoothing);              
    }

    /**
     * @brief Set a constant delay time in samples for all channels.
     * @param newDelaySamples Delay time in samples
     */
    void setDelaySamples(T newDelaySamples, bool skipSmoothing = false)
    {
        delaySamples.setTarget(newDelaySamples, skipSmoothing);
    }

    /**
     * @brief Set a constant delay time in samples for a specific channel.
     * @param ch Channel index
     * @param newDelaySamples Delay time in samples
     */
    void setDelaySamples(size_t ch, T newDelaySamples, bool skipSmoothing = false)
    {
        delaySamples.setTarget(ch, newDelaySamples, skipSmoothing);
    }


private:
    T sampleRate = T(44100);                                // Sample rate in Hz
    AudioBuffer<T> buffer;                                  // Multi-channel circular buffer
    std::vector<size_t> writeIndex;                         // Current write index in the buffer per channel
    size_t bufferSize;                                      // Maximum delay in samples (always power of two)
    DspParam<T, SmootherType, SmootherOrder> delaySamples;  // Delay time in samples per channel
    size_t numChannels;                                     // Number of audio channels

    // Helper function to compute read index and fractional part for interpolation
    std::pair<size_t, T> computeReadIndexAndFrac(T delay, size_t writeIdx) const
    {
        // Clamp delay to valid range as safety measure
        delay = std::max(T(0), std::min(delay, static_cast<T>(bufferSize - 1)));
        
        // Split into integer and fractional parts using floor for proper handling
        T delayFloor = std::floor(delay);
        size_t delayInt = static_cast<size_t>(delayFloor);
        T delayFrac = delay - delayFloor;
    
        // Calculate read index with wrap-around
        size_t readIndex = (writeIdx + bufferSize - delayInt) & (bufferSize - 1);

        return {readIndex, delayFrac};
    }
};
} // namespace Jonssonic

