// Jonssonic - A C++ audio DSP library
// Compressor effect header file
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/utils/detail/config_utils.h"
#include <jonssonic/core/common/dsp_param.h>
#include <jonssonic/core/common/quantities.h>
#include <jonssonic/models/dynamics/dynamics_stage.h>
#include <jonssonic/utils/buffer_utils.h>
#include <jonssonic/utils/math_utils.h>

namespace jnsc::effects {
/**
 * @brief Example compressor effect.
 * @tparam T Sample data type (e.g., float, double)
 */
template <typename T>
class Compressor {
    /**
     * @brief Tunable constants for the compressor effect
     * @param CONTROL_SMOOTH_TIME_MS Control smoothing time in milliseconds.
     * @param GAIN_SMOOTH_ATTACK_MS Gain smoother attack time in milliseconds.
     * @param GAIN_SMOOTH_RELEASE_MS Gain smoother release time in milliseconds.
     */
    static constexpr T CONTROL_SMOOTH_TIME_MS = T(50);
    static constexpr T GAIN_SMOOTH_ATTACK_MS = T(0.1);
    static constexpr T GAIN_SMOOTH_RELEASE_MS = T(5);

  public:
    /// Default constructor.
    Compressor() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels
     * @param newMaxBlockSize Maximum block size for processing (at base sample rate)
     * @param newSampleRate Sample rate in Hz
     */
    Compressor(size_t newNumChannels, T newSampleRate) { prepare(newNumChannels, newSampleRate); }

    /// Default destructor.
    ~Compressor() = default;

    /// No copy nor move semantics.
    Compressor(const Compressor&) = delete;
    Compressor& operator=(const Compressor&) = delete;
    Compressor(Compressor&&) = delete;
    Compressor& operator=(Compressor&&) = delete;

    /**
     * @brief Prepare the Compressor effect for processing.
     * @param newNumChannels Number of channels
     * @param newMaxBlockSize Maximum block size for processing (at base sample rate)
     * @param newSampleRate Sample rate in Hz
     */
    void prepare(size_t newNumChannels, T newSampleRate) {

        // Store global parameters
        numChannels = utils::detail::clampChannels(newNumChannels);
        sampleRate = utils::detail::clampSampleRate(newSampleRate);

        // Prepare compressor components
        compressor.prepare(numChannels, sampleRate);
        outputGain.prepare(numChannels, sampleRate);
        gainReductionOutput.resize(numChannels, T(1));

        // Configure fixed parameters
        compressor.setGainSmootherAttackTime(Time<T>::Milliseconds(GAIN_SMOOTH_ATTACK_MS));
        compressor.setGainSmootherReleaseTime(Time<T>::Milliseconds(GAIN_SMOOTH_RELEASE_MS));
        compressor.setControlSmoothingTime(Time<T>::Milliseconds(CONTROL_SMOOTH_TIME_MS));
        outputGain.setSmoothingTime(Time<T>::Milliseconds(CONTROL_SMOOTH_TIME_MS));

        // Set Default Parameters
        setThreshold(T(-24), true);
        setRatio(T(4), true);
        setKnee(T(6), true);
        setAttackTime(T(10), true);
        setReleaseTime(T(100), true);
        setOutputGain(T(0), true);

        // Mark as prepared
        togglePrepared = true;
    }

    void reset() {
        compressor.reset();
        outputGain.reset();
    }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input buffer (numChannels x numSamples)
     * @param detectorInput Detector input buffer (numChannels x numSamples)
     * @param output Output buffer (numChannels x numSamples)
     * @param numSamples Number of samples to process
     */
    void processBlock(const T* const* input,
                      const T* const* detectorInput,
                      T* const* output,
                      size_t numSamples) {

        // Process through compressor
        compressor.processBlock(input,
                                detectorInput,
                                output,
                                numSamples,
                                gainReductionOutput.data());

        // Apply output gain
        outputGain.applyToBuffer(output, numSamples);

        // Update gain reduction metering (max reduction across channels)
        gainReduction.store(
            *std::max_element(gainReductionOutput.begin(), gainReductionOutput.end()));
    }

    // SETTERS FOR PARAMETERS

    /**
     * @brief Set threshold in dB.
     * @param newthresholdDb New threshold value in dB
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setThreshold(T newthresholdDb, bool skipSmoothing = false) {
        compressor.setThreshold(newthresholdDb, skipSmoothing);
    }

    /**
     * @brief Set attack time in milliseconds.
     * @param attackTimeMs Attack time in milliseconds
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setAttackTime(T attackTimeMs, bool skipSmoothing = false) {
        compressor.setEnvelopeAttackTime(Time<T>::Milliseconds(attackTimeMs), skipSmoothing);
    }

    /**
     * @brief Set release time in milliseconds.
     * @param releaseTimeMs Release time in milliseconds
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setReleaseTime(T releaseTimeMs, bool skipSmoothing = false) {
        compressor.setEnvelopeReleaseTime(Time<T>::Milliseconds(releaseTimeMs), skipSmoothing);
    }

    /**
     * @brief Set compression ratio.
     * @param newRatio New ratio value
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setRatio(T newRatio, bool skipSmoothing = false) {
        compressor.setRatio(newRatio, skipSmoothing);
    }

    /**
     * @brief Set knee width in dB.
     * @param newKneeDb New knee width in dB
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setKnee(T newKneeDb, bool skipSmoothing = false) {
        compressor.setKnee(newKneeDb, skipSmoothing);
    }

    /**
     * @brief Set output gain in dB.
     * @param gainDb Output gain in dB
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setOutputGain(T gainDb, bool skipSmoothing = false) {
        outputGain.setTarget(utils::dB2Mag(gainDb), skipSmoothing);
    }

    /// Get number of channels.
    size_t getNumChannels() const { return numChannels; }

    /// Get sample rate.
    T getSampleRate() const { return sampleRate; }

    /// Check if the processor is prepared.
    bool isPrepared() const { return togglePrepared; }

    /// Get gain reduction value.
    T getGainReduction() const { return gainReduction.load(); }

  private:
    // Config variables
    size_t numChannels = 0;
    T sampleRate = T(44100);
    bool togglePrepared = false;

    // COMPONENTS
    models::CompressorRMSFeedforward<T> compressor;
    DspParam<T> outputGain;

    // Metering variables
    std::vector<T> gainReductionOutput;
    std::atomic<T> gainReduction{T(1)};
};

} // namespace jnsc::effects