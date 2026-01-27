// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Modulated Delay Stage header file
// SPDX-License-Identifier: MIT

#pragma once
#include <jonssonic/core/common/dsp_param.h>
#include <jonssonic/core/common/quantities.h>
#include <jonssonic/core/delays/delay_line.h>
#include <jonssonic/core/filters/first_order_filter.h>
#include <jonssonic/core/generators/oscillator.h>
#include <jonssonic/utils/math_utils.h>

namespace jnsc::models {
/**
 * @brief Modulated delay stage with LFO modulation.
 * @tparam T Sample data type (e.g., float, double)
 * @tparam Interpolator Interpolator type for delay line (default: LinearInterpolator)
 * @tparam UseInternalLFO If true, includes an internal LFO for modulation.
 * @tparam UseDamping If true, includes a one-pole lowpass damping filter in the feedback path.
 * @tparam UseCrossFeedback If true, enables cross-feedback between channels for ping-pong effects.
 */
template <typename T,
          typename Interpolator = LinearInterpolator<T>,
          bool UseInternalLFO = true,
          bool UseDamping = false,
          bool UseCrossFeedback = false>
class ModulatedDelayStage {
  public:
    /// Default constructor.
    ModulatedDelayStage() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels
     * @param newMaxDelayMs Maximum delay time in milliseconds
     * @param newSampleRate Sample rate in Hz
     */
    ModulatedDelayStage(size_t newNumChannels, T newMaxDelayMs, T newSampleRate) {
        prepare(newNumChannels, newMaxDelayMs, newSampleRate);
    }

    /// Default destructor.
    ~ModulatedDelayStage() = default;

    /// No copy nor move semantics
    ModulatedDelayStage(const ModulatedDelayStage&) = delete;
    ModulatedDelayStage& operator=(const ModulatedDelayStage&) = delete;
    ModulatedDelayStage(ModulatedDelayStage&&) = delete;
    ModulatedDelayStage& operator=(ModulatedDelayStage&&) = delete;

    /**
     * @brief Prepare the modulated delay stage for processing.
     * @param newNumChannels Number of channels
     * @param newMaxDelay Maximum delay time struct.
     * @param newSampleRate Sample rate in Hz
     */
    void prepare(size_t newNumChannels, Time<T> newMaxDelay, T newSampleRate) {
        // Store config variables
        numChannels = utils::detail::clampChannels(newNumChannels);
        sampleRate = utils::detail::clampSampleRate(newSampleRate);

        // Prepare delay line
        delayLine.prepare(numChannels, sampleRate, newMaxDelay);

        // Prepare damping filter if enabled
        if constexpr (UseDamping)
            dampingFilter.prepare(numChannels, sampleRate, FirstOrderType::Lowpass);

        // Prepare internal LFO if enabled
        if constexpr (UseInternalLFO)
            lfo.prepare(numChannels, sampleRate);

        // Prepare parameters if enabled
        feedforward.prepare(numChannels, sampleRate);
        feedback.prepare(numChannels, sampleRate);
        modDepthSamples.prepare(numChannels, sampleRate);

        if constexpr (UseCrossFeedback)
            crossFeedback.prepare(numChannels, sampleRate);
        if constexpr (UseInternalLFO)
            lfoPhaseOffset.prepare(numChannels, sampleRate);

        // Preapre state buffer if cross-feedback is enabled
        if constexpr (UseCrossFeedback)
            delayedSamples.resize(numChannels, T(0));

        // Set bounds for parameters that are enabled
        T maxModSamples = newMaxDelay.toSamples(sampleRate);
        modDepthSamples.setBounds(T(0), maxModSamples);
        feedforward.setBounds(T(0), T(1));
        feedback.setBounds(T(-0.99), T(0.99));
        if constexpr (UseCrossFeedback)
            crossFeedback.setBounds(T(-0.99), T(0.99));
        if constexpr (UseInternalLFO)
            lfoPhaseOffset.setBounds(T(0), T(1));
    }

    /// Reset the modulated delay stage state.
    void reset() {
        delayLine.reset();
        feedforward.reset();
        feedback.reset();
        if constexpr (UseDamping)
            dampingFilter.reset();
        if constexpr (UseCrossFeedback) {
            crossFeedback.reset();
            delayedSamples.clear();
        }
        modDepthSamples.reset();
        if constexpr (UseInternalLFO)
            lfoPhaseOffset.reset();
    }

    /**
     * @brief Process a block of samples with internal LFO.
     * @param input Input sample pointers (one per channel)
     * @param output Output sample pointers (one per channel)
     * @param numSamples Number of samples to process
     */

    void processBlock(const T* const* input, T* const* output, size_t numSamples) {
        if constexpr (UseCrossFeedback) {
            processWithCrossFeedback(input, output, numSamples);
        } else {
            processWithoutCrossFeedback(input, output, numSamples);
        }
    }

    /**
     * @brief Process a block of samples with external modulation.
     * @param input Input sample pointers (one per channel)
     * @param output Output sample pointers (one per channel)
     * @param mod Delay time modulation signal pointers (one per channel)
     * @param numSamples Number of samples to process
     */
    void processBlock(const T* const* input, T* const* output, const T* const* mod, size_t numSamples) {
        if constexpr (UseCrossFeedback) {
            processWithCrossFeedback(input, output, mod, numSamples);
        } else {
            processWithoutCrossFeedback(input, output, mod, numSamples);
        }
    }

    /**
     * @brief Set the control smoothing time in various units.
     * @param time Time value representing the smoothing time
     */
    void setControlSmoothingTime(Time<T> time) {
        delayLine.setControlSmoothingTime(time);
        feedforward.setSmoothingTime(time);
        feedback.setSmoothingTime(time);
        modDepthSamples.setSmoothingTime(time);

        if constexpr (UseCrossFeedback)
            crossFeedback.setSmoothingTime(time);

        if constexpr (UseInternalLFO) {
            lfo.setControlSmoothingTime(time);
            lfoPhaseOffset.setSmoothingTime(time);
        }
    }

    /**
     * @brief Set the delay time for all channels.
     * @param newDelay Delay time struct.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setDelay(Time<T> newDelay, bool skipSmoothing = false) { delayLine.setDelay(newDelay, skipSmoothing); }

    /**
     * @brief Set the delay time for a specific channel.
     * @param ch Channel index
     * @param newDelay Delay time struct.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setDelay(size_t ch, Time<T> newDelay, bool skipSmoothing = false) {
        delayLine.setDelay(ch, newDelay, skipSmoothing);
    }

    /**
     * @brief Set the damping filter cutoff frequency in Hz.
     * @param newCutoff Cutoff frequency.
     * @note Only applicable if UseDamping is true.
     */
    void setDampingCutoff(Frequency<T> newCutoff) {
        if constexpr (UseDamping) {
            dampingFilter.setFreq(newCutoff);
        }
    }

    /**
     * @brief Set feedforward amount.
     * @param newFeedforward Feedforward amount in [0, 1].
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setFeedforward(T newFeedforward, bool skipSmoothing = false) {
        feedforward.setTarget(newFeedforward, skipSmoothing);
    }

    /**
     * @brief Set feedback amount.
     * @param newFeedback Feedback amount in [-1, 1].
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setFeedback(T newFeedback, bool skipSmoothing = false) { feedback.setTarget(newFeedback, skipSmoothing); }

    /**
     * @brief Set cross-feedback amount.
     * @param newCrossFeedback Cross-feedback amount in [-1, 1].
     * @param skipSmoothing If true, skip smoothing and set immediately.
     * @note Only applicable if UseCrossFeedback is true.
     */
    void setCrossFeedback(T newCrossFeedback, bool skipSmoothing = false) {
        if constexpr (UseCrossFeedback) {
            crossFeedback.setTarget(newCrossFeedback, skipSmoothing);
        }
    }

    /**
     * @brief Set delay time modulation depth.
     * @param newDepth Modulation depth time struct.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setModulationDepth(Time<T> newDepth, bool skipSmoothing = false) {
        modDepthSamples.setTarget(newDepth.toSamples(sampleRate), skipSmoothing);
    }

    /**
     * @brief Set LFO type.
     * @param newType New LFO waveform type.
     * @note Only applicable if UseInternalLFO is true.
     */
    void setLfoType(Waveform newType) {
        if constexpr (UseInternalLFO) {
            lfo.setWaveformType(newType);
        }
    }

    /**
     * @brief Set constant LFO frequency for all channels in Hz.
     * @param newFreq Frequency in Hz.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     * @note Only applicable if UseInternalLFO is true.
     */
    void setLfoFrequency(Frequency<T> newFreq, bool skipSmoothing = false) {
        if constexpr (UseInternalLFO) {
            lfo.setFrequency(newFreq, skipSmoothing);
        }
    }

    /**
     * @brief Set LFO phase offset for a specific channel.
     * @param ch Channel index.
     * @param phaseOffset Phase offset in [0, 1].
     * @param skipSmoothing If true, skip smoothing and set immediately.
     * @note Only applicable if UseInternalLFO is true.
     */

    void setLfoPhaseOffset(size_t ch, T phaseOffset, bool skipSmoothing = false) {
        if constexpr (UseInternalLFO) {
            lfoPhaseOffset.setTarget(ch, phaseOffset, skipSmoothing);
        }
    }

  private:
    // Config variables
    size_t numChannels = 0;
    T sampleRate = T(44100);

    // Processing components
    DelayLine<T, Interpolator> delayLine;
    FirstOrderFilter<T> dampingFilter;
    Oscillator<T> lfo;

    // Parameters
    DspParam<T> feedforward;     // feedforward gain
    DspParam<T> feedback;        // intra channel feedback
    DspParam<T> crossFeedback;   // inter channel feedback
    DspParam<T> modDepthSamples; // modulation depth in samples
    DspParam<T> lfoPhaseOffset;  // lfo phase offset per channel

    // State variables
    std::vector<T> delayedSamples; // needed for cross-feedback processing

    // Process block with cross-feedback and internal LFO
    void processWithCrossFeedback(const T* const* input, T* const* output, size_t numSamples) {
        for (size_t n = 0; n < numSamples; ++n) {
            // First pass: read delayed samples from all channels
            for (size_t ch = 0; ch < numChannels; ++ch) {
                T modValue = lfo.processSample(ch, lfoPhaseOffset.getNextValue(ch)) * modDepthSamples.getNextValue(ch);
                T sample = delayLine.readSample(ch, modValue);

                if constexpr (UseDamping)
                    sample = dampingFilter.processSample(ch, sample);

                delayedSamples[ch] = sample;
            }

            // Second pass: compute feedback and write back to delay lines
            for (size_t ch = 0; ch < numChannels; ++ch) {
                // Retrieve delayed sample for this channel and next channel
                T sample = delayedSamples[ch];
                size_t nextCh = (ch + 1) % numChannels;
                T nextChSample = delayedSamples[nextCh];

                // Compute mixed sample with cross-feedback
                T crossFeedbackValue = crossFeedback.getNextValue(ch);
                T mixedSample = sample * (T(1) - crossFeedbackValue) + nextChSample * crossFeedbackValue;

                // Compute feedback sample
                T feedbackSample = mixedSample * feedback.getNextValue(ch);

                // Scale inputs to other than first channel based on cross-feedback amount.
                T isFirstChannel = static_cast<T>(ch == 0);
                T inputGain = (T(1) - crossFeedbackValue) * (T(1) - isFirstChannel) + isFirstChannel;

                // Write input + feedback back into delay line
                delayLine.writeSample(ch, input[ch][n] * inputGain + feedbackSample);

                // Output the delayed sample + feedforward
                output[ch][n] = sample + feedforward.getNextValue(ch) * input[ch][n];
            }
        }
    }

    // Process block with cross-feedback and external modulation
    void processWithCrossFeedback(const T* const* input, T* const* output, const T* const* mod, size_t numSamples) {
        for (size_t n = 0; n < numSamples; ++n) {
            // First pass: read delayed samples from all channels
            for (size_t ch = 0; ch < numChannels; ++ch) {
                T modValue = mod[ch][n] * modDepthSamples.getNextValue(ch);
                T sample = delayLine.readSample(ch, modValue);

                if constexpr (UseDamping)
                    sample = dampingFilter.processSample(ch, sample);

                delayedSamples[ch] = sample;
            }

            // Second pass: compute feedback and write back to delay lines
            for (size_t ch = 0; ch < numChannels; ++ch) {
                // Retrieve delayed sample for this channel and next channel
                T sample = delayedSamples[ch];
                size_t nextCh = (ch + 1) % numChannels;
                T nextChSample = delayedSamples[nextCh];

                // Compute mixed sample with cross-feedback
                T crossFeedbackValue = crossFeedback.getNextValue(ch);
                T mixedSample = sample * (T(1) - crossFeedbackValue) + nextChSample * crossFeedbackValue;

                // Compute feedback sample
                T feedbackSample = mixedSample * feedback.getNextValue(ch);

                // Scale inputs to other than first channel based on cross-feedback amount.
                T isFirstChannel = static_cast<T>(ch == 0);
                T inputGain = (T(1) - crossFeedbackValue) * (T(1) - isFirstChannel) + isFirstChannel;

                // Write input + feedback back into delay line
                delayLine.writeSample(ch, input[ch][n] * inputGain + feedbackSample);

                // Output the delayed sample + feedforward
                output[ch][n] = sample + feedforward.getNextValue(ch) * input[ch][n];
            }
        }
    }

    // Proces block without cross-feedback with internal LFO
    void processWithoutCrossFeedback(const T* const* input, T* const* output, size_t numSamples) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t n = 0; n < numSamples; ++n) {
                // Get LFO modulation value if internal LFO is used
                T modValue = T(0);
                if constexpr (UseInternalLFO) {
                    modValue =
                        lfo.processSample(ch, lfoPhaseOffset.getNextValue(ch)) * modDepthSamples.getNextValue(ch);
                }

                // Read delayed sample with modulation
                T sample = delayLine.readSample(ch, modValue);

                // Apply damping filter if enabled
                if constexpr (UseDamping)
                    sample = dampingFilter.processSample(ch, sample);

                // Apply feedback
                T feedbackSample = sample * feedback.getNextValue(ch);

                // Write input + feedback back into delay line
                delayLine.writeSample(ch, input[ch][n] + feedbackSample);

                // Output the delayed sample + feedforward
                output[ch][n] = sample + feedforward.getNextValue(ch) * input[ch][n];
            }
        }
    }

    // Process block without cross-feedback with external modulation
    void processWithoutCrossFeedback(const T* const* input, T* const* output, const T* const* mod, size_t numSamples) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t n = 0; n < numSamples; ++n) {
                // Scale modulation signal by modulation depth
                T modValue = mod[ch][n] * modDepthSamples.getNextValue(ch);

                // Read delayed sample with modulation
                T sample = delayLine.readSample(ch, modValue);

                // Apply damping filter if enabled
                if constexpr (UseDamping)
                    sample = dampingFilter.processSample(ch, sample);

                // Apply feedback
                T feedbackSample = sample * feedback.getNextValue(ch);

                // Write input + feedback back into delay line
                delayLine.writeSample(ch, input[ch][n] + feedbackSample);

                // Output the delayed sample + feedforward
                output[ch][n] = sample + feedforward.getNextValue(ch) * input[ch][n];
            }
        }
    }
};

} // namespace jnsc::models
