// Jonssonic - A C++ audio DSP library
// Delay line class header file
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/core/delays/detail/interpolators.h"
#include "jonssonic/utils/detail/config_utils.h"
#include <jonssonic/core/common/circular_audio_buffer.h>
#include <jonssonic/core/common/dsp_param.h>
#include <jonssonic/core/common/quantities.h>
#include <jonssonic/utils/math_utils.h>

namespace jnsc {
/**
 * @brief A multichannel delay line class for audio processing with fractional delay support
 * @param T Sample data type (e.g., float, double)
 * @param Interpolator Interpolator type for fractional delay support (default: LinearInterpolator)
 */
template <typename T, typename Interpolator = detail::LinearInterpolator<T>>
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
        // Store config parameters with clamping
        numChannels = utils::detail::clampChannels(newNumChannels);
        sampleRate = utils::detail::clampSampleRate(newSampleRate);

        // Prepare circular buffer based on max delay time
        size_t maxDelaySamples = newMaxDelay.toSamples(sampleRate); // convert to samples
        circularBuffer.resize(numChannels, maxDelaySamples + 1); // resize circular buffer (buffer size = max delay + 1)

        // Prepare parameter smoother for delay time control
        delaySamples.prepare(numChannels, sampleRate);

        // Set bounds for delay time
        delaySamples.setBounds(T(0), static_cast<T>(maxDelaySamples));

        // Mark as prepared
        togglePrepared = true;
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
        // Write input sample to buffer (advances write position)
        circularBuffer.write(ch, input);

        // Read output sample with interpolation
        T output = Interpolator::interpolate(circularBuffer, ch, delaySamples.getNextValue(ch));

        return output;
    }

    /**
     * @brief Process a single sample for a specific channel with modulated delay.
     * @param ch Channel index
     * @param input Input sample
     * @param modulation Modulation value in samples to be added to base delay
     * @return Delayed output sample
     */

    T processSample(size_t ch, T input, T modulation) {
        // Write input sample to buffer (advances write position)
        circularBuffer.write(ch, input);

        // Calculate modulated delay time with internal clamping (base delay + modulation)
        T modulatedDelay = delaySamples.applyAdditiveMod(ch, modulation);

        // Read output sample with interpolation
        T output = Interpolator::interpolate(circularBuffer, ch, modulatedDelay);

        return output;
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
     * @note The modulation signal is added to the base delay time and clamped internally.
     * @note Input, output, and modulation must all have the same number of channels as prepared.
     */
    void processBlock(const T* const* input, T* const* output, const T* const* modulation, size_t numSamples) {
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
        if (!togglePrepared)
            return;
        delaySamples.setTarget(newDelay.toSamples(sampleRate), skipSmoothing);
    }

    /**
     * @brief Set a constant delay time in milliseconds for a specific channel.
     * @param ch Channel index
     * @param newDelay Delay time struct.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setDelay(size_t ch, Time<T> newDelay, bool skipSmoothing = false) {
        if (!togglePrepared)
            return;
        delaySamples.setTarget(ch, newDelay.toSamples(sampleRate), skipSmoothing);
    }

    /**
     * @brief Read a delayed sample from the buffer without writing.
     * @param ch Channel index
     * @return Delayed output sample
     */
    T readSample(size_t ch) { return Interpolator::interpolate(circularBuffer, ch, delaySamples.getNextValue(ch)); }

    /**
     * @brief Read a delayed sample from the buffer with modulated delay without writing.
     * @param ch Channel index
     * @param modulation Modulation value in samples to be added to base delay
     * @return Delayed output sample
     */
    T readSample(size_t ch, T modulation) {
        // Calculate modulated delay time with internal clamping (base delay + modulation)
        T modulatedDelay = delaySamples.applyAdditiveMod(ch, modulation);

        // Read output sample with interpolation
        return Interpolator::interpolate(circularBuffer, ch, modulatedDelay);
    }

    /**
     * @brief Write a sample to the delay line and advance the write position.
     * @param ch Channel index
     * @param input Input sample to write
     */
    void writeSample(size_t ch, T input) { circularBuffer.write(ch, input); }

    /// Get target delay time for specific channel
    Time<T> getTargetDelay(size_t ch) const { return Time<T>::Samples(delaySamples.getTargetValue(ch)); }

    /// Get current delay time for specific channel
    Time<T> getCurrentDelay(size_t ch) const { return Time<T>::Samples(delaySamples.getCurrentValue(ch)); }

    /// Get current sample rate
    T getSampleRate() const { return sampleRate; }

    /// Get number of channels
    size_t getNumChannels() const { return numChannels; }

  private:
    // Config variables
    T sampleRate = T(44100); // Sample rate in Hz
    size_t numChannels;      // Number of audio channels
    bool togglePrepared = false;

    // DSP Components
    CircularAudioBuffer<T> circularBuffer; // Multi-channel circular buffer
    DspParam<T> delaySamples;              // Multi-channel Delay time in samples
};

/**
 * @brief Type aliases for common delay line configurations with different interpolators.
 * @tparam T Sample data type (e.g., float, double)
 * @param NearestDelayLine: Delay line using nearest neighbor interpolation (no fractional delay support)
 * @param LinearDelayLine: Delay line using linear interpolation (basic fractional delay support)
 * @param LagrangeDelayLine: Delay line using 4-point Lagrange interpolation (higher-quality fractional delay support)
 */
template <typename T>
using NearestDelayLine = DelayLine<T, detail::NearestInterpolator<T>>;
template <typename T>
using LinearDelayLine = DelayLine<T, detail::LinearInterpolator<T>>;
template <typename T>
using LagrangeDelayLine = DelayLine<T, detail::LagrangeInterpolator<T>>;

} // namespace jnsc
