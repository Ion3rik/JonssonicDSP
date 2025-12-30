// Jonssonic - A Modular Realtime C++ Audio DSP Library
// FirstOrderFilter class header file
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/utils/detail/config_utils.h"
#include "first_order_core.h"
#include "first_order_coeffs.h"

#include <filter_types.h>
#include <algorithm>

namespace jonssonic::filters {

/**
 * @brief Single section first-order filter wrapper class.
 * @param T Sample data type (e.g., float, double).
 */
template<typename T>
class FirstOrderFilter {
public:
    /// Default constructor.
    FirstOrderFilter() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels.
     * @param newSampleRate Sample rate in Hz.
     * @param newType Filter type (Default: Lowpass).
     */
    FirstOrderFilter(size_t newNumChannels, T newSampleRate, FirstOrderType newType = FirstOrderType::Lowpass) {
        prepare(newNumChannels, newSampleRate, newType);
    }

    /// Default destructor.
    ~FirstOrderFilter() = default;

    /// No copy nor move semantics.
    FirstOrderFilter(const FirstOrderFilter&) = delete;
    FirstOrderFilter& operator=(const FirstOrderFilter&) = delete;
    FirstOrderFilter(FirstOrderFilter&&) = delete;
    FirstOrderFilter& operator=(FirstOrderFilter&&) = delete;

    /**
     * @brief Prepare the first-order filter for processing.
     * @param newNumChannels Number of channels.
     * @param newSampleRate Sample rate in Hz.
     * @param newType Filter type (Default: Lowpass).
     */
    void prepare(size_t newNumChannels, T newSampleRate, FirstOrderType newType = FirstOrderType::Lowpass) {
        numChannels = utils::detail::clampChannels(newNumChannels);
        sampleRate = utils::detail::clampSampleRate(newSampleRate);
        type = newType;
        FirstOrderCore.prepare(numChannels, 1); // prepare single section
        updateCoeffs();
        togglePrepared = true;
    }

    /// Reset the filter state.
    void reset() { FirstOrderCore.reset(); }

    /**
     * @brief Process a single sample for a given channel.
     * @param ch Channel index.
     * @param input Input sample.
     * @return Output sample.
     * @note Must call @ref prepare before processing.
     */
    T processSample(size_t ch, T input) {
        return FirstOrderCore.processSample(ch, input);
    }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input sample pointers (one per channel).
     * @param output Output sample pointers (one per channel).
     * @param numSamples Number of samples to process.
     * @note Must call @ref prepare before processing.
     */
    void processBlock(const T* const* input, T* const* output, size_t numSamples) {
        FirstOrderCore.processBlock(input, output, numSamples);
    }

    /**
     * @brief Set the cutoff frequency in Hz (clamped between 1 Hz and Nyquist).
     * @param newFreqHz Cutoff frequency in Hz.
     * @note Coeffs are only updated if @ref prepare has been called before.
     */
    void setFreq(T newFreqHz) {
        assert(sampleRate > T(0) && "Sample rate must be set before setting frequency");
        freqNormalized = std::clamp(newFreqHz / sampleRate, T(1) / sampleRate, T(0.5));
        updateCoeffs();
    }

    /**
     * @brief Set the cutoff frequency as a normalized value.
     * @param normalizedFreq Normalized frequency (clamped between 1 Hz and Nyquist).
     * @note This allows setting frequency before sample rate is known.
     */
    void setFreqNormalized(T normalizedFreq) {
        assert(sampleRate > T(0) && "Sample rate must be set before setting frequency");
        freqNormalized = std::clamp(normalizedFreq, T(1) / sampleRate, T(0.5));
        updateCoeffs();
    }

    /**
     * @brief Set the filter type.
     * @param newType Filter type.
     * @note Coeffs are only updated if @ref prepare has been called before.
     */
    void setType(FirstOrderType newType) {
        type = newType;
        updateCoeffs();
    }

    /**
     * @brief Set the linear filter gain. (Applicable for shelf filters)
     * @param newGainLin Gain in linear scale.
     * @note Coeffs are only updated if @ref prepare has been called before.
     */
    void setGainLinear(T newGainLin) {
        gain = std::clamp(newGainLin, T(0.001), T(10));
        updateCoeffs();
    }

    /**
     * @brief Set the filter gain in dB. (Applicable for shelf filters)
     * @param newGainDb Gain in dB.
     * @note Coeffs are only updated if @ref prepare has been called before.
     */
    void setGainDb(T newGainDb) {
        gain = Jonssonic::dB2Mag(newGainDb);
        updateCoeffs();
    }

private:
    bool togglePrepared = false;
    size_t numChannels = 0;
    T sampleRate = T(44100);
    T freqNormalized = T(0.25); 
    T gain = T(1); 

    FirstOrderType type;
    FirstOrderCore<T> FirstOrderCore;

    void updateCoeffs() {
        // Early exit if not prepared
        if(!togglePrepared) return; 
        
        // Initialize coefficients variables
        T b0 = T(0), b1 = T(0), a1 = T(0);

        // Compute coefficients based on filter type
        switch (type) {
            case FirstOrderType::Lowpass:
                jonssonic::detail::computeFirstOrderLowpassCoeffs<T>(freqNormalized, b0, b1, a1);
                FirstOrderCore.setSectionCoeffs(0, b0, b1, a1);
                break;
            case FirstOrderType::Highpass:
                jonssonic::detail::computeFirstOrderHighpassCoeffs<T>(freqNormalized, b0, b1, a1);
                FirstOrderCore.setSectionCoeffs(0, b0, b1, a1);
                break;
            case FirstOrderType::Allpass:
                jonssonic::detail::computeFirstOrderAllpassCoeffs<T>(freqNormalized, b0, b1, a1);
                FirstOrderCore.setSectionCoeffs(0, b0, b1, a1);
                break;
            case FirstOrderType::Lowshelf:
                jonssonic::detail::computeFirstOrderLowshelfCoeffs<T>(freqNormalized, gain, b0, b1, a1);
                FirstOrderCore.setSectionCoeffs(0, b0, b1, a1);
                break;
            case FirstOrderType::Highshelf:
                jonssonic::detail::computeFirstOrderHighshelfCoeffs<T>(freqNormalized, gain, b0, b1, a1);
                FirstOrderCore.setSectionCoeffs(0, b0, b1, a1);
                break;
            default:
                // Handle unsupported types or add implementations
                break;
        }
    }
};


} // namespace jonssonic::filters
