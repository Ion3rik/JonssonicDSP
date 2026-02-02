// Jonssonic - A C++ audio DSP library
// Biqaud filter class header file
// SPDX-License-Identifier: MIT

#pragma once

#include "detail/biquad_coeffs.h"
#include "detail/biquad_core.h"
#include "detail/filter_limits.h"
#include "jonssonic/utils/detail/config_utils.h"

#include <algorithm>
#include <jonssonic/core/common/quantities.h>
#include <jonssonic/core/filters/filter_types.h>
#include <jonssonic/utils/math_utils.h>

namespace jnsc {

/**
 * @brief Biquad filter wrapper for single section with integrated coeff computation for common
 * types.
 * @param T Sample data type (e.g., float, double)
 */
template <typename T>
class BiquadFilter {
  public:
    /// Default constructor
    BiquadFilter() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels.
     * @param newSampleRate Sample rate in Hz.
     * @param newType Filter type (Default: Lowpass).
     */
    BiquadFilter(size_t newNumChannels, T newSampleRate, BiquadType newType = BiquadType::Lowpass) {
        prepare(newNumChannels, newSampleRate, newType);
    }

    /// Default destructor.
    ~BiquadFilter() = default;

    /// No copy nor move semantics.
    BiquadFilter(const BiquadFilter&) = delete;
    BiquadFilter& operator=(const BiquadFilter&) = delete;
    BiquadFilter(BiquadFilter&&) = delete;
    BiquadFilter& operator=(BiquadFilter&&) = delete;

    /**
     * @brief Prepare the biquad filter for processing.
     * @param newNumChannels Number of channels.
     * @param newSampleRate Sample rate in Hz.
     * @param newType Filter type (Default: Lowpass).
     */
    void prepare(size_t newNumChannels, T newSampleRate, BiquadType newType = BiquadType::Lowpass) {
        numChannels = utils::detail::clampChannels(newNumChannels);
        sampleRate = utils::detail::clampSampleRate(newSampleRate);
        biquadCore.prepare(numChannels, 1);
        type = newType;

        freqNormalized = T(0.25);
        Q = T(0.707);
        gain = T(1);

        updateCoeffs();
        togglePrepared = true;
    }

    /// Reset the filter state.
    void reset() { biquadCore.reset(); }

    /**
     * @brief Process a single sample for a given channel.
     * @param ch Channel index.
     * @param input Input sample.
     * @return Output sample.
     * @note Must call @ref prepare before processing.
     */
    T processSample(size_t ch, T input) { return biquadCore.processSample(ch, input); }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input sample pointers (one per channel).
     * @param output Output sample pointers (one per channel).
     * @param numSamples Number of samples to process.
     * @note Must call @ref prepare before processing.
     */
    void processBlock(const T* const* input, T* const* output, size_t numSamples) {
        biquadCore.processBlock(input, output, numSamples);
    }

    /**
     * @brief Set the filter gain. (Applicable for shelf and peak filters)
     * @param newGain Gain struct. (Clamped to avoid instability)
     * @note Coeffs are only updated if @ref prepare has been called before.
     */
    void setGain(Gain<T> newGain) {
        // Early exit if not prepared
        if (!(togglePrepared))
            return;

        // clamp gain to avoid instability
        gain = std::clamp(newGain.toLinear(),
                          detail::FilterLimits<T>::MIN_GAIN_LIN,
                          detail::FilterLimits<T>::MAX_GAIN_LIN);
        updateCoeffs();
    }

    /**
     * @brief Set the cutoff/center frequency in Hz.
     * @param newFreq Cutoff/center frequency struct. (Clamped to avoid instability)
     * @note Coeffs are only updated if @ref prepare has been called before.
     */
    void setFreq(Frequency<T> newFreq) {
        // Early exit if not prepared
        if (!(togglePrepared))
            return;
        // clamp normalized frequency to avoid instability
        freqNormalized = std::clamp(newFreq.toNormalized(sampleRate),
                                    detail::FilterLimits<T>::MIN_FREQ_NORM,
                                    detail::FilterLimits<T>::MAX_FREQ_NORM);
        updateCoeffs();
    }

    /**
     * @brief Set the quality factor Q.
     * @param newQ Quality factor Q (Clamped to avoid instability)
     * @note Coeffs are only updated if @ref prepare has been called before.
     */
    void setQ(T newQ) {
        Q = std::clamp(newQ, detail::BiquadLimits<T>::MIN_Q, detail::BiquadLimits<T>::MAX_Q);
        updateCoeffs();
    }

    /**
     * @brief Set the filter type.
     * @param newType Filter type.
     * @note Coeffs are only updated if @ref prepare has been called before.
     */
    void setType(BiquadType newType) {
        type = newType;
        updateCoeffs();
    }

  private:
    bool togglePrepared = false;
    T sampleRate = T(44100);
    size_t numChannels = 0;
    T freqNormalized = T(0.25);
    T Q;
    T gain;
    BiquadType type;
    detail::BiquadCore<T> biquadCore;

    void updateCoeffs() {
        // Early exit if not prepared
        if (!(togglePrepared))
            return;

        // Initialize coefficients variables
        T b0 = T(0), b1 = T(0), b2 = T(0), a1 = T(0), a2 = T(0);

        // Compute coefficients based on filter type
        switch (type) {
        case BiquadType::Lowpass:
            detail::computeLowpassCoeffs<T>(freqNormalized, Q, b0, b1, b2, a1, a2);
            biquadCore.setSectionCoeffs(0, b0, b1, b2, a1, a2);
            break;
        case BiquadType::Highpass:
            detail::computeHighpassCoeffs<T>(freqNormalized, Q, b0, b1, b2, a1, a2);
            biquadCore.setSectionCoeffs(0, b0, b1, b2, a1, a2);
            break;
        case BiquadType::Bandpass:
            detail::computeBandpassCoeffs<T>(freqNormalized, Q, b0, b1, b2, a1, a2);
            biquadCore.setSectionCoeffs(0, b0, b1, b2, a1, a2);
            break;
        case BiquadType::Allpass:
            detail::computeAllpassCoeffs<T>(freqNormalized, Q, b0, b1, b2, a1, a2);
            biquadCore.setSectionCoeffs(0, b0, b1, b2, a1, a2);
            break;
        case BiquadType::Notch:
            detail::computeNotchCoeffs<T>(freqNormalized, Q, b0, b1, b2, a1, a2);
            biquadCore.setSectionCoeffs(0, b0, b1, b2, a1, a2);
            break;
        case BiquadType::Peak:
            detail::computePeakCoeffs<T>(freqNormalized, Q, gain, b0, b1, b2, a1, a2);
            biquadCore.setSectionCoeffs(0, b0, b1, b2, a1, a2);
            break;
        case BiquadType::Lowshelf:
            detail::computeLowshelfCoeffs<T>(freqNormalized, Q, gain, b0, b1, b2, a1, a2);
            biquadCore.setSectionCoeffs(0, b0, b1, b2, a1, a2);
            break;
        case BiquadType::Highshelf:
            detail::computeHighshelfCoeffs<T>(freqNormalized, Q, gain, b0, b1, b2, a1, a2);
            biquadCore.setSectionCoeffs(0, b0, b1, b2, a1, a2);
            break;
        default:
            // Handle unsupported types or add implementations
            break;
        }
    }
};
} // namespace jnsc