// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Saturation stage model header file
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/utils/detail/config_utils.h"
#include <jonssonic/core/common/dsp_param.h>
#include <jonssonic/core/common/quantities.h>
#include <jonssonic/core/filters/biquad_filter.h>
#include <jonssonic/core/nonlinear/wave_shaper_processor.h>
#include <jonssonic/core/oversampling/oversampled_processor.h>

namespace jnsc::models {
/**
 * @brief Saturation stage model with oversampled waveshaping, pre- and post-filters, and pre and
 * post dynamics.
 * @tparam T Sample data type (e.g., float, double)
 * @tparam ShaperType Type of wave shaper nonlinearity
 * @tparam PreFilter If true, enables pre-waveshaper filtering
 * @tparam PostFilter If true, enables post-waveshaper filtering
 * @tparam OversamplingFactor Oversampling factor (1, 2, 4, 8, or 16)
 */
template <typename T,
          WaveShaperType ShaperType,
          bool PreFilter = true,
          bool PostFilter = true,
          size_t OversamplingFactor = 1>

class SaturationStage {
    static_assert(OversamplingFactor == 1 || OversamplingFactor == 2 || OversamplingFactor == 4 ||
                      OversamplingFactor == 8 || OversamplingFactor == 16,
                  "OversamplingFactor must be 1, 2, 4, 8, or 16.");

  public:
    /// Default constructor.
    SaturationStage() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     */
    SaturationStage(size_t newNumChannels, T newSampleRate) {
        prepare(newNumChannels, newSampleRate);
    }

    /// Default destructor.
    ~SaturationStage() = default;

    /// No copy nor move semantics
    SaturationStage(const SaturationStage&) = delete;
    SaturationStage& operator=(const SaturationStage&) = delete;
    SaturationStage(SaturationStage&&) = delete;
    SaturationStage& operator=(SaturationStage&&) = delete;

    /**
     * @brief Prepare the saturation stage for processing.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     */
    void prepare(size_t newNumChannels, size_t newMaxBlockSize, T newSampleRate) {
        // Clamp and set number of channels and sample rate
        numChannels = utils::detail::clampChannels(newNumChannels);
        sampleRate = utils::detail::clampSampleRate(newSampleRate);

        // Prepare oversampled processor if needed
        if constexpr (OversamplingFactor > 1)
            oversampledProcessor.prepare(numChannels, newMaxBlockSize);

        // Prepare wave shaper processor at proper sample rate
        waveShaper.prepare(numChannels, sampleRate * OversamplingFactor);

        // Prepare pre- and post-filters if enabled
        if constexpr (PreFilter)
            preFilter.prepare(numChannels, sampleRate);

        if constexpr (PostFilter)
            postFilter.prepare(numChannels, sampleRate);
    }

    /// Reset the saturation stage state.
    void reset() {
        if constexpr (OversamplingFactor > 1)
            oversampledProcessor.reset();

        waveShaper.reset();

        if constexpr (PreFilter)
            preFilter.reset();

        if constexpr (PostFilter)
            postFilter.reset();
    }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input sample pointers (one per channel)
     * @param output Output sample pointers (one per channel)
     * @param numSamples Number of samples to process
     * @note Must call @ref prepare before processing.
     */

    void processBlock(const T* const* input, T* const* output, size_t numSamples) {

        // PRE-FILTERING ENABLED SIGNAL PATH
        if constexpr (PreFilter) {
            preFilter.processBlock(input, output, numSamples);

            // Oversampled waveshaping
            if constexpr (OversamplingFactor > 1) {
                oversampledProcessor.processBlock(
                    output,
                    output,
                    numSamples,
                    [this](const T* const* in, T* const* out, size_t oversampledSamples) {
                        waveShaper.processBlock(in, out, oversampledSamples);
                    });

            }
            // No oversampling
            else
                waveShaper.processBlock(output, output, numSamples);
        }

        // PRE-FILTERING DISABLED SIGNAL PATH
        if constexpr (!PreFilter) {
            // Oversampled waveshaping
            if constexpr (OversamplingFactor > 1) {
                oversampledProcessor.processBlock(
                    input,
                    output,
                    numSamples,
                    [this](const T* const* in, T* const* out, size_t oversampledSamples) {
                        waveShaper.processBlock(in, out, oversampledSamples);
                    });

            }
            // No oversampling
            else
                waveShaper.processBlock(input, output, numSamples);
        }

        // Post-filtering if enabled
        if constexpr (PostFilter)
            postFilter.processBlock(output, output, numSamples);
    }

    /**
     * @brief Set control smoothing time for all parameters.
     * @param time Smoothing time
     */
    void setControlSmoothingTime(Time<T> time) { waveShaper.setControlSmoothingTime(time); }

    /**
     * @brief Set the drive (i.e. input gain of the waveshaper).
     * @param newGain New input gain
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setDrive(Gain<T> newGain, bool skipSmoothing = false) {
        waveShaper.setInputGain(newGain, skipSmoothing);
    }

    /**
     * @brief Set the bias amount.
     * @param newBias New bias amount
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setBias(T newBias, bool skipSmoothing = false) {
        waveShaper.setBias(newBias, skipSmoothing);
    }

    /**
     * @brief Set the asymmetry amount.
     * @param newAsymmetry New asymmetry amount
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setAsymmetry(T newAsymmetry, bool skipSmoothing = false) {
        waveShaper.setAsymmetry(newAsymmetry, skipSmoothing);
    }

    /**
     * @brief Set the shape parameter (only functional with Dynamic shaper).
     * @param newShape New shape parameter
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setShape(T newShape, bool skipSmoothing = false) {
        waveShaper.setShape(newShape, skipSmoothing);
    }

    /**
     * @brief Set the output gain of the saturation stage.
     * @param newGain New output gain
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setOutputGain(Gain<T> newGain, bool skipSmoothing = false) {
        waveShaper.setOutputGain(newGain, skipSmoothing);
    }

    /**
     * @brief Set the pre-filter type.
     * @param newType New filter type
     * @note Only applicable if PreFilter is true.
     */
    void setPreFilterType(BiquadType newType) {
        if constexpr (PreFilter)
            preFilter.setType(newType);
    }

    /**
     * @brief Set the pre-filter gain.
     * @param newGain New gain
     * @note Only applicable if PreFilter is true.
     */
    void setPreFilterGain(Gain<T> newGain) {
        if constexpr (PreFilter)
            preFilter.setGain(newGain);
    }

    /**
     * @brief Set the pre-filter cutoff frequency.
     * @param newFreq New cutoff frequency
     * @note Only applicable if PreFilter is true.
     */
    void setPreFilterFrequency(Frequency<T> newFreq) {
        if constexpr (PreFilter)
            preFilter.setFreq(newFreq);
    }

    /**
     * @brief Set the pre-filter Q factor.
     * @param newQ New Q factor
     * @note Only applicable if PreFilter is true.
     */

    void setPreFilterQ(T newQ) {
        if constexpr (PreFilter)
            preFilter.setQ(newQ);
    }

    /**
     * @brief Set the post-filter type.
     * @param newType New filter type
     * @note Only applicable if PostFilter is true.
     */
    void setPostFilterType(BiquadType newType) {
        if constexpr (PostFilter)
            postFilter.setType(newType);
    }

    /**
     * @brief Set the post-filter gain.
     * @param newGain New gain
     * @note Only applicable if PostFilter is true.
     */
    void setPostFilterGain(Gain<T> newGain) {
        if constexpr (PostFilter)
            postFilter.setGain(newGain);
    }

    /**
     * @brief Set the post-filter cutoff frequency.
     * @param newFreq New cutoff frequency
     * @note Only applicable if PostFilter is true.
     */
    void setPostFilterFrequency(Frequency<T> newFreq) {
        if constexpr (PostFilter)
            postFilter.setFreq(newFreq);
    }

    /**
     * @brief Set the post-filter Q factor.
     * @param newQ New Q factor
     * @note Only applicable if PostFilter is true.
     */

    void setPostFilterQ(T newQ) {
        if constexpr (PostFilter)
            postFilter.setQ(newQ);
    }

    /// Get the number of channels.
    size_t getNumChannels() const { return numChannels; }

    /// Get the sample rate in Hz.
    T getSampleRate() const { return sampleRate; }

    /// Get latency in samples at base sample rate.
    size_t getLatencySamples() const {
        if constexpr (OversamplingFactor > 1)
            return oversampledProcessor.getLatencySamples();
        else
            return 0;
    }

  private:
    // Config variables
    size_t numChannels = 0;
    T sampleRate = T(44100);

    // Components
    OversampledProcessor<T, OversamplingFactor> oversampledProcessor;
    WaveShaperProcessor<T, ShaperType> waveShaper;
    BiquadFilter<T> preFilter;
    BiquadFilter<T> postFilter;
};

} // namespace jnsc::models