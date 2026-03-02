// Jonssonic - A C++ audio DSP library
// Multi tap delay line class header file
// SPDX-License-Identifier: MIT

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
 * @param T Sample data type (e.g., float, double).
 * @param NumTaps Number of delay taps.
 * @param Interpolator Interpolator type for fractional delay support (default: LinearInterpolator).
 */
template <typename T, size_t NumTaps, typename Interpolator = LinearInterpolator<T>>
class MultiTapDelayLine {
  public:
    /// Default constructor
    MultiTapDelayLine() = default;

    /**
     * @brief Parmetrized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     * @param newMaxDelay Maximum delay time in various units.
     */
    MultiTapDelayLine(size_t newNumChannels, T newSampleRate, Time<T> newMaxDelay) {
        prepare(newNumChannels, newSampleRate, newMaxDelay);
    }

    /// Default destructor
    ~MultiTapDelayLine() = default;

    /// No copy semantics nor move semantics
    MultiTapDelayLine(const MultiTapDelayLine&) = delete;
    const MultiTapDelayLine& operator=(const MultiTapDelayLine&) = delete;
    MultiTapDelayLine(MultiTapDelayLine&&) = delete;
    const MultiTapDelayLine& operator=(MultiTapDelayLine&&) = delete;

    /**
     * @brief Prepare the multi tap delay line for processing.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     * @param newMaxDelay Maximum delay time in various units.
     * @param newNumTaps Number of delay taps (default is 2 for a simple multi-tap)
     */
    void prepare(size_t newNumChannels, T newSampleRate, Time<T> newMaxDelay) {
        // Store config parameters with clamping
        numChannels = utils::detail::clampChannels(newNumChannels);
        sampleRate = utils::detail::clampSampleRate(newSampleRate);

        // Prepare circular buffer based on max delay time
        size_t maxDelaySamples = newMaxDelay.toSamples(sampleRate); // convert to samples
        circularBuffer.resize(numChannels, maxDelaySamples);        // resize circular buffer
        bufferSize = circularBuffer.getBufferSize();                // get actual buffer size (power of two)

        // Prepare DSP parameters for tap delay times and gains
        tapDelay.prepare(numChannels * NumTaps, sampleRate);
        tapGain.prepare(numChannels * NumTaps, sampleRate);

        // Set default bounds for delay times and gains
        tapDelay.setBounds(T(0), static_cast<T>(bufferSize - 1));
        tapGain.setBounds(T(-1), T(1));

        // Mark as prepared
        togglePrepared = true;
    }

    /// Reset the delay line state
    void reset() {
        circularBuffer.clear();
        tapDelay.reset();
        tapGain.reset();
    }

    /**
     * @brief Process a single sample for a specific channel.
     * @param ch Channel index
     * @param input Input sample
     * @return Output sample
     * @note This method uses no interpolation.
     */
    T processSample(size_t ch, T input) {

        // Accumulate output from all taps
        T output = T(0);
        for (size_t tap = 0; tap < NumTaps; ++tap)
            output += tapGain.getNextValue(index(ch, tap)) *
                      circularBuffer.read(ch, static_cast<size_t>(tapDelay.getNextValue(index(ch, tap))));

        // Write input sample to buffer (increments and wraps internally)
        circularBuffer.write(ch, input);

        // Return the mixed output from all taps
        return output;
    }

    /**
     * @brief Process a single sample for a specific channel with modulated delay.
     * @param ch Channel index
     * @param input Input sample
     * @param modulation Array of modulation values in samples to be added to base delay for each tap
     * @return Output sample
     */

    T processSample(size_t ch, T input, const std::array<T, NumTaps>& modulation) {
        // Accumulate output from all taps with modulated delay
        T output = T(0);
        for (size_t tap = 0; tap < NumTaps; ++tap) {
            // Calculate modulated delay time with internal clamping (base delay + modulation)
            T modulatedDelay = tapDelay.applyAdditiveMod(index(ch, tap), modulation[tap]);

            // Split into integer and fractional parts for interpolation
            size_t readIndex;
            T delayFrac;
            computeReadIndexAndFrac(modulatedDelay, circularBuffer.getChannelWriteIndex(ch), readIndex, delayFrac);

            // Accumulate output from this tap with gain
            output +=
                tapGain.getNextValue(index(ch, tap)) *
                Interpolator::interpolateBackward(circularBuffer.readChannelPtr(ch), readIndex, delayFrac, bufferSize);
        }
        // Write input sample to buffer (increments and wraps internally)
        circularBuffer.write(ch, input);
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
     * @param modulation Array of modulation signal pointers (one per channel, each containing an array of modulation
     * values for each tap in samples)
     * @param numSamples Number of samples to process
     *
     * @note The modulation signal is added to the base delay time.
     *       Modulation values should be in samples and will be clamped to valid range [0,
     * bufferSize-1].
     * @note Input, output, and modulation must all have the same number of channels as prepared.
     */
    void processBlock(const T* const* input,
                      T* const* output,
                      const std::array<T, NumTaps>* const* modulation,
                      size_t numSamples) {
        for (size_t ch = 0; ch < numChannels; ++ch)
            for (size_t n = 0; n < numSamples; ++n)
                output[ch][n] = processSample(ch, input[ch][n], modulation[ch][n]);
    }

    /**
     * @brief Set the control smoothing time in various units.
     * @param time Time value representing the smoothing time
     */
    void setControlSmoothingTime(Time<T> time) {
        tapDelay.setSmoothingTime(time);
        tapGain.setSmoothingTime(time);
    }

    /**
     * @brief Set the delay time for a specific tap and all channels.
     * @param tap Tap index
     * @param newDelay Delay time struct.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setTapDelay(size_t tap, Time<T> newDelay, bool skipSmoothing = false) {
        assert(tap < NumTaps && "Tap index out of range");
        if (!togglePrepared)
            return;
        for (size_t ch = 0; ch < numChannels; ++ch)
            tapDelay.setTarget(index(ch, tap), newDelay.toSamples(sampleRate), skipSmoothing);
    }

    /**
     * @brief Set the delay time for a specific channel and tap.
     * @param ch Channel index
     * @param tap Tap index
     * @param newDelay Delay time struct.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setTapDelay(size_t ch, size_t tap, Time<T> newDelay, bool skipSmoothing = false) {
        assert(tap < NumTaps && "Tap index out of range");
        assert(ch < numChannels && "Channel index out of range");
        if (!togglePrepared)
            return;
        tapDelay.setTarget(index(ch, tap), newDelay.toSamples(sampleRate), skipSmoothing);
    }

    /**
     * @brief Set the gain for a specific tap and all channels.
     * @param tap Tap index
     * @param newGain Gain struct.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setTapGain(size_t tap, Gain<T> newGain, bool skipSmoothing = false) {
        assert(tap < NumTaps && "Tap index out of range");
        if (!togglePrepared)
            return;
        for (size_t ch = 0; ch < numChannels; ++ch)
            tapGain.setTarget(index(ch, tap), newGain.toLinear(), skipSmoothing);
    }

    /**
     * @brief Set the gain for a specific channel and tap.
     * @param ch Channel index
     * @param tap Tap index
     * @param newGain Gain struct.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setTapGain(size_t ch, size_t tap, Gain<T> newGain, bool skipSmoothing = false) {
        assert(tap < NumTaps && "Tap index out of range");
        assert(ch < numChannels && "Channel index out of range");
        if (!togglePrepared)
            return;
        tapGain.setTarget(index(ch, tap), newGain.toLinear(), skipSmoothing);
    }

    /**
     * @brief Read all taps for a specific channel and mix them together without writing.
     * @param ch Channel index
     * @return Delayed output sample
     * @note This method uses no interpolation.
     */
    T readSample(size_t ch) {
        T output = T(0);
        for (size_t tap = 0; tap < NumTaps; ++tap)
            output += tapGain.getNextValue(index(ch, tap)) *
                      circularBuffer.read(ch, static_cast<size_t>(tapDelay.getNextValue(index(ch, tap))));

        return output;
    }

    /**
     * @brief Read all taps for a specific channel with modulated delay and mix them together without writing.
     * @param ch Channel index
     * @param modulation Array of modulation values in samples to be added to base delay for each tap
     * @return Delayed output sample
     */
    T readSample(size_t ch, const std::array<T, NumTaps>& modulation) {
        T output = T(0);
        for (size_t tap = 0; tap < NumTaps; ++tap) {
            // Calculate modulated delay time with internal clamping (base delay + modulation)
            T modulatedDelay = tapDelay.applyAdditiveMod(index(ch, tap), modulation[tap]);

            // Calculate read index and fractional part for interpolation
            size_t readIndex;
            T delayFrac;
            computeReadIndexAndFrac(modulatedDelay, circularBuffer.getChannelWriteIndex(ch), readIndex, delayFrac);

            // Read output sample with interpolation
            output +=
                tapGain.getNextValue(index(ch, tap)) *
                Interpolator::interpolateBackward(circularBuffer.readChannelPtr(ch), readIndex, delayFrac, bufferSize);
        }
        return output;
    }

    /**
     * @brief Write a sample to the circular buffer and advance the write position.
     * @param ch Channel index
     * @param input Input sample to write
     */
    void writeSample(size_t ch, T input) { circularBuffer.write(ch, input); }

    /// Get target delay time for specific channel and tap
    Time<T> getTargetTapDelay(size_t ch, size_t tap) const {
        assert(ch < numChannels && "Channel index out of range");
        assert(tap < NumTaps && "Tap index out of range");
        return Time<T>::Samples(tapDelay.getTargetValue(index(ch, tap)));
    }

    /// Get current delay time for specific channel, and tap
    Time<T> getCurrentTapDelay(size_t ch, size_t tap = 0) const {
        assert(ch < numChannels && "Channel index out of range");
        assert(tap < NumTaps && "Tap index out of range");
        return Time<T>::Samples(tapDelay.getCurrentValue(index(ch, tap)));
    }

    /// Get target gain for specific channel and tap
    Gain<T> getTargetTapGain(size_t ch, size_t tap) const {
        assert(ch < numChannels && "Channel index out of range");
        assert(tap < NumTaps && "Tap index out of range");
        return Gain<T>::Linear(tapGain.getTargetValue(index(ch, tap)));
    }

    /// Get current gain for specific channel and tap
    Gain<T> getCurrentTapGain(size_t ch, size_t tap) const {
        assert(ch < numChannels && "Channel index out of range");
        assert(tap < NumTaps && "Tap index out of range");
        return Gain<T>::Linear(tapGain.getCurrentValue(index(ch, tap)));
    }

  private:
    // Config variables
    T sampleRate = T(44100); // Sample rate in Hz
    size_t numChannels;      // Number of audio channels
    size_t bufferSize;       // Maximum delay in samples (always power of two)
    size_t numTaps;          // Number of delay taps
    bool togglePrepared = false;

    // DSP Components
    CircularAudioBuffer<T> circularBuffer; // Multi-channel circular buffer
    DspParam<T> tapDelay;                  // Multi-channel, multi-tap delay time in samples
    DspParam<T> tapGain;                   // Multi-channel, multi-tap gain for each tap

    // Helper function to compute read index and fractional part for interpolation (in-place
    // version)
    void computeReadIndexAndFrac(T delay, size_t writeIdx, size_t& readIndex, T& delayFrac) const {
        // Split into integer and fractional parts using floor for proper handling
        T delayFloor = std::floor(delay);
        size_t delayInt = static_cast<size_t>(delayFloor);
        delayFrac = delay - delayFloor;

        // Calculate read index with wrap-around
        readIndex = (writeIdx + bufferSize - delayInt) & (bufferSize - 1);
    }

    // Helper function to calculate parameter index for multi-tap delay (where taps are stored contiguously per channel)
    inline size_t index(size_t ch, size_t tap) const { return ch * numTaps + tap; }
};
} // namespace jnsc
