// Jonssonic - A C++ audio DSP library
// GainSmoother class header file
// SPDX-License-Identifier: MIT

#pragma once
#include <cmath>
#include "../common/DspParam.h"
#include "../../utils/MathUtils.h"

namespace Jonssonic::core {

/**
 * @brief Gain smoother type enumeration.
 */

enum class GainSmootherType {
    AttackRelease
    // OnePole,
    //Linear
};

// =============================================================================
// Template Declaration
// =============================================================================
template<typename T, GainSmootherType Type = GainSmootherType::AttackRelease>
class GainSmoother;

// =============================================================================
// Attack-Release Gain Smoother Specialization
// =============================================================================
template<typename T>
class GainSmoother<T, GainSmootherType::AttackRelease>
{
public:
    // Constructors and Destructor
    GainSmoother() = default;
    GainSmoother(size_t newNumChannels, T newSampleRate, T paramSmoothingTimeMs = T(20.0))
    {
        prepare(newNumChannels, newSampleRate, paramSmoothingTimeMs);
    }
    ~GainSmoother() = default;

    // No copy or move semantics
    GainSmoother(const GainSmoother&) = delete;
    GainSmoother& operator=(const GainSmoother&) = delete;
    GainSmoother(GainSmoother&&) = delete;
    GainSmoother& operator=(GainSmoother&&) = delete;

    /**
     * @brief Prepare the gain smoother for processing.
     * @param numChannels Number of channels
     * @param sampleRate Sample rate in Hz
     */
    void prepare(size_t numNewChannels, T newSampleRate, T paramSmoothingTimeMs = T(20.0)) {
        numChannels = numNewChannels;
        sampleRate = newSampleRate;
        attackCoeff.prepare(numChannels, sampleRate, paramSmoothingTimeMs);
        releaseCoeff.prepare(numChannels, sampleRate, paramSmoothingTimeMs);

        gainDb.assign(numChannels, T(0.0)); // initialize gain to unity

    }

    /**
     * @brief Reset the gain smoother state.
     * @param gainValueDb Initial gain value after reset
     */
    void reset(T gainValueDb = T(0.0)) {
        std::fill(gainDb.begin(), gainDb.end(), gainValueDb);
    }

    /**
     * @brief Process a single sample for a given channel.
     * @param ch Channel index
     * @param targetGain Target linear gain
     * @return Smoothed gain in dB 
     */

    T processSample(size_t ch, T targetGainDb) {
        T attackPhaseMask = static_cast<T>(targetGainDb < gainDb[ch]); // mask for attack phase
        T releasePhaseMask = T(1) - attackPhaseMask; // mask for release phase
        T coeff = attackPhaseMask * attackCoeff.getNextValue(ch) + releasePhaseMask * releaseCoeff.getNextValue(ch); // select coefficient
        gainDb[ch] += coeff * (targetGainDb - gainDb[ch]); // exponential smoothing
        return dB2Mag(gainDb[ch]); // convert smoothed dB gain to linear
    }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input (target gain in dB) sample pointers (one per channel)
     * @param output Output (smoothed gain in linear) sample pointers (one per channel)
     * @param numSamples Number of samples to process
     */

    void processBlock(const T* const* input, T* const* output, size_t numSamples)
    {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t n = 0; n < numSamples; ++n) {
                output[ch][n] = processSample(ch, input[ch][n]);
            }
        }
    }

    /**
     * @brief Set attack time in milliseconds.
     */
    void setAttackTime(T newAttackTimeMs, bool skipSmoothing = false)
    {
        attackTime = std::max(newAttackTimeMs, T(1.0)); // prevent super fast attack times
        updateCoefficients(skipSmoothing);
    }

    /**
     * @brief Set release time in milliseconds.
     */
    void setReleaseTime(T newReleaseTimeMs, bool skipSmoothing = false)
    {
        releaseTime = std::max(newReleaseTimeMs, T(5.0)); // prevent super fast release times
        updateCoefficients(skipSmoothing);
    }

    size_t getNumChannels() const { return numChannels; }
    T getSampleRate() const { return sampleRate; }

private:
    // Global parameters
    size_t numChannels = 0;
    T sampleRate = T(44100);

    // State variables
    std::vector<T> gainDb;

    // User Parameters
    T attackTime;
    T releaseTime;

    // Coefficient smoothers
    DspParam<T> attackCoeff;
    DspParam<T> releaseCoeff;


    void updateCoefficients(bool skipSmoothing = false)
    {
        // Set target coefficients based on current attack and release times
        attackCoeff.setTarget(1.0 - std::exp(-1.0 / (0.001 * attackTime * sampleRate)), skipSmoothing);
        releaseCoeff.setTarget(1.0 - std::exp(-1.0 / (0.001 * releaseTime * sampleRate)), skipSmoothing);
    }
};

} // namespace Jonssonic::core