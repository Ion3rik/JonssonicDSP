// Jonssonic - A C++ audio DSP library
// Delay line class header file
// SPDX-License-Identifier: MIT
// TO DO: Seprate a lower level DelayCore class without modulation nor smoothing?

#pragma once
#include <jonssonic/core/common/circular_audio_buffer.h>
#include <jonssonic/utils/math_utils.h>
#include <jonssonic/core/common/interpolators.h>
#include <jonssonic/core/common/dsp_param.h>

namespace jonssonic::core::delays
{
/**
 * @brief A multichannel delay line class for audio processing with fractional delay support
 * @param T Sample data type (e.g., float, double)
 * @param Interpolator Interpolator type for fractional delay support (default: LinearInterpolator)
 * @param SmootherType Type of smoother for delay time modulation (default: OnePole)
 * @param SmootherOrder Order of the smoother for delay time modulation (default: 1)
 * @param Layout Buffer layout type (Planar or Interleaved, default: Planar)
 */
template<
    typename T,
    typename Interpolator = common::LinearInterpolator<T>,
    common::SmootherType SmootherType = common::SmootherType::OnePole,
    int SmootherOrder = 1
>
class DelayLine
{
/// Type aliases for convenience, readability, and future-proofing
using CircularAudioBufferType =  common::CircularAudioBuffer<T>;
using DspParamType = common::DspParam<T, SmootherType, SmootherOrder>;
public:
    /// Default constructor
    DelayLine() = default;

    /**
     * @brief Parmetrized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     * @param newMaxDelayMs Maximum delay time in milliseconds
     * @param newSmoothingTimeMs Smoothing time for delay time control in milliseconds
     */
    DelayLine(size_t newNumChannels, T newSampleRate, T newMaxDelayMs)
    {
        prepare(newNumChannels, newSampleRate, newMaxDelayMs);
    }

    /// Default destructor
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
        size_t maxDelaySamples = utils::msToSamples(newMaxDelayMs, sampleRate); // convert ms to samples
        circularBuffer.resize(numChannels, maxDelaySamples); // resize circular buffer
        bufferSize = circularBuffer.getBufferSize(); // get actual buffer size (power of two)
        delaySamples.prepare(numChannels, newSampleRate); // prepare smoother for delay time
        delaySamples.setBounds(T(0), static_cast<T>(maxDelaySamples)); // set bounds to actual max delay requested
    }

    /// Reset the delay line state
    void reset()
    {
        circularBuffer.clear();
        delaySamples.reset();
    }

    /**
     * @brief Process a single sample for a specific channel.
     * @param ch Channel index
     * @param input Input sample
     * @return Delayed output sample
     */
    T processSample(size_t ch, T input)
    {
        // Calculate integer read index and fractional part for interpolation
        auto [readIndex, delayFrac] = computeReadIndexAndFrac(delaySamples.getNextValue(ch), circularBuffer.getChannelWriteIndex(ch));

        // Write input sample to buffer (increments and wraps internally)
        circularBuffer.write(ch, input);

        // Read output sample with interpolation
        return Interpolator::interpolateBackward(circularBuffer.readChannelPtr(ch), readIndex, delayFrac, bufferSize);
    }

    /**
    * @brief Process a single sample for a specific channel with modulated delay.
    * @param ch Channel index
    * @param input Input sample
    * @param modulation Modulation value in samples to be added to base delay
    * @return Delayed output sample
    */
   
    T processSample(size_t ch, T input, T modulation)
    {
        // Calculate modulated delay time with internal clamping (base delay + modulation)
        T modulatedDelay = delaySamples.applyAdditiveMod(ch, modulation);
        
        // Split into integer and fractional parts for interpolation
        auto [readIndex, delayFrac] = computeReadIndexAndFrac(modulatedDelay, circularBuffer.getChannelWriteIndex(ch));
        
        // Write input sample to buffer (increments and wraps internally)
        circularBuffer.write(ch, input);

        // Interpolate output sample
        return Interpolator::interpolateBackward(circularBuffer.readChannelPtr(ch), readIndex, delayFrac, bufferSize);
    

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
            for (size_t n = 0; n < numSamples; ++n)
                output[ch][n] = processSample(ch, input[ch][n]);
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
            for (size_t n = 0; n < numSamples; ++n)
                output[ch][n] = processSample(ch, input[ch][n], modulation[ch][n]);
    }

    /**
     * @brief Set the parameter smoothing time in milliseconds for delay time changes.
     * @param timeMs Smoothing time in milliseconds
     */
    void setSmoothingTimeMs(T timeMs)
    {
        delaySamples.setSmoothingTimeMs(timeMs);
    }

    /**
     * @brief Set a constant delay time in milliseconds for all channels.
     * @param newDelayMs Delay time in milliseconds
     */
    void setDelayMs(T newDelayMs, bool skipSmoothing = false)
    {
        T newDelaySamples = utils::msToSamples(newDelayMs, sampleRate); // convert ms to samples  
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

    /**
     * @brief Read a delayed sample from the buffer without writing.
     * @param ch Channel index
     * @return Delayed output sample
     */
    T readSample(size_t ch)
    {
        // Calculate read index and fractional part for interpolation
        auto [readIndex, delayFrac] = computeReadIndexAndFrac(delaySamples.getNextValue(ch), circularBuffer.getChannelWriteIndex(ch));

        // Read output sample with interpolation
        return Interpolator::interpolateBackward(circularBuffer.readChannelPtr(ch), readIndex, delayFrac, bufferSize);
    }

    /**
     * @brief Read a delayed sample from the buffer with modulated delay without writing.
     * @param ch Channel index
     * @param modulation Modulation value in samples to be added to base delay
     * @return Delayed output sample
     */
    T readSample(size_t ch, T modulation)
    {
        // Calculate modulated delay time with internal clamping (base delay + modulation)
        T modulatedDelay = delaySamples.applyAdditiveMod(modulation, ch);
        
        // Calculate read index and fractional part for interpolation
        auto [readIndex, delayFrac] = computeReadIndexAndFrac(modulatedDelay, circularBuffer.getChannelWriteIndex(ch));

        // Read output sample with interpolation
        return Interpolator::interpolateBackward(circularBuffer.readChannelPtr(ch), readIndex, delayFrac, bufferSize);
    }

    /**
     * @brief Write a sample to the circular buffer and advance the write position.
     * @param ch Channel index
     * @param input Input sample to write
     */
    void writeSample(size_t ch, T input)
    {
        circularBuffer.write(ch, input);
    }


private:
    // Global parameters
    T sampleRate = T(44100);                                // Sample rate in Hz
    size_t numChannels;                                     // Number of audio channels
    size_t bufferSize;                                      // Maximum delay in samples (always power of two)

    // DSP Components
    CircularAudioBufferType circularBuffer;                  // Multi-channel circular buffer
    DspParamType delaySamples;  // Multi-channel Delay time in samples


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

