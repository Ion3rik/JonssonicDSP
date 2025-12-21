// Jonssonic - A C++ audio DSP library
// Delay line class header file
// SPDX-License-Identifier: MIT

#pragma once

#include <cmath>
#include <limits>
#include "../common/DspParam.h"
#include "../../utils/MathUtils.h"

namespace Jonssonic::core {

enum class GainType {
    Compressor
    //ExpanderDownward,
    //ExpanderUpward,
    //Limiter,
    //Gate
    // Add more types as needed
};

// =============================================================================
// Template Declaration
// =============================================================================
template<typename T, GainType Type>
class GainComputer;

// =============================================================================
// Compressor Gain Computer Specialization
// =============================================================================
template<typename T>
class GainComputer<T, GainType::Compressor>
{
public:
    // Constructors and Destructor
    GainComputer() = default;
    GainComputer(size_t newNumChannels, T newSampleRate)
    {
        prepare(newNumChannels, newSampleRate);
    }
    ~GainComputer() = default;

    // No copy or move semantics
    GainComputer(const GainComputer&) = delete;
    GainComputer& operator=(const GainComputer&) = delete;
    GainComputer(GainComputer&&) = delete;
    GainComputer& operator=(GainComputer&&) = delete;

    /**
     * @brief Prepare the gain computer for processing.
     * @param numChannels Number of channels
     * @param sampleRate Sample rate in Hz
     */
    void prepare(size_t numNewChannels, T newSampleRate, T smoothingTimeMs = T(20.0)) {
        numChannels = numNewChannels;
        sampleRate = newSampleRate;

        threshold.prepare(numChannels, sampleRate, smoothingTimeMs);
        ratio.prepare(numChannels, sampleRate, smoothingTimeMs);
        knee.prepare(numChannels, sampleRate, smoothingTimeMs);
    }

    /**
     * @brief Reset the gain computer state.
     */
    void reset()
    {
    }

    /**
     * @brief Process a single sample for a given channel.
     * @param ch Channel index
     * @param input Input signal sample
     * @return Computed gain
     */
    T processSample(size_t ch, T input)
    {
        // convert input to dB
        T inputDb = mag2dB(input);
        T thresholdVal = threshold.getNextValue(ch);
        T ratioVal = ratio.getNextValue(ch);
        T kneeVal = knee.getNextValue(ch);

        T over = inputDb - thresholdVal; // amount over threshold
        T halfKnee = kneeVal * T(0.5); // half knee width
        T kneePos = over + halfKnee; // position within knee
        T oneMinusInvRatio = (T(1.0) - T(1.0) / ratioVal); // 1 - 1/ratio

        // Soft knee branchless
        T inSoftKnee = static_cast<T>(kneeVal > 0); // mask for soft knee
        T below = static_cast<T>(over <= -halfKnee); // mask for below knee
        T above = static_cast<T>(over >= halfKnee); // mask for above knee
        T within = inSoftKnee * (T(1) - below - above); // mask for within knee

        constexpr T epsilon = std::numeric_limits<T>::epsilon();
        T isZeroKnee = static_cast<T>(kneeVal == T(0));
        T safeKnee = kneeVal + isZeroKnee * epsilon; // avoid division by zero
        T gainDb_soft =
            below * T(0) +
            above * ((thresholdVal - inputDb) * oneMinusInvRatio) +
            within * (-(oneMinusInvRatio) * (kneePos * kneePos) / (T(2.0) * safeKnee));

        // Hard knee branchless
        T aboveHard = static_cast<T>(inputDb > thresholdVal);
        T gainDb_hard = aboveHard * ((thresholdVal - inputDb) * oneMinusInvRatio);

        // Select soft or hard knee
        T gainDb = inSoftKnee * gainDb_soft + (T(1) - inSoftKnee) * gainDb_hard;
        return dB2Mag(gainDb);
    }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input sample pointers (one per channel)
     * @param output Output sample pointers (one per channel)
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
     * @brief Set threshold in dB.
     * @param newThreshold Threshold in dB
     * @param skipSmoothing If true, update coefficients immediately
     */
    void setThreshold(T newThresholdDb, bool skipSmoothing = false)
    {
        threshold.setTarget(newThresholdDb, skipSmoothing);
    }

    /**
     * @brief Set ratio.
     * @param newRatioDb Compression ratio
     * @param skipSmoothing If true, update coefficients immediately
     */
    void setRatio(T newRatio, bool skipSmoothing = false)
    {
        ratio.setTarget(newRatio, skipSmoothing);
    }

    /**
     * @brief Set knee width in dB.
     * @param newKneeDb Knee width in dB
     * @param skipSmoothing If true, update coefficients immediately
     */
    void setKnee(T newKneeDb, bool skipSmoothing = false)
    {
        knee.setTarget(newKneeDb, skipSmoothing); 
    }

    size_t getNumChannels() const { return numChannels; }
    T getSampleRate() const { return sampleRate; }

private:
    // Global parameters
    size_t numChannels = 0;
    T sampleRate = T(44100);

    // User Parameters
    DspParam<T> threshold; // threshold in dB
    DspParam<T> ratio;     // Compression ratio
    DspParam<T> knee;      // knee width in dB


};
}// namespace Jonssonic::core