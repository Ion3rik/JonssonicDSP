// Jonssonic - A C++ audio DSP library
// Oscillator class header file
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/utils/detail/config_utils.h"

#include <cmath>
#include <jonssonic/core/common/dsp_param.h>
#include <jonssonic/core/common/quantities.h>
#include <vector>

namespace jonssonic::core::generators {
/// Waveform types
enum class Waveform { Sine, Saw, Square, Triangle };
/**
 * @brief Waveform generator class.
 * This class generates basic waveforms (sine, square, sawtooth, triangle) at a specified frequency.
 */
template <typename T,
          common::SmootherType SmootherType = common::SmootherType::OnePole,
          int SmootherOrder = 1>
class Oscillator {
    /// Type aliases for convenience, readability and future-proofing
    using DspParam = common::DspParam<T, SmootherType, SmootherOrder>;

  public:
    /// Default constructor
    Oscillator() : numChannels(0), sampleRate(T(0)) {}
    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     */
    Oscillator(size_t newNumChannels, T newSampleRate, T newSmoothingTimeMs = T(10)) {
        prepare(newNumChannels, newSampleRate, newSmoothingTimeMs);
    }

    /// Default destructor
    ~Oscillator() = default;

    /// No copy semantics nor move semantics
    Oscillator(const Oscillator &) = delete;
    const Oscillator &operator=(const Oscillator &) = delete;
    Oscillator(Oscillator &&) = delete;
    const Oscillator &operator=(Oscillator &&) = delete;

    /**
     * @brief Prepare the oscillator for processing.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     */
    void prepare(size_t newNumChannels, T newSampleRate) {
        // Update number of channels and sample rate
        numChannels = utils::detail::clampChannels(newNumChannels);
        sampleRate = utils::detail::clampSampleRate(newSampleRate);

        // Resize and initialize to zero
        phase.assign(numChannels, T(0));
        phaseIncrement.prepare(numChannels, sampleRate);
    }

    /// Reset phase for all channels
    void reset() { std::fill(phase.begin(), phase.end(), T(0)); }

    /// Reset phase for specific channel
    void reset(size_t channel) { phase[channel] = T(0); }

    /**
     * @brief Process single sample for a specific channel (no phase modulation)
     * @param ch Channel index
     */
    T processSample(size_t ch) {
        // Generate waveform at current phase
        T output = generateWaveform(phase[ch]);

        // Advance and wrap phase using floor
        phase[ch] += phaseIncrement.getNextValue(ch);
        phase[ch] = phase[ch] - std::floor(phase[ch]);

        return output;
    }

    /**
     * @brief Process single sample for a specific channel (with phase modulation)
     * @param ch Channel index
     * @param phaseMod Phase modulation value (0.0 to 1.0)
     */
    T processSample(size_t ch, T phaseMod) {
        // Calculate modulated phase and wrap using floor
        T modulatedPhase = phase[ch] + phaseMod;
        modulatedPhase = modulatedPhase - std::floor(modulatedPhase);

        // Generate waveform at modulated phase
        T output = generateWaveform(modulatedPhase);

        // Advance and wrap current phase using floor
        phase[ch] += phaseIncrement.getNextValue(ch);
        phase[ch] = phase[ch] - std::floor(phase[ch]);

        return output;
    }

    /**
     * @brief Process block for all channels (no phase modulation)
     * @param output Output sample pointers (one per channel)
     * @param numSamples Number of samples to process
     */
    void processBlock(T *const *output, size_t numSamples) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t i = 0; i < numSamples; ++i) {
                // Generate waveform at current phase
                output[ch][i] = generateWaveform(phase[ch]);

                // Advance and wrap current phase using floor
                phase[ch] += phaseIncrement.getNextValue(ch);
                phase[ch] = phase[ch] - std::floor(phase[ch]);
            }
        }
    }

    /**
     * @brief Process block for all channels (with phase modulation)
     * @param output Output sample pointers (one per channel)
     * @param phaseMod Phase modulation sample pointers (one per channel)
     * @param numSamples Number of samples to process
     */
    void processBlock(T *const *output, const T *const *phaseMod, size_t numSamples) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t i = 0; i < numSamples; ++i) {

                // Calculate and wrap modulated phase using floor
                T modulatedPhase = phase[ch] + phaseMod[ch][i];
                modulatedPhase = modulatedPhase - std::floor(modulatedPhase);

                // Generate waveform at modulated phase
                output[ch][i] = generateWaveform(modulatedPhase);

                // Advance and wrap current phase using floor
                phase[ch] += phaseIncrement.getNextValue(ch);
                phase[ch] = phase[ch] - std::floor(phase[ch]);
            }
        }
    }

    /**
     * @brief Set control smoothing time for frequency changes.
     * @param time Smoothing time struct.
     */
    void setControlSmoothingTime(Time<T> time) { phaseIncrement.setSmoothingTime(time); }

    /**
     * @brief Set frequency for all channels.
     * @param freq Frequency struct.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setFrequency(Frequency<T> freq, bool skipSmoothing = false) {
        phaseIncrement.setTarget(freq.toNormalized(sampleRate), skipSmoothing);
    }

    /**
     * @brief Set frequency for a specific channel.
     * @param channel Channel index.
     * @param freq Frequency struct.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setFrequency(size_t channel, Frequency<T> freq, bool skipSmoothing = false) {
        phaseIncrement.setTarget(channel, freq.toNormalized(sampleRate), skipSmoothing);
    }

    /**
     * @brief Set the waveform type.
     * @param newWaveform Waveform type enum.
     */
    void setWaveform(Waveform newWaveform) { waveform = newWaveform; }

    /**
     * @brief Enable or disable bandlimited waveform generation (if applicable).
     * @param enable True to enable anti-aliasing, false to disable.
     * @note Currently not implemented; placeholder for future development.
     */
    void setAntiAliasing(bool enable) { useAntiAliasing = enable; }

  private:
    // Generate waveform sample at given phase (0.0 to 1.0)
    inline T generateWaveform(T phase) const {
        switch (waveform) {
        case Waveform::Sine:
            // Sine wave
            return std::sin(T(2.0 * M_PI) * phase);

        case Waveform::Saw:
            // Sawtooth wave
            return T(2.0) * phase - T(1.0);

        case Waveform::Square:
            // Square wave
            return (phase < T(0.5)) ? T(-1.0) : T(1.0);

        case Waveform::Triangle:
            // Triangle wave
            return T(1.0) - std::abs(T(4.0) * phase - T(2.0));

        default:
            return T(0);
        }
    }

    T sampleRate = 44100.0;
    size_t numChannels;
    Waveform waveform = Waveform::Sine;
    bool useAntiAliasing = false;

    std::vector<T> phase;    // Phase per channel
    DspParam phaseIncrement; // Phase increment per channel
};
} // namespace jonssonic::core::generators