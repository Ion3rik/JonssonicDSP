// Jonssonic - A C++ audio DSP library
// WaveShaperProcessor class header file
// SPDX-License-Identifier: MIT

#pragma once

#include "WaveShaper.h"
#include "../common/DspParam.h"
#include <cstddef>
#include <algorithm>
#include <vector>

namespace Jonssonic {

/**
 * @brief Parametric wave shaping stage with gain, bias, and asymmetry.
 *
 * @tparam SampleType   Sample data type (e.g., float, double)
 * @tparam ShaperType   Nonlinear shaping type (from WaveShaperType)
 */
template 
    <typename T, 
    WaveShaperType ShaperType,
    SmootherType SmootherType = SmootherType::OnePole,
    int SmootherOrder = 1>

class WaveShaperProcessor {
public:
   WaveShaperProcessor() = default;
   ~WaveShaperProcessor() = default;

    // no copy semantics nor move semantics
    WaveShaperProcessor(const WaveShaperProcessor&) = delete;
    const WaveShaperProcessor& operator=(const WaveShaperProcessor&) = delete;
    WaveShaperProcessor(WaveShaperProcessor&&) = delete;
    const WaveShaperProcessor& operator=(WaveShaperProcessor&&) = delete;

    void prepare(size_t newNumChannels, T sampleRate, T smoothTimeMs = T(10)) {
        numChannels = newNumChannels;
        inputGain.prepare(newNumChannels, sampleRate, smoothTimeMs);
        outputGain.prepare(newNumChannels, sampleRate, smoothTimeMs);
        bias.prepare(newNumChannels, sampleRate, smoothTimeMs);
        asymmetry.prepare(newNumChannels, sampleRate, smoothTimeMs);
        shape.prepare(newNumChannels, sampleRate, smoothTimeMs); // only functional with Dynamic shaper

        // Set bounds for parameters
        inputGain.setBounds(T(0.001), T(100)); // -60 dB to +40 dB
        outputGain.setBounds(T(0.001), T(10)); // -60 dB to +20 dB
        bias.setBounds(T(-1), T(1)); // -1 to +1
        asymmetry.setBounds(T(-1), T(1)); // -1 to +1

        // Set default values
        inputGain.setTarget(T(1), true);
        outputGain.setTarget(T(1), true);
        bias.setTarget(T(0), true);
        asymmetry.setTarget(T(0), true);
        shape.setTarget(T(1), true); // Only functional with Dynamic shaper
    }

    void reset() {
        inputGain.reset();
        outputGain.reset();
        bias.reset();
        asymmetry.reset();
    }

    /** 
     * @brief Process a single sample of specified channel.
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
     */ 
    void processBlock(const T* const* input, T* const* output, size_t numSamples) {
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
                T sign = (T(0) < sample) - (sample < T(0)); // 1 if input>0, -1 if input<0, 0 if input==0
                sample *= (T(1) + asym * sign);
                // Apply waveshaping
                output[ch][n] = shaper.processSample(sample, shape.getNextValue(ch));
                // Apply output gain
                output[ch][n] *= outputGain.getNextValue(ch);
            }
        }
    }

    /**
     * @brief Set input gain for all channels.
     */
    void setInputGain(T gain, bool skipSmoothing = false) {
        inputGain.setTarget(gain, skipSmoothing);
    }

    /**
     * @brief Set input gain for a specific channel.
     */
    void setInputGain(size_t ch, T gain, bool skipSmoothing = false) {
        inputGain.setTarget(ch, gain, skipSmoothing);
    }

    /**
     * @brief Set output gain for all channels.
     */
    void setOutputGain(T gain, bool skipSmoothing = false) {
        outputGain.setTarget(gain, skipSmoothing);
    }

    /**
     * @brief Set output gain for a specific channel.
     */
    void setOutputGain(size_t ch, T gain, bool skipSmoothing = false) {
        outputGain.setTarget(ch, gain, skipSmoothing);
    }

    /**
     * @brief Set bias for all channels.
     */
    void setBias(T biasValue, bool skipSmoothing = false) {
        bias.setTarget(biasValue, skipSmoothing);
    }

    /**
     * @brief Set bias for a specific channel.
     */
    void setBias(size_t ch, T biasValue, bool skipSmoothing = false) {
        bias.setTarget(ch, biasValue, skipSmoothing);
    }
    /**
     * @brief Set asymmetry for all channels.
     */
    void setAsymmetry(T asymValue, bool skipSmoothing = false) {
        asymmetry.setTarget(asymValue, skipSmoothing);
    }

    /**
     * @brief Set asymmetry for a specific channel.
     */
    void setAsymmetry(size_t ch, T asymValue, bool skipSmoothing = false) {
        asymmetry.setTarget(ch, asymValue, skipSmoothing);
    }

    /**
     * @brief Set shape for a specific channel (only functional with Dynamic shaper).
     */
    void setShape(size_t ch, T shapeValue, bool skipSmoothing = false) {
        shape.setTarget(ch, shapeValue, skipSmoothing);
    }
    /**
     * @brief Set shape for all channels (only functional with Dynamic shaper).
     */
    void setShape(T shapeValue, bool skipSmoothing = false) {
        shape.setTarget(shapeValue, skipSmoothing);
    }

private:
    size_t numChannels = 0;
    DspParam<T, SmootherType, SmootherOrder> inputGain;
    DspParam<T, SmootherType, SmootherOrder> outputGain;
    DspParam<T, SmootherType, SmootherOrder> bias;
    DspParam<T, SmootherType, SmootherOrder> asymmetry;
    DspParam<T, SmootherType, SmootherOrder> shape; // Only functional with Dynamic shaper
    WaveShaper<T, ShaperType> shaper;
};

// Convenient type aliases for common distortion stages
template<typename T>
using HardDistortion = WaveShaperProcessor<T, WaveShaperType::HardClip>;

template<typename T>
using AtanDistortion = WaveShaperProcessor<T, WaveShaperType::Atan>;

template<typename T>
using TanhDistortion = WaveShaperProcessor<T, WaveShaperType::Tanh>;

template<typename T>
using FullWaveRectifierDistortion = WaveShaperProcessor<T, WaveShaperType::FullWaveRectifier>;

template<typename T>
using HalfWaveRectifierDistortion = WaveShaperProcessor<T, WaveShaperType::HalfWaveRectifier>;

} // namespace Jonssonic
