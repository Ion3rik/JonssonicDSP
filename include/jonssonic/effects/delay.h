// Jonssonic - A C++ audio DSP library
// Delay effect header file
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/utils/detail/config_utils.h"
#include <jonssonic/core/common/audio_buffer.h>
#include <jonssonic/core/generators/oscillator.h>
#include <jonssonic/models/delays/modulated_delay_stage.h>

namespace jnsc::effects {
/**
 * @brief Delay Effect with Feedback, Damping, and ping-pong cross-talk between channels.
 * @param T Sample data type (e.g., float, double)
 */
template <typename T>
class Delay {
    /**
     * @brief Tunable constants for the delay effect.
     * @param MAX_DELAY_MS Maximum delay in milliseconds.
     * @param MAX_MODULATION_MS Maximum delay time modulation depth in milliseconds.
     * @param WOW_PORTION_OF_MODULATION Portion of modulation depth for wow effect (rest for flutter).
     * @param SMOOTHING_TIME_MS Smoothing time for parameter changes in milliseconds.
     * @param DAMPING_MIN_HZ Minimum damping frequency in Hz.
     */
    static constexpr T MAX_DELAY_MS = T(2000);
    static constexpr T MAX_MODULATION_MS = T(3.0);
    static constexpr T WOW_PORTION_OF_MODULATION = T(0.8);
    static constexpr int SMOOTHING_TIME_MS = 300;
    static constexpr T DAMPING_MIN_HZ = T(2000);

  public:
    /// Default Constructor
    Delay() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     * @param maxDelayMs Maximum delay in milliseconds
     */
    Delay(size_t newNumChannels, size_t newMaxBlockSize, T newSampleRate) {
        prepare(newNumChannels, newMaxBlockSize, newSampleRate);
    }

    /// Default Destructor
    ~Delay() = default;

    /// No copy nor move semantics
    Delay(const Delay&) = delete;
    Delay& operator=(const Delay&) = delete;
    Delay(Delay&&) = delete;
    Delay& operator=(Delay&&) = delete;

    /**
     * @brief Prepare the delay effect for processing.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     * @param maxDelayMs Maximum delay in milliseconds
     */
    void prepare(size_t newNumChannels, size_t newMaxBlockSize, T newSampleRate) {
        // Store global parameters
        numChannels = utils::detail::clampChannels(newNumChannels);
        sampleRate = utils::detail::clampSampleRate(newSampleRate);

        // Prepare Modulated Delay Stage
        modulatedDelayStage.prepare(numChannels, Time<T>::Milliseconds(MAX_DELAY_MS), sampleRate);
        modulatedDelayStage.setControlSmoothingTime(Time<T>::Milliseconds(T(SMOOTHING_TIME_MS)));

        // Prepare Wow LFO
        wowLfo.prepare(newNumChannels, newSampleRate);
        wowLfo.setControlSmoothingTime(Time<T>::Milliseconds(T(SMOOTHING_TIME_MS)));
        wowLfo.setWaveform(Waveform::Sine);
        wowLfo.setFrequency(Frequency<T>::Hertz(0.3)); // 0.3 Hz for wow

        // Prepare Flutter LFO
        flutterLfo.prepare(newNumChannels, newSampleRate);
        flutterLfo.setControlSmoothingTime(Time<T>::Milliseconds(T(SMOOTHING_TIME_MS)));
        flutterLfo.setWaveform(Waveform::Sine);
        flutterLfo.setFrequency(Frequency<T>::Hertz(6.0)); // 6 Hz for flutter

        // Prepare modulation buffer
        modulationBuffer.resize(numChannels, newMaxBlockSize);

        // Set default parameter values
        setFeedback(T(0), true);
        setPingPong(T(0), true);
        setModDepth(T(0), true);
        setDelayMs(T(500), true);
        setDamping(T(0), true);
    }

    void reset() {
        modulatedDelayStage.reset();
        flutterLfo.reset();
        wowLfo.reset();
        modulationBuffer.clear();
    }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input buffer (numChannels x numSamples)
     * @param output Output buffer (numChannels x numSamples)
     * @param numSamples Number of samples to process
     */
    void processBlock(const T* const* input, T* const* output, size_t numSamples) {
        // Process LFOs to modulation buffer (note: could implement oscillator bank to internalize additive oscillators)
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t n = 0; n < numSamples; ++n) {
                T wowValue = wowLfo.processSample(ch);
                T flutterValue = flutterLfo.processSample(ch);
                modulationBuffer[ch][n] =
                    wowValue * WOW_PORTION_OF_MODULATION + flutterValue * (T(1) - WOW_PORTION_OF_MODULATION);
            }
        }

        // Process delay stage with external modulation
        modulatedDelayStage.processBlock(input, output, modulationBuffer.readPtrs(), numSamples);
    }

    /**
     * @brief Set the delay time in milliseconds.
     * @param newDelayMs New delay time in milliseconds.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setDelayMs(T newDelayMs, bool skipSmoothing = false) {
        modulatedDelayStage.setDelay(Time<T>::Milliseconds(newDelayMs), skipSmoothing);
    }

    /**
     * @brief Set the feedback amount.
     * @param newFeedback New feedback amount in [-1, 1].
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setFeedback(T newFeedback, bool skipSmoothing = false) {
        modulatedDelayStage.setFeedback(newFeedback, skipSmoothing);
    }

    /**
     * @brief Set the damping amount.
     * @param newDamping New damping amount in [0, 1].
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setDamping(T newDamping, bool skipSmoothing = false) {
        T newDampingHz = DAMPING_MIN_HZ * std::pow(T(15000) / DAMPING_MIN_HZ, T(1) - newDamping);
        modulatedDelayStage.setDampingCutoff(Frequency<T>::Hertz(newDampingHz));
    }

    /**
     * @brief Set the ping-pong amount.
     * @param newPingPong New ping-pong amount in [0, 1].
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setPingPong(T newPingPong, bool skipSmoothing = false) {
        modulatedDelayStage.setCrossFeedback(newPingPong, skipSmoothing);
    }

    /**
     * @brief Set the modulation depth.
     * @param newModDepth New modulation depth in [0, 1].
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setModDepth(T newModDepth, bool skipSmoothing = false) {
        modulatedDelayStage.setModulationDepth(Time<T>::Milliseconds(MAX_MODULATION_MS * newModDepth), skipSmoothing);
    }

    /// Get number of channels
    size_t getNumChannels() const { return numChannels; }

    /// Get sample rate
    T getSampleRate() const { return sampleRate; }

  private:
    // Config variables
    size_t numChannels = 0;
    T sampleRate = T(44100);

    // Processing components
    models::ModulatedDelayStage<T, LagrangeInterpolator<T>, false, true, true> modulatedDelayStage;
    Oscillator<T> wowLfo;
    Oscillator<T> flutterLfo;

    // Buffer for modulation
    AudioBuffer<T> modulationBuffer;
};

} // namespace jnsc::effects