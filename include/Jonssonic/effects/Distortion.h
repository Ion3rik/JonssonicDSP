// Jonssonic - A C++ audio DSP library
// Distortion effect header file
// SPDX-License-Identifier: MIT

#pragma once
#include <jonssonic/core/common/audio_buffer.h>
#include <jonssonic/core/common/quantities.h>
#include <jonssonic/core/filters/utility_filters.h>
#include <jonssonic/core/mixing/dry_wet_mixer.h>
#include <jonssonic/models/saturation/saturation_stage.h>
#include <jonssonic/utils/buffer_utils.h>
#include <jonssonic/utils/math_utils.h>

namespace jnsc::effects {
/**
 * @brief Distortion effect with continuous curve shaping and tone control.
 * @param T Sample data type (e.g., float, double)
 */
template <typename T>
class Distortion {
    /**
     * @brief Tunable constants for the distortion effect
     * @param OVERSAMPLING_FACTOR Oversampling factor for waveshaping.
     * @param PARAM_SMOOTH_TIME_MS Smoothing time for parameter changes in milliseconds.
     * @param ASYMMETRY_SCALE Scale factor for asymmetry control.
     * @param SHAPE_MIN Minimum shape parameter value (soft clipping).
     * @param SHAPE_MAX Maximum shape parameter value (hard clipping).
     */
    static constexpr size_t OVERSAMPLING_FACTOR = 8;
    static constexpr T PARAM_SMOOTH_TIME_MS = T(50);
    static constexpr T ASYMMETRY_SCALE = T(0.5);
    static constexpr T SHAPE_MIN = T(2);
    static constexpr T SHAPE_MAX = T(20);

  public:
    /// Default constructor.
    Distortion() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels
     * @param newMaxBlockSize Maximum block size for processing (at base sample rate)
     * @param newSampleRate Sample rate in Hz
     */
    Distortion(size_t newNumChannels, size_t newMaxBlockSize, T newSampleRate) {
        prepare(newNumChannels, newMaxBlockSize, newSampleRate);
    }

    /// Default destructor.
    ~Distortion() = default;

    /// No copy nor move semantics
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
        numChannels = utils::detail::clampChannels(newNumChannels);
        sampleRate = utils::detail::clampSampleRate(newSampleRate);

        // Prepare processing stages
        distortion.prepare(newNumChannels, newMaxBlockSize, newSampleRate);
        distortionOS.prepare(newNumChannels, newMaxBlockSize, newSampleRate);

        // Prepare DC blocker
        dcBlocker.prepare(newNumChannels, newSampleRate);

        // Setup post-filters for tone control
        distortion.setPostFilterType(core::filters::BiquadType::Lowpass);
        distortionOS.setPostFilterType(core::filters::BiquadType::Lowpass);

        // Setup pre-filters to remove low-frequency content
        distortion.setPreFilterType(core::filters::BiquadType::Highpass);
        distortionOS.setPreFilterType(core::filters::BiquadType::Highpass);
        distortion.setPreFilterFrequency(Frequency<T>::Hertz(T(100)));
        distortionOS.setPreFilterFrequency(Frequency<T>::Hertz(T(100)));

        // Prepare dry/wet mixer with latency compensation and output gain
        size_t oversamplerLatencySamples = distortionOS.getLatencySamples();
        dryWetMixer.prepare(newNumChannels, newSampleRate, oversamplerLatencySamples);

        // Prepare internal buffers and oversampler
        fxBuffer.resize(newNumChannels, newMaxBlockSize);

        // Set parameter smoothing times
        distortion.setControlSmoothingTime(Time<T>::Milliseconds(PARAM_SMOOTH_TIME_MS));
        distortionOS.setControlSmoothingTime(Time<T>::Milliseconds(PARAM_SMOOTH_TIME_MS));
        dryWetMixer.setControlSmoothingTime(Time<T>::Milliseconds(PARAM_SMOOTH_TIME_MS));

        // Set Default Parameters
        setDriveDb(T(0), true);           // 0 dB drive
        setAsymmetry(T(0), true);         // 0 asymmetry
        setShape(T(0.5), true);           // Mid shape
        setToneFrequency(T(12000), true); // 12 kHz tone cutoff
        setOutputGainDb(T(0), true);      // 0 dB output gain
        setOversamplingEnabled(false);
    }

    void reset() {
        distortion.reset();
        distortionOS.reset();
        dcBlocker.reset();
        outputGain.reset();
        dryWetMixer.reset();
        fxBuffer.clear();
    }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input buffer (numChannels x numSamples)
     * @param output Output buffer (numChannels x numSamples)
     * @param numSamples Number of samples to process
     */
    void processBlock(const T* const* input, T* const* output, size_t numSamples) {
        // Copy input to fxBuffer for processing
        utils::copyToBuffer<T>(input, fxBuffer.writePtrs(), numChannels, numSamples);

        // Process oversampled distortion if enabled
        if (toggleOversampling) {
            distortionOS.processBlock(fxBuffer.readPtrs(), fxBuffer.writePtrs(), numSamples);
        }
        // Process non-oversampled distortion otherwise
        else {
            distortion.processBlock(fxBuffer.readPtrs(), fxBuffer.writePtrs(), numSamples);
        }

        // Apply DC blocker to counter act possible asymmetry induced DC offset
        dcBlocker.processBlock(fxBuffer.readPtrs(), fxBuffer.writePtrs(), numSamples);

        // Apply dry/wet mixing (with delay compensation if oversampling)
        size_t dryDelaySamples = getLatencySamples();
        dryWetMixer.processBlock(input, fxBuffer.readPtrs(), output, numSamples, dryDelaySamples);

        // Apply output gain
        outputGain.applyToBuffer(output, numSamples);
    }

    // SETTERS FOR PARAMETERS

    /**
     * @brief Set drive in dB.
     * @param driveDb Drive amount in decibels.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setDriveDb(T driveDb, bool skipSmoothing = false) {
        distortion.setDrive(Gain<T>::Decibels(driveDb), skipSmoothing);
        distortionOS.setDrive(Gain<T>::Decibels(driveDb), skipSmoothing);
    }

    /**
     * @brief Set asymmetry in normalized range [-1, 1].
     * @param asymmetryNormalized Asymmetry amount in normalized range [-1, 1].
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setAsymmetry(T asymmetryNormalized, bool skipSmoothing = false) {
        distortion.setAsymmetry(asymmetryNormalized * ASYMMETRY_SCALE, skipSmoothing);
        distortionOS.setAsymmetry(asymmetryNormalized * ASYMMETRY_SCALE, skipSmoothing);
    }

    /**
     * @brief Set shape parameter in normalized range [0, 1].
     * @param shapeNormalized Shape parameter in normalized range [0, 1].
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setShape(T shapeNormalized, bool skipSmoothing = false) {
        T shape =
            SHAPE_MIN + shapeNormalized * (SHAPE_MAX - SHAPE_MIN); // Map to [SHAPE_MIN, SHAPE_MAX]
        distortion.setShape(shape, skipSmoothing);
        distortionOS.setShape(shape, skipSmoothing);
    }

    /**
     * @brief Set tone control frequency in Hz.
     * @param frequencyHz Tone cutoff frequency in Hertz.
     */
    void setToneFrequency(T frequencyHz) {
        distortion.setPostFilterFrequency(Frequency<T>::Hertz(frequencyHz));
        distortionOS.setPostFilterFrequency(Frequency<T>::Hertz(frequencyHz));
    }

    /**
     * @brief Set mix between dry and wet signal.
     * @param mixNormalized Mix amount in normalized range [0, 1] (0 = full dry, 1 = full wet).
     */
    void setMix(T mixNormalized, bool skipSmoothing) {
        dryWetMixer.setMix(mixNormalized, skipSmoothing);
    }

    /**
     * @brief Set output gain.
     * @param gainDb Output gain in decibels.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setOutputGainDb(T gainDb, bool skipSmoothing = false) {
        outputGain.setTarget(Gain<T>::Decibels(gainDb).toLinear(), skipSmoothing);
    }

    /**
     * @brief Enable or disable oversampling.
     * @param enabled True to enable oversampling, false to disable.
     */
    void setOversamplingEnabled(bool enabled) { toggleOversampling = enabled; }

    /// Get number of channels.
    size_t getNumChannels() const { return numChannels; }

    /// Get sample rate.
    T getSampleRate() const { return sampleRate; }

    /// Get latency in samples at base sample rate.
    size_t getLatencySamples() const {
        return toggleOversampling ? distortionOS.getLatencySamples()
                                  : distortion.getLatencySamples();
    }

  private:
    // GLOBAL PARAMETERS
    size_t numChannels = 0;
    T sampleRate = T(44100);
    bool toggleOversampling = false;

    // PROCESSORS
    models::SaturationStage<T, true, true, OVERSAMPLING_FACTOR> distortionOS;
    models::SaturationStage<T, true, true, 1> distortion;
    DCBlocker<T> dcBlocker;
    DryWetMixer<T> dryWetMixer;
    DspParam<T> outputGain;

    // BUFFERS
    AudioBuffer<T> fxBuffer; // buffer for the effect processing
};

} // namespace jnsc::effects