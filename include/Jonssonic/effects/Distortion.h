// Jonssonic - A C++ audio DSP library
// Distortion effect header file
// SPDX-License-Identifier: MIT

#pragma once
#include "../core/nonlinear/WaveShaperProcessor.h"
#include "../core/oversampling/Oversampler.h"
#include "../core//common/AudioBuffer.h"
#include "../core/filters/FirstOrderFilter.h"
#include "../core/filters/UtilityFilters.h"
#include "../utils/MathUtils.h"

namespace Jonssonic::effects {
/**
 * @brief Distortion effect with continuous curve shaping and tone control.
 * @param T Sample data type (e.g., float, double)
 */
template<typename T>
class Distortion {
public:
    // TUNABLE CONSTANTS
    /**
     * @brief Oversampling factor
     */
    static constexpr size_t OVERSAMPLING_FACTOR = 4;

    /**
     * @brief Smoothing time in milliseconds for parameter changes.
     */
    static constexpr T PARAM_SMOOTH_TIME_MS = T(50);

    /**
     * @brief Asymmetry scale factor to map normalized [-1,1] to actual asymmetry amount.
     */
    static constexpr T ASYMMETRY_SCALE = T(0.5); 

    /**
     * @brief Shape minimum value (used for mapping normalized shape parameter).
     */
    static constexpr T SHAPE_MIN = T(2);

    /**
     * @brief Shape maximum value (used for mapping normalized shape parameter).
     */
    static constexpr T SHAPE_MAX = T(20);



    // Constructor and Destructor
    Distortion() = default;
    Distortion(size_t newNumChannels, size_t newMaxBlockSize, T newSampleRate) {
        prepare(newNumChannels, newMaxBlockSize, newSampleRate);
    }
    ~Distortion() = default;

    // No copy or move semantics
    Distortion(const Distortion&) = delete;
    Distortion& operator=(const Distortion&) = delete;
    Distortion(Distortion&&) = delete;
    Distortion& operator=(Distortion&&) = delete;

    /**
     * @brief Prepare the Distortion effect for processing.
     * @param newNumChannels Number of channels
     * @param newMaxBlockSize Maximum block size for processing (at base sample rate)
     * @param newSampleRate Sample rate in Hz
     */
    void prepare(size_t newNumChannels, size_t newMaxBlockSize, T newSampleRate) {

        // Store global parameters
        numChannels = newNumChannels;
        sampleRate = newSampleRate;

        // Prepare waveshaper processor
        shaper.prepare(newNumChannels, newSampleRate, PARAM_SMOOTH_TIME_MS); 

        // Prepare tone filter and dc blocker
        toneFilter.prepare(newNumChannels, newSampleRate);
        toneFilter.setType(FirstOrderType::Lowpass);
        dcBlocker.prepare(newNumChannels, newSampleRate);


        // Prepare oversampler and oversampled buffer
        oversampler.prepare(newNumChannels, newMaxBlockSize);
        oversampledBuffer.resize(newNumChannels, newMaxBlockSize * OVERSAMPLING_FACTOR);

        // Set Default Parameters
        setDriveDb(T(0), true); // 0 dB drive
        setAsymmetry(T(0), true);    // 0 asymmetry
        setShape(T(0.5), true); // Mid shape
        setToneFrequency(T(12000), true); // 12 kHz tone cutoff
        setOversamplingEnabled(false);

        // Mark as prepared
        togglePrepared = true;
    }

    void reset() {
        shaper.reset();
        toneFilter.reset();
        oversampler.reset();
    }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input buffer (numChannels x numSamples)
     * @param output Output buffer (numChannels x numSamples)
     * @param numSamples Number of samples to process
     */
    void processBlock(const T* const* input, T* const* output, size_t numSamples)
    {   
        // Process waveshaper with oversampling
        if (toggleOversampling) {
            // Upsample
            size_t numOversampledSamples = oversampler.upsample(input, oversampledBuffer.writePtrs(), numSamples);

            // Process waveshaper
            shaper.processBlock(oversampledBuffer.readPtrs(), oversampledBuffer.writePtrs(), numOversampledSamples);

            // Downsample
            oversampler.downsample(oversampledBuffer.readPtrs(), output, numSamples);
        } 
        // Process waveshaper without oversampling
        else{
            shaper.processBlock(input, output, numSamples);
        }

        // Apply tone control always at base sample rate
        toneFilter.processBlock(output, output, numSamples);

        // Apply DC blocker to counter act possible asymmetry induced DC offset
        dcBlocker.processBlock(output, output, numSamples);
    }

    // SETTERS FOR PARAMETERS

    /**
     * @brief Set drive in dB.
     * @param driveDb Drive amount in decibels.
     */
    void setDriveDb(T driveDb, bool skipSmoothing = false) {
        T driveLinear = dB2Mag(driveDb); // Convert dB to linear
        shaper.setInputGain(driveLinear, skipSmoothing);
    }

    /**
     * @brief Set asymmetry in normalized range [-1, 1].
     */
    void setAsymmetry(T asymmetryNormalized, bool skipSmoothing = false) {
        shaper.setAsymmetry(asymmetryNormalized * ASYMMETRY_SCALE, skipSmoothing);
    }

    /**
     * @brief Set shape parameter in normalized range [0, 1].
     */
    void setShape(T shapeNormalized, bool skipSmoothing = false) {
        T shape = SHAPE_MIN + shapeNormalized * (SHAPE_MAX - SHAPE_MIN); // Map to [SHAPE_MIN, SHAPE_MAX]
        shaper.setShape(shape, skipSmoothing);
    }

    /**
     * @brief Set tone control frequency in Hz.
     */
    void setToneFrequency(T frequencyHz, bool /*skipSmoothing*/) {
        toneFilter.setFreq(frequencyHz);
    }

    /**
     * @brief Enable or disable oversampling.
     */
    void setOversamplingEnabled(bool enabled) {
        toggleOversampling = enabled;
    }


    // GETTERS FOR GLOBAL PARAMETERS
    size_t getNumChannels() const {
        return numChannels;
    }
    T getSampleRate() const {
        return sampleRate;
    }
    bool isPrepared() const {
        return togglePrepared;
    }
    size_t getLatencySamples() const {
        if (toggleOversampling) {
            return oversampler.getLatencySamples(); // latency due to oversampling
        } else {
            return 0; // otherwise no latency
        }
    }

private:
    
    // GLOBAL PARAMETERS
    size_t numChannels = 0;
    T sampleRate = T(44100);
    bool togglePrepared = false;
    bool toggleOversampling = false;

    // PROCESSORS
    WaveShaperProcessor<T, WaveShaperType::Dynamic> shaper; // waveshaper processor with dynamic type
    FirstOrderFilter<T> toneFilter; // tone control filter
    DCBlocker<T> dcBlocker; // DC blocker after distortion

    Oversampler<T, OVERSAMPLING_FACTOR> oversampler; // oversampler for nonlinear EQ
    AudioBuffer<T> oversampledBuffer; // buffer for oversampling
    

};

} // namespace Jonssonic::effects