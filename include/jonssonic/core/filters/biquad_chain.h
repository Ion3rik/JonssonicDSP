// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Biqaud filter chain class header file
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
 * @brief Multi-section biquad filter chain class.
 * @param T Sample data type (e.g., float, double)
 */
template <typename T>
class BiquadChain {
  public:
    /// Default constructor
    BiquadChain() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels
     * @param newNumSections Number of second-order sections
     * @param newSampleRate Sample rate
     */
    BiquadChain(size_t newNumChannels,
                size_t newNumSections,
                T newSampleRate) // Parameterized constructor (for the faint hearted)
    {
        prepare(newNumChannels, newNumSections, newSampleRate);
    }
    ~BiquadChain() = default;

    // No copy or move semantics
    BiquadChain(const BiquadChain&) = delete;
    BiquadChain& operator=(const BiquadChain&) = delete;
    BiquadChain(BiquadChain&&) = delete;
    BiquadChain& operator=(BiquadChain&&) = delete;

    /**
     * @brief Prepare the biquad chain for processing.
     * @param newNumChannels Number of channels
     * @param newNumSections Number of second-order sections
     * @param newSampleRate Sample rate
     * @note Must be called before processing.
     */
    void prepare(size_t newNumChannels, size_t newNumSections, T newSampleRate) {
        // Initialize parameters
        numChannels = utils::detail::clampChannels(newNumChannels);
        sampleRate = utils::detail::clampSampleRate(newSampleRate);
        numSections = std::clamp(newNumSections, size_t(1), detail::FilterLimits<T>::MAX_SECTIONS);

        // Initialize per-section parameter vectors
        freqNormalized.assign(numSections, T(0.25));   // default to quarter Nyquist
        Q.assign(numSections, T(0.707));               // default to Butterworth
        gain.assign(numSections, T(1));                // default to unity gain
        type.assign(numSections, BiquadType::Lowpass); // default to lowpass

        // Prepare the underlying biquad core processor
        biquadCore.prepare(numChannels, numSections);
        togglePrepared = true;

        // Update coefficients for all sections
        updateCoeffs();
    }

    /// Reset the filter state
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
     * @param numSamples Number of samples to process
     * @note Must call @ref prepare before processing.
     */
    void processBlock(const T* const* input, T* const* output, size_t numSamples) {
        biquadCore.processBlock(input, output, numSamples);
    }

    /**
     * @brief Set gain in dB for a specific section.
     * @param section Section index.
     * @param newGain Gain struct. (Value clamped to limits defined in filter_limits.h).
     * @note Coeffs are only updated if @ref prepare has been called before.
     */
    void setGain(size_t section, Gain<T> newGain) {
        // Early exit if not prepared
        if (!(togglePrepared))
            return;
        // clamp gain to avoid instability
        gain[section] = std::clamp(newGain.toLinear(),
                                   detail::FilterLimits<T>::MIN_GAIN_LIN,
                                   detail::FilterLimits<T>::MAX_GAIN_LIN);
        updateCoeffs(section);
    }

    /**
     * @brief Set frequency in various units for a specific section.
     * @param section Section index.
     * @param newFreq Frequency struct.
     * @note Coeffs are only updated if @ref prepare has been called before.
     */
    void setFreq(size_t section, Frequency<T> newFreq) {
        // Early exit if not prepared
        if (!(togglePrepared))
            return;
        // clamp normalized frequency to avoid instability
        freqNormalized[section] = std::clamp(newFreq.toNormalized(sampleRate),
                                             detail::FilterLimits<T>::MIN_FREQ_NORM,
                                             detail::FilterLimits<T>::MAX_FREQ_NORM);
        updateCoeffs(section);
    }
    /**
     * @brief Set Q factor for a specific section.
     * @param section Section index
     * @param newQ Q factor
     * @note Coeffs are only updated if @ref prepare has been called before.
     */
    void setQ(size_t section, T newQ) {
        // clamp Q to avoid instability
        Q[section] =
            std::clamp(newQ, detail::BiquadLimits<T>::MIN_Q, detail::BiquadLimits<T>::MAX_Q);

        updateCoeffs(section);
    }

    /**
     * @brief Set filter type for a specific section.
     * @param section Section index.
     * @param newType Filter type @ref BiquadType.
     * @note Coeffs are only updated if @ref prepare has been called before.
     */
    void setType(size_t section, BiquadType newType) {
        type[section] = newType;
        updateCoeffs(section);
    }

  private:
    bool togglePrepared = false;
    size_t numChannels = 0;
    size_t numSections = 0;
    T sampleRate = T(44100);
    std::vector<T> freqNormalized;
    std::vector<T> Q;
    std::vector<T> gain;
    std::vector<BiquadType> type;
    detail::BiquadCore<T> biquadCore;

    /**
     * @brief Update filter coefficients of single section.
     * @param section Section index
     */
    void updateCoeffs(size_t section) {
        // Early exit if not prepared
        if (!(togglePrepared))
            return;
        // Initialize coefficients variables
        T b0 = T(0), b1 = T(0), b2 = T(0), a1 = T(0), a2 = T(0);
        switch (type[section]) {
        case BiquadType::Lowpass:
            detail::computeLowpassCoeffs<T>(freqNormalized[section],
                                            Q[section],
                                            b0,
                                            b1,
                                            b2,
                                            a1,
                                            a2);
            biquadCore.setSectionCoeffs(section, b0, b1, b2, a1, a2);
            break;
        case BiquadType::Highpass:
            detail::computeHighpassCoeffs<T>(freqNormalized[section],
                                             Q[section],
                                             b0,
                                             b1,
                                             b2,
                                             a1,
                                             a2);
            biquadCore.setSectionCoeffs(section, b0, b1, b2, a1, a2);
            break;
        case BiquadType::Bandpass:
            detail::computeBandpassCoeffs<T>(freqNormalized[section],
                                             Q[section],
                                             b0,
                                             b1,
                                             b2,
                                             a1,
                                             a2);
            biquadCore.setSectionCoeffs(section, b0, b1, b2, a1, a2);
            break;
        case BiquadType::Allpass:
            detail::computeAllpassCoeffs<T>(freqNormalized[section],
                                            Q[section],
                                            b0,
                                            b1,
                                            b2,
                                            a1,
                                            a2);
            biquadCore.setSectionCoeffs(section, b0, b1, b2, a1, a2);
            break;
        case BiquadType::Notch:
            detail::computeNotchCoeffs<T>(freqNormalized[section], Q[section], b0, b1, b2, a1, a2);
            biquadCore.setSectionCoeffs(section, b0, b1, b2, a1, a2);
            break;
        case BiquadType::Peak:
            detail::computePeakCoeffs<T>(freqNormalized[section],
                                         Q[section],
                                         gain[section],
                                         b0,
                                         b1,
                                         b2,
                                         a1,
                                         a2);
            biquadCore.setSectionCoeffs(section, b0, b1, b2, a1, a2);
            break;
        case BiquadType::Lowshelf:
            detail::computeLowshelfCoeffs<T>(freqNormalized[section],
                                             Q[section],
                                             gain[section],
                                             b0,
                                             b1,
                                             b2,
                                             a1,
                                             a2);
            biquadCore.setSectionCoeffs(section, b0, b1, b2, a1, a2);
            break;
        case BiquadType::Highshelf:
            detail::computeHighshelfCoeffs<T>(freqNormalized[section],
                                              Q[section],
                                              gain[section],
                                              b0,
                                              b1,
                                              b2,
                                              a1,
                                              a2);
            biquadCore.setSectionCoeffs(section, b0, b1, b2, a1, a2);
            break;
        default:
            // Handle unsupported types or add implementations
            break;
        }
    }

    /**
     * @brief Update filter coefficients for all sections.
     */
    void updateCoeffs() {
        for (size_t s = 0; s < biquadCore.getNumSections(); ++s) {
            updateCoeffs(s);
        }
    }
};
} // namespace jnsc