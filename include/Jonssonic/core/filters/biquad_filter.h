// Jonssonic - A C++ audio DSP library
// Biqaud filter class header file
// SPDX-License-Identifier: MIT

#pragma once

#include "jonssonic/utils/detail/config_utils.h"
#include "detail/biquad_core.h"
#include "detail/biquad_coeffs.h"
#include <jonssonic/core/filters/filter_types.h>
#include <jonssonic/core/nonlinear/wave_shaper.h>
#include <jonssonic/utils/math_utils.h>
#include <algorithm>

namespace jonssonic::filters
{

/**
 * @brief Biquad Filter
 * @param T Sample data type (e.g., float, double)
 * @param ShaperType Type of waveshaper for nonlinear processing (default: None)
 */
template<typename T, WaveShaperType ShaperType = WaveShaperType::None>
class BiquadFilter
{
public:
    /// Default constructor
    BiquadFilter() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels.
     * @param newSampleRate Sample rate in Hz.
     * @param newType Filter type (Default: Lowpass).
     */
    BiquadFilter(size_t newNumChannels, T newSampleRate, BiquadType newType = BiquadType::Lowpass)
    {
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
    void prepare(size_t newNumChannels, T newSampleRate, BiquadType newType = BiquadType::Lowpass)
    {

        sampleRate = utils::detail::clampSampleRate(newSampleRate);   
        BiquadCore.prepare(newNumChannels, 1);  
        type = newType;  

        freqNormalized = T(0.25);               
        Q = T(0.707);                           
        gain = T(1);                            
                               

        updateCoeffs();                         
        togglePrepared = true;
    }

    /// Reset the filter state.
    void reset() { BiquadCore.reset(); }

    /**
     * @brief Process a single sample for a given channel.
     * @param ch Channel index.
     * @param input Input sample.
     * @return Output sample.
     * @note Must call @ref prepare before processing.
     */
    T processSample(size_t ch, T input)
    {
        return BiquadCore.processSample(ch, input);
    }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input sample pointers (one per channel).
     * @param output Output sample pointers (one per channel).
     * @param numSamples Number of samples to process.
     * @note Must call @ref prepare before processing.
     */
    void processBlock(const T* const* input, T* const* output, size_t numSamples)
    {
        BiquadCore.processBlock(input, output, numSamples);
    }

    /**
     * @brief Set the filter gain in dB. (Applicable for shelf and peak filters)
     * @param newGainDb Gain in dB (clamped between -60 dB and +20 dB).
     * @note Coeffs are only updated if @ref prepare has been called before.
     */
    void setGainDb(T newGainDb)
    {
        newGainDb = std::clamp(newGainDb, T(-60), T(20));
        gain = Jonssonic::dB2Mag(newGainDb); 
        updateCoeffs();
    }

    /**
     * @brief Set the filter gain in linear scale. (Applicable for shelf and peak filters)
     * @param newGainLin Gain in linear scale (clamped between 0.001 and 10).
     * @note Coeffs are only updated if @ref prepare has been called before.
     */
    void setGainLinear(T newGainLin)
    {   
        newGainLin = std::clamp(newGainLin, T(0.001), T(10));
        gain = newGainLin;
        updateCoeffs();
    }

    /**
     * @brief Set the cutoff/center frequency in Hz.
     * @param newFreqHz Cutoff/center frequency in Hz (clamped between 1 Hz and Nyquist).
     * @note Coeffs are only updated if @ref prepare has been called before.
     */
    void setFreq(T newFreqHz)
    {
        freqNormalized = std::clamp(newFreqHz / sampleRate, T(1) / sampleRate, T(0.5));
        updateCoeffs();
    }

    /**
     * @brief Set the quality factor Q.
     * @param newQ Quality factor Q (clamped between 0.1 and 10).
     * @note Coeffs are only updated if @ref prepare has been called before.
     */
    void setQ(T newQ)
    {
        Q = std::clamp(newQ, T(0.1), T(10));
        updateCoeffs();
    }

    /**
     * @brief Set the filter type.
     * @param newType Filter type.
     * @note Coeffs are only updated if @ref prepare has been called before.
     */
    void setType(BiquadType newType)
    {
        type = newType;
        updateCoeffs();
    }

private:
    bool togglePrepared = false;
    T sampleRate = T(44100); // default sample rate
    size_t numChannels = 0; // number of channels
    T freqNormalized = T(0.25); // default to quarter Nyquist
    T Q;    // quality factor
    T gain; // linear gain for shelving/peak filters
    BiquadType type; // filter type
    BiquadCore<T, ShaperType> BiquadCore;

    void updateCoeffs()
    {   
        // Early exit if not prepared
        if (!(togglePrepared))
            return;

        // Initialize coefficients variables
        T b0 = T(0), b1 = T(0), b2 = T(0), a1 = T(0), a2 = T(0);

        // Compute coefficients based on filter type
        switch (type)
        {
            case BiquadType::Lowpass:
                jonssonic::detail::computeLowpassCoeffs<T>(freqNormalized, Q,
                    b0, b1, b2, a1, a2);
                BiquadCore.setSectionCoeffs(0, b0, b1, b2, a1, a2);
                break;
            case BiquadType::Highpass:
                jonssonic::detail::computeHighpassCoeffs<T>(freqNormalized, Q,
                    b0, b1, b2, a1, a2);
                BiquadCore.setSectionCoeffs(0, b0, b1, b2, a1, a2);
                break;
            case BiquadType::Bandpass:
                jonssonic::detail::computeBandpassCoeffs<T>(freqNormalized, Q,
                    b0, b1, b2, a1, a2);
                BiquadCore.setSectionCoeffs(0, b0, b1, b2, a1, a2);
                break;
            case BiquadType::Allpass:
                jonssonic::detail::computeAllpassCoeffs<T>(freqNormalized, Q,
                    b0, b1, b2, a1, a2);
                BiquadCore.setSectionCoeffs(0, b0, b1, b2, a1, a2);
                break;
            case BiquadType::Notch:
                jonssonic::detail::computeNotchCoeffs<T>(freqNormalized, Q,
                    b0, b1, b2, a1, a2);
                BiquadCore.setSectionCoeffs(0, b0, b1, b2, a1, a2);
                break;
            case BiquadType::Peak:
                jonssonic::detail::computePeakCoeffs<T>(freqNormalized, Q, gain,
                    b0, b1, b2, a1, a2);
                BiquadCore.setSectionCoeffs(0, b0, b1, b2, a1, a2);
                break;
            case BiquadType::Lowshelf:
                jonssonic::detail::computeLowshelfCoeffs<T>(freqNormalized, Q, gain,
                    b0, b1, b2, a1, a2);
                BiquadCore.setSectionCoeffs(0, b0, b1, b2, a1, a2);
                break;
            case BiquadType::Highshelf:
                jonssonic::detail::computeHighshelfCoeffs<T>(freqNormalized, Q, gain,
                    b0, b1, b2, a1, a2);
                BiquadCore.setSectionCoeffs(0, b0, b1, b2, a1, a2);
                break;
            default:
                // Handle unsupported types or add implementations
                break;
        }
    }
};
} // namespace jonssonic::filters