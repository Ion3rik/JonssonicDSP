// Jonssonic - A C++ audio DSP library
// Biqaud filter class header file
// SPDX-License-Identifier: MIT

#pragma once
#include "BiquadCore.h"
#include "FilterTypes.h"
#include "BiquadCoeffs.h"
#include "../nonlinear/WaveShaper.h"
#include "../../utils/MathUtils.h"
#include <algorithm>

namespace Jonssonic
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
    // Constructor and Destructor
    BiquadFilter() = default;
    BiquadFilter(size_t newNumChannels, T newSampleRate) // Parameterized constructor (for the faint hearted)
    {
        prepare(newNumChannels, newSampleRate);
    }
    ~BiquadFilter() = default;

    // No copy or move semantics
    BiquadFilter(const BiquadFilter&) = delete;
    BiquadFilter& operator=(const BiquadFilter&) = delete;
    BiquadFilter(BiquadFilter&&) = delete;
    BiquadFilter& operator=(BiquadFilter&&) = delete;

    /**
     * @brief Prepare the biquad filter for processing.
     * @param newNumChannels Number of channels
     */
    void prepare(size_t newNumChannels, T newSampleRate)
    {
        sampleRate = newSampleRate;
        // If not set, default to quarter Nyquist
        if (!freqNormalizedSet) {
            freqNormalized = T(0.25);
        }
        Q = T(0.707);             // default to Butterworth
        gain = T(1);              // unity gain
        type = BiquadType::Lowpass; // default to lowpass
        BiquadCore.prepare(newNumChannels, 1); // single second-order section
        updateCoeffs();
    }

    void clear()
    {
        BiquadCore.clear();
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

    void setGainDb(T newGainDb)
    {
        // clamp gain to avoid instability
        newGainDb = std::clamp(newGainDb, T(-60), T(20));
        gain = Jonssonic::dB2Mag(newGainDb); // convert to linear
        updateCoeffs();
    }

    void setGainLinear(T newGainLin)
    {   
        // clamp gain to avoid instability
        newGainLin = std::clamp(newGainLin, T(0.001), T(10));
        gain = newGainLin;
        updateCoeffs();
    }

    void setFreq(T newFreq)
    {
        // Convert Hz to normalized frequency (0..0.5)
        if (sampleRate > T(0)) {
            freqNormalized = std::clamp(newFreq / sampleRate, T(0), T(0.5));
        } else {
            freqNormalized = std::clamp(newFreq / T(44100), T(0), T(0.5));
        }
        freqNormalizedSet = true;
        updateCoeffs();
    }

    void setQ(T newQ)
    {
        // clamp Q to avoid instability
        Q = std::clamp(newQ, T(0.1), T(10));
        updateCoeffs();
    }

    void setType(BiquadType newType)
    {
        type = newType;
        updateCoeffs();
    }

private:
    T sampleRate = T(44100); // default sample rate
    T freqNormalized = T(0.25); // default to quarter Nyquist
    bool freqNormalizedSet = false;
    T Q;    // quality factor
    T gain; // linear gain for shelving/peak filters
    BiquadType type; // filter type
    BiquadCore<T, ShaperType> BiquadCore;

    /**
     * @brief Update filter coefficients based on current parameters.
     */
    void updateCoeffs()
    {   
        // Initialize coefficients variables
        T b0 = T(0), b1 = T(0), b2 = T(0), a1 = T(0), a2 = T(0);
        switch (type)
        {
            case BiquadType::Lowpass:
                Jonssonic::computeLowpassCoeffs<T>(freqNormalized, Q,
                    b0, b1, b2, a1, a2);
                BiquadCore.setSectionCoeffs(0, b0, b1, b2, a1, a2);
                break;
            case BiquadType::Highpass:
                Jonssonic::computeHighpassCoeffs<T>(freqNormalized, Q,
                    b0, b1, b2, a1, a2);
                BiquadCore.setSectionCoeffs(0, b0, b1, b2, a1, a2);
                break;
            case BiquadType::Bandpass:
                Jonssonic::computeBandpassCoeffs<T>(freqNormalized, Q,
                    b0, b1, b2, a1, a2);
                BiquadCore.setSectionCoeffs(0, b0, b1, b2, a1, a2);
                break;
            case BiquadType::Allpass:
                Jonssonic::computeAllpassCoeffs<T>(freqNormalized, Q,
                    b0, b1, b2, a1, a2);
                BiquadCore.setSectionCoeffs(0, b0, b1, b2, a1, a2);
                break;
            case BiquadType::Notch:
                Jonssonic::computeNotchCoeffs<T>(freqNormalized, Q,
                    b0, b1, b2, a1, a2);
                BiquadCore.setSectionCoeffs(0, b0, b1, b2, a1, a2);
                break;
            case BiquadType::Peak:
                Jonssonic::computePeakCoeffs<T>(freqNormalized, Q, gain,
                    b0, b1, b2, a1, a2);
                BiquadCore.setSectionCoeffs(0, b0, b1, b2, a1, a2);
                break;
            case BiquadType::Lowshelf:
                Jonssonic::computeLowshelfCoeffs<T>(freqNormalized, Q, gain,
                    b0, b1, b2, a1, a2);
                BiquadCore.setSectionCoeffs(0, b0, b1, b2, a1, a2);
                break;
            case BiquadType::Highshelf:
                Jonssonic::computeHighshelfCoeffs<T>(freqNormalized, Q, gain,
                    b0, b1, b2, a1, a2);
                BiquadCore.setSectionCoeffs(0, b0, b1, b2, a1, a2);
                break;
            default:
                // Handle unsupported types or add implementations
                break;
        }
    }
};
} // namespace Jonssonic