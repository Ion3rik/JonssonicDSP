// Jonssonic - A C++ audio DSP library
// Biqaud filter chain class header file
// SPDX-License-Identifier: MIT

#pragma once
#include "BiquadCore.h"
#include "FilterTypes.h"
#include "BiquadCoeffs.h"
#include "../../utils/MathUtils.h"
#include "../nonlinear/WaveShaper.h"
#include <algorithm>

namespace Jonssonic
{


/**
 * @brief This class implements a chain of biquad filters, with each section configurable independently.
 * @param T Sample data type (e.g., float, double)
 */
template<typename T, WaveShaperType ShaperType = WaveShaperType::None>
class BiquadChain
{
public:
    // Constructor and Destructor
    BiquadChain() = default;
    BiquadChain(size_t newNumChannels, size_t newNumSections, T newSampleRate) // Parameterized constructor (for the faint hearted)
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
    void prepare(size_t newNumChannels, size_t newNumSections, T newSampleRate)
    {
        // Initialize parameters
        sampleRate = newSampleRate;
        freqNormalized.assign(newNumSections, T(0.25));     // default to quarter Nyquist
        Q.assign(newNumSections, T(0.707));                 // default to Butterworth
        gain.assign(newNumSections, T(1));                  // default to unity gain
        type.assign(newNumSections, BiquadType::Lowpass);   // default to lowpass
        BiquadCore.prepare(newNumChannels, newNumSections); // initialize core processor

        // Update coefficients for all sections
        updateCoeffs();
    }

    void reset()
    {
        BiquadCore.reset();
    }

    /**
     * @brief Process a single sample for a given channel.
     * @param ch Channel index
     * @param input Input sample
     * @return Output sample
     */
    T processSample(size_t ch, T input)
    {
        return BiquadCore.processSample(ch, input);
    }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input sample pointers (one per channel)
     * @param output Output sample pointers (one per channel)
     * @param numSamples Number of samples to process
     */
    void processBlock(const T* const* input, T* const* output, size_t numSamples)
    {
        BiquadCore.processBlock(input, output, numSamples);
    }

    /** 
     * @brief Set gain in dB for a specific section.
     * @param section Section index
     * @param newGainDb Gain in decibels
    */
    void setGainDb(size_t section, T newGainDb)
    {
        // clamp gain to avoid instability
        newGainDb = std::clamp(newGainDb, T(-60), T(20));
        gain[section] = Jonssonic::dB2Mag(newGainDb); // convert to linear
        updateCoeffs(section);
    }

    /** 
     * @brief Set gain in linear scale for a specific section.
     * @param section Section index
     * @param newGainLin Linear gain
    */
    void setGainLinear(size_t section, T newGainLin)
    {   
        // clamp gain to avoid instability
        newGainLin = std::clamp(newGainLin, T(0.001), T(10));
        gain[section] = newGainLin;
        updateCoeffs(section);
    }

    /** 
     * @brief Set frequency in Hz for a specific section.
     * @param section Section index
     * @param newFreqHz Frequency in Hertz
    */
    void setFreq(size_t section,T newFreqHz)
    {
        // Convert Hz to normalized frequency (0..0.5)
        if (sampleRate > T(0)) {
            freqNormalized[section] = std::clamp(newFreqHz / sampleRate, T(0), T(0.5));
        } else {
            freqNormalized[section] = std::clamp(newFreqHz / T(44100), T(0), T(0.5)); // default to 44.1kHz sample rate
        }
        updateCoeffs(section);
    }
    /** 
     * @brief Set Q factor for a specific section.
     * @param section Section index
     * @param newQ Q factor
    */
    void setQ(size_t section, T newQ)
    {
        // clamp Q to avoid instability
        Q[section] = std::clamp(newQ, T(0.1), T(10));
        updateCoeffs(section);
    }

    /** 
     * @brief Set filter type for a specific section.
     * @param section Section index
     * @param newType Filter type
     * @note BiquadType is defined in FilterTypes.h
    */
    void setType(size_t section, BiquadType newType)
    {
        type[section] = newType;
        updateCoeffs(section);
    }

private:
    T sampleRate = T(44100);                // default sample rate
    std::vector<T> freqNormalized;          // normalized frequency
    std::vector<T> Q;                       // quality factor
    std::vector<T> gain;                    // linear gain for shelving/peak filters
    std::vector<BiquadType> type;           // filter type
    BiquadCore<T, ShaperType> BiquadCore;   // underlying biquad core processor 

    /**
     * @brief Update filter coefficients of single section.
     * @param section Section index
     */
    void updateCoeffs(size_t section)
    {
        // Initialize coefficients variables
        T b0 = T(0), b1 = T(0), b2 = T(0), a1 = T(0), a2 = T(0);
        switch (type[section])
        {
            case BiquadType::Lowpass:
                Jonssonic::computeLowpassCoeffs<T>(freqNormalized[section], Q[section],
                    b0, b1, b2, a1, a2);
                BiquadCore.setSectionCoeffs(section, b0, b1, b2, a1, a2);
                break;
            case BiquadType::Highpass:
                Jonssonic::computeHighpassCoeffs<T>(freqNormalized[section], Q[section],
                    b0, b1, b2, a1, a2);
                BiquadCore.setSectionCoeffs(section, b0, b1, b2, a1, a2);
                break;
            case BiquadType::Bandpass:
                Jonssonic::computeBandpassCoeffs<T>(freqNormalized[section], Q[section],
                    b0, b1, b2, a1, a2);
                BiquadCore.setSectionCoeffs(section, b0, b1, b2, a1, a2);
                break;
            case BiquadType::Allpass:
                Jonssonic::computeAllpassCoeffs<T>(freqNormalized[section], Q[section],
                    b0, b1, b2, a1, a2);
                BiquadCore.setSectionCoeffs(section, b0, b1, b2, a1, a2);
                break;
            case BiquadType::Notch:
                Jonssonic::computeNotchCoeffs<T>(freqNormalized[section], Q[section],
                    b0, b1, b2, a1, a2);
                BiquadCore.setSectionCoeffs(section, b0, b1, b2, a1, a2);
                break;
            case BiquadType::Peak:
                Jonssonic::computePeakCoeffs<T>(freqNormalized[section], Q[section], gain[section],
                    b0, b1, b2, a1, a2);
                BiquadCore.setSectionCoeffs(section, b0, b1, b2, a1, a2);
                break;
            case BiquadType::Lowshelf:
                Jonssonic::computeLowshelfCoeffs<T>(freqNormalized[section], Q[section], gain[section],
                    b0, b1, b2, a1, a2);
                BiquadCore.setSectionCoeffs(section, b0, b1, b2, a1, a2);
                break;
            case BiquadType::Highshelf:
                Jonssonic::computeHighshelfCoeffs<T>(freqNormalized[section], Q[section], gain[section],
                    b0, b1, b2, a1, a2);
                BiquadCore.setSectionCoeffs(section, b0, b1, b2, a1, a2);
                break;
            default:
                // Handle unsupported types or add implementations
                break;
        }
    }

    /**
     * @brief Update filter coefficients for all sections.
     */
    void updateCoeffs()
    {
        for (size_t s = 0; s < BiquadCore.getNumSections(); ++s)
        {
            updateCoeffs(s);
        }
    }
};
} // namespace Jonssonic