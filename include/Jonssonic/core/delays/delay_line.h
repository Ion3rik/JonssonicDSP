// Jonssonic - A C++ audio DSP library
// Delay line class header file
// SPDX-License-Identifier: MIT
// TO DO: Seprate a lower level DelayCore class without modulation nor smoothing?

#pragma once
#include "jonssonic/utils/detail/config_utils.h"
#include <jonssonic/core/common/circular_audio_buffer.h>
#include <jonssonic/core/common/dsp_param.h>
#include <jonssonic/core/common/interpolators.h>
#include <jonssonic/core/common/quantities.h>
#include <jonssonic/utils/math_utils.h>

namespace jnsc {
/**
 * @brief A multichannel delay line class for audio processing with fractional delay support
 * @param T Sample data type (e.g., float, double)
 * @param Interpolator Interpolator type for fractional delay support (default: LinearInterpolator)
 */
template <typename T, typename Interpolator = LinearInterpolator<T>>
class DelayLine {
  public:
    /// Default constructor
    DelayLine() = default;

    /**
     * @brief Parmetrized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     * @param newMaxDelay Maximum delay time in various units.
     */
    DelayLine(size_t newNumChannels, T newSampleRate, Time<T> newMaxDelay) {
        prepare(newNumChannels, newSampleRate, newMaxDelay);
    }

    /// Default destructor
    ~DelayLine() = default;

    /// No copy semantics nor move semantics
    DelayLine(const DelayLine&) = delete;
    const DelayLine& operator=(const DelayLine&) = delete;
    DelayLine(DelayLine&&) = delete;
    const DelayLine& operator=(DelayLine&&) = delete;

    /**
     * @brief Prepare the delay line for processing.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     * @param newMaxDelay Maximum delay time in various units.
     */
    void prepare(size_t newNumChannels, T newSampleRate, Time<T> newMaxDelay) {
        numChannels = utils::detail::clampChannels(newNumChannels);
        sampleRate = utils::detail::clampSampleRate(newSampleRate);
        size_t maxDelaySamples = newMaxDelay.toSamples(sampleRate); // convert to samples
        circularBuffer.resize(numChannels, maxDelaySamples);        // resize circular buffer
        bufferSize = circularBuffer.getBufferSize();   // get actual buffer size (power of two)
        delaySamples.prepare(numChannels, sampleRate); // prepare smoother for delay time
        delaySamples.setBounds(
            T(0),
            static_cast<T>(maxDelaySamples)); // set bounds to actual max delay requested
    }

    /// Reset the delay line state
    void reset() {
        circularBuffer.clear();
        delaySamples.reset();
    }

    /**
     * @brief Process a single sample for a specific channel.
     * @param ch Channel index
     * @param input Input sample
     * @return Delayed output sample
     */
    T processSample(size_t ch, T input) {
        // Calculate integer read index and fractional part for interpolation
        size_t readIndex;
        T delayFrac;
        computeReadIndexAndFrac(delaySamples.getNextValue(ch),
                                circularBuffer.getChannelWriteIndex(ch),
                                readIndex,
                                delayFrac);

        // Write input sample to buffer (increments and wraps internally)
        circularBuffer.write(ch, input);

        // Read output sample with interpolation
        return Interpolator::interpolateBackward(circularBuffer.readChannelPtr(ch),
                                                 readIndex,
                                                 delayFrac,
                                                 bufferSize);
    }

    /**
     * @brief Process a single sample for a specific channel with modulated delay.
     * @param ch Channel index
     * @param input Input sample
     * @param modulation Modulation value in samples to be added to base delay
     * @return Delayed output sample
     */

    T processSample(size_t ch, T input, T modulation) {
        // Calculate modulated delay time with internal clamping (base delay + modulation)
        T modulatedDelay = delaySamples.applyAdditiveMod(ch, modulation);

        // Split into integer and fractional parts for interpolation
        size_t readIndex;
        T delayFrac;
        computeReadIndexAndFrac(modulatedDelay,
                                circularBuffer.getChannelWriteIndex(ch),
                                readIndex,
                                delayFrac);

        // Write input sample to buffer (increments and wraps internally)
        circularBuffer.write(ch, input);

        // Interpolate output sample
        return Interpolator::interpolateBackward(circularBuffer.readChannelPtr(ch),
                                                 readIndex,
                                                 delayFrac,
                                                 bufferSize);
    }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input sample pointers (one per channel)
     * @param output Output sample pointers (one per channel)
     * @param numSamples Number of samples to process
     *
     * @note Input and output must have the same number of channels as prepared.
     */
    void processBlock(const T* const* input, T* const* output, size_t numSamples) {
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
     *       Modulation values should be in samples and will be clamped to valid range [0,
     * bufferSize-1].
     * @note Input, output, and modulation must all have the same number of channels as prepared.
     */
    void processBlock(const T* const* input,
                      T* const* output,
                      const T* const* modulation,
                      size_t numSamples) {
        for (size_t ch = 0; ch < numChannels; ++ch)
            for (size_t n = 0; n < numSamples; ++n)
                output[ch][n] = processSample(ch, input[ch][n], modulation[ch][n]);
    }

    /**
     * @brief Set the control smoothing time in various units.
     * @param time Time value representing the smoothing time
     */
    void setControlSmoothingTime(Time<T> time) { delaySamples.setSmoothingTime(time); }

    /**
     * @brief Set the base delay time in various units for all channels.
     * @param newDelay Delay time struct.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setDelay(Time<T> newDelay, bool skipSmoothing = false) {
        delaySamples.setTarget(newDelay.toSamples(sampleRate), skipSmoothing);
    }

    /**
     * @brief Set a constant delay time in milliseconds for a specific channel.
     * @param ch Channel index
     * @param newDelay Delay time struct.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setDelay(size_t ch, Time<T> newDelay, bool skipSmoothing = false) {
        delaySamples.setTarget(ch, newDelay.toSamples(sampleRate), skipSmoothing);
    }

    /**
     * @brief Read a delayed sample from the buffer without writing.
     * @param ch Channel index
     * @return Delayed output sample
     */
    T readSample(size_t ch) {
        // Calculate read index and fractional part for interpolation
        size_t readIndex;
        T delayFrac;
        computeReadIndexAndFrac(delaySamples.getNextValue(ch),
                                circularBuffer.getChannelWriteIndex(ch),
                                readIndex,
                                delayFrac);

        // Read output sample with interpolation
        return Interpolator::interpolateBackward(circularBuffer.readChannelPtr(ch),
                                                 readIndex,
                                                 delayFrac,
                                                 bufferSize);
    }

    /**
     * @brief Read a delayed sample from the buffer with modulated delay without writing.
     * @param ch Channel index
     * @param modulation Modulation value in samples to be added to base delay
     * @return Delayed output sample
     */
    T readSample(size_t ch, T modulation) {
        // Calculate modulated delay time with internal clamping (base delay + modulation)
        T modulatedDelay = delaySamples.applyAdditiveMod(modulation, ch);

        // Calculate read index and fractional part for interpolation
        size_t readIndex;
        T delayFrac;
        computeReadIndexAndFrac(modulatedDelay,
                                circularBuffer.getChannelWriteIndex(ch),
                                readIndex,
                                delayFrac);

        // Read output sample with interpolation
        return Interpolator::interpolateBackward(circularBuffer.readChannelPtr(ch),
                                                 readIndex,
                                                 delayFrac,
                                                 bufferSize);
    }

    /**
     * @brief Write a sample to the circular buffer and advance the write position.
     * @param ch Channel index
     * @param input Input sample to write
     */
    void writeSample(size_t ch, T input) { circularBuffer.write(ch, input); }

  private:
    // Global parameters
    T sampleRate = T(44100); // Sample rate in Hz
    size_t numChannels;      // Number of audio channels
    size_t bufferSize;       // Maximum delay in samples (always power of two)

    // DSP Components
    CircularAudioBuffer<T> circularBuffer; // Multi-channel circular buffer
    DspParam<T> delaySamples;              // Multi-channel Delay time in samples

    // Helper function to compute read index and fractional part for interpolation (in-place
    // version)
    void computeReadIndexAndFrac(T delay, size_t writeIdx, size_t& readIndex, T& delayFrac) const {
        // Clamp delay to valid range as safety measure
        delay = std::max(T(0), std::min(delay, static_cast<T>(bufferSize - 1)));

        // Split into integer and fractional parts using floor for proper handling
        T delayFloor = std::floor(delay);
        size_t delayInt = static_cast<size_t>(delayFloor);
        delayFrac = delay - delayFloor;

        // Calculate read index with wrap-around
        readIndex = (writeIdx + bufferSize - delayInt) & (bufferSize - 1);
    }
};
} // namespace jnsc
