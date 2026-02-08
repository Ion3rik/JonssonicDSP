// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Biqaud filter class header file
// SPDX-License-Identifier: MIT

#pragma once

#include "detail/biquad_coeffs.h"
#include "detail/biquad_core.h"
#include "detail/filter_limits.h"
#include "jonssonic/utils/detail/config_utils.h"

#include <algorithm>
#include <jonssonic/core/common/dsp_param.h>
#include <jonssonic/core/common/quantities.h>
#include <jonssonic/core/filters/filter_types.h>
#include <jonssonic/utils/math_utils.h>

namespace jnsc {

// =====================================================================================================================
// RUNTIME TYPE SELECTION BIQUAD FILTER
// =====================================================================================================================
/**
 * @brief Multi-section biquad filter class with runtime @ref BiquadType selection.
 * @param T Sample data type (e.g., float, double).
 * @note This specialization is intended for quasi time-invariant filtering.
 *       -> Accurate, potentially expensive, infrequent coefficient updates.
 *       -> Efficient @ref processSample() and @ref processBlock() calls with fixed coefficients.
 */
template <typename T>
class BiquadFilter {
  public:
    /// Default constructor
    BiquadFilter() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels
     * @param newNumSections Number of second-order sections
     * @param newSampleRate Sample rate
     */
    BiquadFilter(size_t newNumChannels, size_t newNumSections, T newSampleRate) {
        prepare(newNumChannels, newNumSections, newSampleRate);
    }
    /// Default destructor
    ~BiquadFilter() = default;

    /// No copy or move semantics
    BiquadFilter(const BiquadFilter&) = delete;
    BiquadFilter& operator=(const BiquadFilter&) = delete;
    BiquadFilter(BiquadFilter&&) = delete;
    BiquadFilter& operator=(BiquadFilter&&) = delete;

    /**
     * @brief Prepare the biquad filter for processing.
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
     * @brief Set gain for a specific section.
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
        // Early exit if not prepared
        if (!(togglePrepared))
            return;
        // clamp Q to avoid instability
        Q[section] = std::clamp(newQ, detail::BiquadLimits<T>::MIN_Q, detail::BiquadLimits<T>::MAX_Q);

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
            detail::computeLowpassCoeffs<T>(freqNormalized[section], Q[section], b0, b1, b2, a1, a2);
            biquadCore.setSectionCoeffs(section, b0, b1, b2, a1, a2);
            break;
        case BiquadType::Highpass:
            detail::computeHighpassCoeffs<T>(freqNormalized[section], Q[section], b0, b1, b2, a1, a2);
            biquadCore.setSectionCoeffs(section, b0, b1, b2, a1, a2);
            break;
        case BiquadType::Bandpass:
            detail::computeBandpassCoeffs<T>(freqNormalized[section], Q[section], b0, b1, b2, a1, a2);
            biquadCore.setSectionCoeffs(section, b0, b1, b2, a1, a2);
            break;
        case BiquadType::Allpass:
            detail::computeAllpassCoeffs<T>(freqNormalized[section], Q[section], b0, b1, b2, a1, a2);
            biquadCore.setSectionCoeffs(section, b0, b1, b2, a1, a2);
            break;
        case BiquadType::Notch:
            detail::computeNotchCoeffs<T>(freqNormalized[section], Q[section], b0, b1, b2, a1, a2);
            biquadCore.setSectionCoeffs(section, b0, b1, b2, a1, a2);
            break;
        case BiquadType::Peak:
            detail::computePeakCoeffs<T>(freqNormalized[section], Q[section], gain[section], b0, b1, b2, a1, a2);
            biquadCore.setSectionCoeffs(section, b0, b1, b2, a1, a2);
            break;
        case BiquadType::Lowshelf:
            detail::computeLowshelfCoeffs<T>(freqNormalized[section], Q[section], gain[section], b0, b1, b2, a1, a2);
            biquadCore.setSectionCoeffs(section, b0, b1, b2, a1, a2);
            break;
        case BiquadType::Highshelf:
            detail::computeHighshelfCoeffs<T>(freqNormalized[section], Q[section], gain[section], b0, b1, b2, a1, a2);
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

// =====================================================================================================================
// COMPILE-TIME TYPE SELECTION BIQUAD FILTER
// =====================================================================================================================
/**
 * @brief Multi-section biquad filter class with compile-time @ref BiquadType selection.
 * @param T Sample data type (e.g., float, double).
 * @param FilterType A single @BiquadType selected at compile time for each section.
 * @note This specilication is intended for time-variant filtering.
 *      -> Approximated efficient audio rate coefficient updates.
 *      -> Less efficient @ref processSample() and @ref processBlock() calls with time-varying coefficients.
 */
template <typename T, BiquadType Type>
class BiquadFilter {
  public:
    /// Default constructor
    BiquadFilter() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels
     * @param newNumSections Number of second-order sections
     * @param newSampleRate Sample rate
     */
    BiquadFilter(size_t newNumChannels, size_t newNumSections, T newSampleRate) {
        prepare(newNumChannels, newNumSections, newSampleRate);
    }

    /// Default destructor
    ~BiquadFilter() = default;

    /// No copy or move semantics
    BiquadFilter(const BiquadFilter&) = delete;
    BiquadFilter& operator=(const BiquadFilter&) = delete;
    BiquadFilter(BiquadFilter&&) = delete;
    BiquadFilter& operator=(BiquadFilter&&) = delete;

    /**
     * @brief Prepare the biquad filter for processing.
     * @param newNumChannels Number of channels
     * @param newNumSections Number of second-order sections
     * @param newSampleRate Sample rate
     * @note Must be called before processing.
     */
    void prepare(size_t newNumChannels, size_t newNumSections, T newSampleRate) {

        // Clamp and store config variables
        numChannels = utils::detail::clampChannels(newNumChannels);
        sampleRate = utils::detail::clampSampleRate(newSampleRate);
        numSections = std::clamp(newNumSections, size_t(1), detail::FilterLimits<T>::MAX_SECTIONS);

        // Prepare core processor
        biquadCore.prepare(numChannels, numSections);

        // Prepare parameters
        freqNormalized.prepare(numSections, sampleRate);
        Q.prepare(numSections, sampleRate);
        gain.prepare(numSections, sampleRate);

        // Clamp parameter ranges
        freqNormalized.setBounds(detail::FilterLimits<T>::MIN_FREQ_NORM, detail::FilterLimits<T>::MAX_FREQ_NORM);
        Q.setBounds(detail::BiquadLimits<T>::MIN_Q, detail::BiquadLimits<T>::MAX_Q);
        gain.setBounds(detail::FilterLimits<T>::MIN_GAIN_LIN, detail::FilterLimits<T>::MAX_GAIN_LIN);
    }

    /// Reset the filter state
    void reset() { biquadCore.reset(); }

    /**
     * @brief Process a single sample for a given channel with frequency modulation.
     * @param ch Channel index.
     * @param input Input sample.
     * @param mod Frequency modulation input.
     * @return Output sample.
     * @note Must call @ref prepare before processing.
     */
    T processSample(size_t ch, T input, T mod) {
        // Apply frequency modulation
        T modulatedFreq = freqNormalized.applyAdditiveMod(ch, mod);
        // Update coefficients for this section based on modulated frequency
        updateCoeffs(ch);
        // Process sample with updated coefficients
        return biquadCore.processSample(ch, input);
    }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input sample pointers (one per channel).
     * @param output Output sample pointers (one per channel).
     * @param mod Frequency modulation input pointers (one per channel).
     * @param numSamples Number of samples to process
     * @note Must call @ref prepare before processing.
     */
    void processBlock(const T* const* input, T* const* output, const T* const* mod, size_t numSamples) {}

    /**
     * @brief Set gain for a specific section.
     * @param section Section index.
     * @param newGain Gain struct. (Value clamped to limits defined in filter_limits.h).
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setGain(size_t section, Gain<T> newGain, bool skipSmoothing = false) {
        gain.setTarget(section, newGain.toLinear(), skipSmoothing);
    }

    /**
     * @brief Set frequency for a specific section.
     * @param section Section index.
     * @param newFreq Frequency struct.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setFreq(size_t section, Frequency<T> newFreq, bool skipSmoothing = false) {
        freqNormalized.setTarget(section, newFreq.toNormalized(sampleRate), skipSmoothing);
    }

    /**
     * @brief Set Q factor for a specific section.
     * @param section Section index
     * @param newQ Q factor
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setQ(size_t section, T newQ, bool skipSmoothing = false) { Q.setTarget(section, newQ, skipSmoothing); }

  private:
    size_t numChannels = 0;
    size_t numSections = 0;
    T sampleRate = T(44100);
    DspParam<T> freqNormalized;
    DspParam<T> Q;
    DspParam<T> gain;
    detail::BiquadCore<T> biquadCore;

    /**
     * @brief Update section coefficients with optional frequency modulation.
     * @param section Section index.
     * @param mod Frequency modulation input.
     */
    void updateCoeffs(size_t section, T mod = T(0)) {
        // Initialize coefficients variables
        T b0 = T(0), b1 = T(0), b2 = T(0), a1 = T(0), a2 = T(0);

        // Apply modulation to frequency parameter
        T modulatedFreq = freqNormalized.applyAdditiveMod(section, mod);

        // Compute coefficients based on compile-time filter type
        if constexpr (Type == BiquadType::Lowpass)
            detail::computeLowpassCoeffs<T>(modulatedFreq, Q, b0, b1, b2, a1, a2);
        if constexpr (Type == BiquadType::Highpass)
            detail::computeHighpassCoeffs<T>(modulatedFreq, Q, b0, b1, b2, a1, a2);
        if constexpr (Type == BiquadType::Bandpass)
            detail::computeBandpassCoeffs<T>(modulatedFreq, Q, b0, b1, b2, a1, a2);
        if constexpr (Type == BiquadType::Allpass)
            detail::computeAllpassCoeffs<T>(modulatedFreq, Q, b0, b1, b2, a1, a2);
        if constexpr (Type == BiquadType::Notch)
            detail::computeNotchCoeffs<T>(modulatedFreq, Q, b0, b1, b2, a1, a2);
        if constexpr (Type == BiquadType::Peak)
            detail::computePeakCoeffs<T>(modulatedFreq, Q, gain, b0, b1, b2, a1, a2);
        if constexpr (Type == BiquadType::Lowshelf)
            detail::computeLowshelfCoeffs<T>(modulatedFreq, Q, gain, b0, b1, b2, a1, a2);
        if constexpr (Type == BiquadType::Highshelf)
            detail::computeHighshelfCoeffs<T>(modulatedFreq, Q, gain, b0, b1, b2, a1, a2);

        // Set coefficients for all sections.
        for (size_t section = 0; section < numSections; ++section)
            biquadCore.setSectionCoeffs(section, b0, b1, b2, a1, a2);
    };

} // namespace jnsc