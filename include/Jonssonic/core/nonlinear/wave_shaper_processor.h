// Jonssonic - A Modular Realtime C++ Audio DSP Library
// WaveShaperProcessor class header file
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/utils/detail/config_utils.h"

#include <algorithm>
#include <cstddef>
#include <jonssonic/core/common/dsp_param.h>
#include <jonssonic/core/nonlinear/wave_shaper.h>
#include <vector>

namespace jonssonic::core::nonlinear {

/**
 * @brief Parametric wave shaping stage with gain, bias, and asymmetry.
 *
 * @tparam SampleType   Sample data type (e.g., float, double)
 * @tparam ShaperType   Nonlinear shaping type (from WaveShaperType)
 */
template <typename T,
          WaveShaperType ShaperType,
          common::SmootherType SmootherType = common::SmootherType::OnePole,
          int SmootherOrder = 1>

class WaveShaperProcessor {
    /// Type aliases for convenience, readability and future-proofing
    using DspParamType = common::DspParam<T, SmootherType, SmootherOrder>;

  public:
    /// Default constructor
    WaveShaperProcessor() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels
     * @param sampleRate Sample rate in Hz
     */
    WaveShaperProcessor(size_t newNumChannels, T sampleRate) {
        prepare(newNumChannels, sampleRate);
    }

    /// Default destructor
    ~WaveShaperProcessor() = default;

    /// No copy semantics nor move semantics
    WaveShaperProcessor(const WaveShaperProcessor &) = delete;
    const WaveShaperProcessor &operator=(const WaveShaperProcessor &) = delete;
    WaveShaperProcessor(WaveShaperProcessor &&) = delete;
    const WaveShaperProcessor &operator=(WaveShaperProcessor &&) = delete;

    /**
     * @brief Prepare the wave shaper processor for processing.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     */
    void prepare(size_t newNumChannels, T newSampleRate) {
        // Clamp and set number of channels and sample rate
        numChannels = utils::detail::clampChannels(newNumChannels);
        sampleRate = utils::detail::clampSampleRate(newSampleRate);

        // Prepare parameters
        inputGain.prepare(numChannels, sampleRate);
        outputGain.prepare(numChannels, sampleRate);
        bias.prepare(numChannels, sampleRate);
        asymmetry.prepare(numChannels, sampleRate);
        shape.prepare(numChannels, sampleRate); // only functional with Dynamic shaper
        // Set bounds for parameters
        inputGain.setBounds(T(0.001), T(1000)); // -60 dB to +60 dB
        outputGain.setBounds(T(0.001), T(10));  // -60 dB to +20 dB
        bias.setBounds(T(-1), T(1));            // -1 to +1
        asymmetry.setBounds(T(-1), T(1));       // -1 to +1

        // Set default values
        inputGain.setTarget(T(1), true);
        outputGain.setTarget(T(1), true);
        bias.setTarget(T(0), true);
        asymmetry.setTarget(T(0), true);
        shape.setTarget(T(1), true); // Only functional with Dynamic shaper
    }

    /// Reset the wave shaper processor state
    void reset() { /* Nothing to reset internally for now.*/ }

    /**
     * @brief Process a single sample of specified channel.
     * @param ch Channel index
     * @param input Input sample
     * @return Output sample
     */
    T processSample(size_t ch, T input) {
        // Apply input gain
        input *= inputGain.getNextValue(ch);
        // Apply bias
        input += bias.getNextValue(ch);
        // Apply asymmetry (branchless)
        T asym = asymmetry.getNextValue(ch);
        T sign = (T(0) < input) - (input < T(0)); // 1 if input>0, -1 if input<0, 0 if input==0
        input *= (T(1) + asym * sign);
        // Apply waveshaping
        input = shaper.processSample(input);
        // Apply output gain
        input *= outputGain.getNextValue(ch);
        return input;
    }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input sample pointers (one per channel)
     * @param output Output sample pointers (one per channel)
     * @param numSamples Number of samples to process
     */
    void processBlock(const T *const *input, T *const *output, size_t numSamples) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t n = 0; n < numSamples; ++n) {
                // Get input sample
                T sample = input[ch][n];
                // Apply input gain
                sample *= inputGain.getNextValue(ch);
                // Apply bias
                sample += bias.getNextValue(ch);
                // Apply asymmetry (branchless)
                T asym = asymmetry.getNextValue(ch);
                T sign =
                    (T(0) < sample) - (sample < T(0)); // 1 if input>0, -1 if input<0, 0 if input==0
                sample *= (T(1) + asym * sign);
                // Apply waveshaping
                output[ch][n] = shaper.processSample(sample, shape.getNextValue(ch));
                // Apply output gain
                output[ch][n] *= outputGain.getNextValue(ch);
            }
        }
    }

    /**
     * @brief Set parameter smoothing time for all controls.
     * @param time Time struct.
     */
    void setControlSmoothingTime(Time<T> time) {
        inputGain.setSmoothingTime(time);
        outputGain.setSmoothingTime(time);
        bias.setSmoothingTime(time);
        asymmetry.setSmoothingTime(time);
        shape.setSmoothingTime(time);
    }

    /**
     * @brief Set input gain for all channels.
     * @param gain Input gain struct.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setInputGain(Gain<T> gain, bool skipSmoothing = false) {
        inputGain.setTarget(gain.toLinear(), skipSmoothing);
    }

    /**
     * @brief Set input gain for a specific channel.
     * @param ch Channel index.
     * @param gain Input gain struct.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setInputGain(size_t ch, Gain<T> gain, bool skipSmoothing = false) {
        inputGain.setTarget(ch, gain.toLinear(), skipSmoothing);
    }

    /**
     * @brief Set output gain for all channels.
     * @param gain Output gain struct.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setOutputGain(Gain<T> gain, bool skipSmoothing = false) {
        outputGain.setTarget(gain.toLinear(), skipSmoothing);
    }

    /**
     * @brief Set output gain for a specific channel.
     * @param ch Channel index.
     * @param gain Output gain struct.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setOutputGain(size_t ch, Gain<T> gain, bool skipSmoothing = false) {
        outputGain.setTarget(ch, gain.toLinear(), skipSmoothing);
    }

    /**
     * @brief Set bias for all channels.
     * @param biasValue Bias value (clamped to -1 to +1)
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setBias(T biasValue, bool skipSmoothing = false) {
        bias.setTarget(biasValue, skipSmoothing);
    }

    /**
     * @brief Set bias for a specific channel.
     * @param ch Channel index.
     * @param biasValue Bias value (clamped to -1 to +1)
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setBias(size_t ch, T biasValue, bool skipSmoothing = false) {
        bias.setTarget(ch, biasValue, skipSmoothing);
    }
    /**
     * @brief Set asymmetry for all channels.
     * @param asymValue Asymmetry value (clamped to -1 to +1)
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setAsymmetry(T asymValue, bool skipSmoothing = false) {
        asymmetry.setTarget(asymValue, skipSmoothing);
    }

    /**
     * @brief Set asymmetry for a specific channel.
     * @param ch Channel index.
     * @param asymValue Asymmetry value (clamped to -1 to +1)
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setAsymmetry(size_t ch, T asymValue, bool skipSmoothing = false) {
        asymmetry.setTarget(ch, asymValue, skipSmoothing);
    }

    /**
     * @brief Set shape for a specific channel.
     * @param ch Channel index.
     * @param shapeValue Shape value (not clamped but usable range ~ [2, 20])
     * @param skipSmoothing If true, skip smoothing and set immediately.
     * @note Only applicable with Dynamic shaper type.
     */
    void setShape(size_t ch, T shapeValue, bool skipSmoothing = false) {
        shape.setTarget(ch, shapeValue, skipSmoothing);
    }
    /**
     * @brief Set shape for all channels.
     * @param shapeValue Shape value (not clamped but usable range ~ [2, 20])
     * @param skipSmoothing If true, skip smoothing and set immediately.
     * @note Only applicable with Dynamic shaper type.
     */
    void setShape(T shapeValue, bool skipSmoothing = false) {
        shape.setTarget(shapeValue, skipSmoothing);
    }

  private:
    size_t numChannels = 0;
    T sampleRate = T(44100);
    DspParamType inputGain;
    DspParamType outputGain;
    DspParamType bias;
    DspParamType asymmetry;
    DspParamType shape;
    WaveShaper<T, ShaperType> shaper;
};

} // namespace jonssonic::core::nonlinear
