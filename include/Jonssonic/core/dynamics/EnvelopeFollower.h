// Jonssonic - A C++ audio DSP library
// Delay line class header file
// SPDX-License-Identifier: MIT

#pragma once

#include <cmath>
#include "../common/DspParam.h"

namespace Jonssonic::core {

enum class EnvelopeType {
    Peak,
    RMS
    // Add more types as needed
};
// =============================================================================
// Template Declaration
// =============================================================================
template<typename T, EnvelopeType Type>
class EnvelopeFollower;

// =============================================================================
// Peak Envelope Follower Specialization
// =============================================================================
template<typename T>
class EnvelopeFollower<T, EnvelopeType::Peak>
{
public:
    // Constructors and Destructor
    EnvelopeFollower() = default;
    EnvelopeFollower(size_t newNumChannels, T newSampleRate)
    {
        prepare(newNumChannels, newSampleRate);
    }
    ~EnvelopeFollower() = default;

    // No copy or move semantics
    EnvelopeFollower(const EnvelopeFollower&) = delete;
    EnvelopeFollower& operator=(const EnvelopeFollower&) = delete;
    EnvelopeFollower(EnvelopeFollower&&) = delete;
    EnvelopeFollower& operator=(EnvelopeFollower&&) = delete;

    /**
     * @brief Prepare the envelope follower for processing.
     * @param numChannels Number of channels
     * @param sampleRate Sample rate in Hz
     */
    void prepare(size_t numNewChannels, T newSampleRate, T smoothingTimeMs = T(20.0)) {
        numChannels = numNewChannels;
        sampleRate = newSampleRate;
        attackCoeff.prepare(numChannels, sampleRate, smoothingTimeMs);
        releaseCoeff.prepare(numChannels, sampleRate, smoothingTimeMs);
        envelope.assign(numChannels, T(0));

        // default values
        attackTime = T(10.0); // default attack time in ms
        releaseTime = T(100.0); // default release time in ms
        updateCoefficients(true);
    }

    /**
     * @brief Reset the envelope follower state.
     * @param value Initial envelope value after reset
     */
    void reset(T value = T(0))
    {
        std::fill(envelope.begin(), envelope.end(), value);
    }

    /**
     * @brief Process a single sample for a given channel.
     * @param ch Channel index
     * @param input Input sample
     * @return Envelope value
     */
    T processSample(size_t ch, T input)
    {
        T rectified = std::abs(input); // abs for peak follower
        T& env = envelope[ch]; // reference to current envelope value
        T coeff = (rectified > env) ? attackCoeff.getNextValue(ch) : releaseCoeff.getNextValue(ch); // choose coefficient based on attack/release
        env += coeff * (rectified - env); // exponential smoothing
        return env;
    }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input (signal) sample pointers (one per channel)
     * @param output Output (envelope) sample pointers (one per channel)
     * @param numSamples Number of samples to process
     */
    void processBlock(const T* const* input, T* const* output, size_t numSamples)
    {
        for (size_t ch = 0; ch < numChannels; ++ch) 
            for (size_t n = 0; n < numSamples; ++n) {
            {
                output[ch][n] = processSample(ch, input[ch][n]);
            }
        }
    }

    /**
     * @brief Set the attack time in milliseconds.
     * @param ms Attack time in milliseconds
     */
    void setAttackTime(T attackTimeMs, bool skipSmoothing = false)
    {
        attackTime = std::max(attackTimeMs, T(0.1)); // prevent super fast attack times
        updateCoefficients(skipSmoothing);
    }
    /**
     * @brief Set the release time in milliseconds.
     * @param ms Release time in milliseconds
     */
    void setReleaseTime(T releaseTimeMs, bool skipSmoothing = false)
    {
        releaseTime = std::max(releaseTimeMs, T(5.0)); // prevent super fast release times
        updateCoefficients(skipSmoothing);
    }

    size_t getNumChannels() const { return numChannels; }
    T getSampleRate() const { return sampleRate; }

private:

    // Global parameters
    size_t numChannels = 0;
    T sampleRate = T(44100);

    // State variables
    std::vector<T> envelope;

    // User Parameters
    T attackTime;
    T releaseTime;
    DspParam<T> attackCoeff;
    DspParam<T> releaseCoeff;


    void updateCoefficients(bool skipSmoothing = false)
    {
        // Set target coefficients based on current attack and release times
        attackCoeff.setTarget(1.0 - std::exp(-1.0 / (0.001 * attackTime * sampleRate)), skipSmoothing);
        releaseCoeff.setTarget(1.0 - std::exp(-1.0 / (0.001 * releaseTime * sampleRate)), skipSmoothing);
    }
};

// =============================================================================
// RMS Envelope Follower Specialization
// =============================================================================
template<typename T>
class EnvelopeFollower<T, EnvelopeType::RMS>
{
public:
    // Constructors and Destructor
    EnvelopeFollower() = default;
    EnvelopeFollower(size_t newNumChannels, T newSampleRate)
    {
        prepare(newNumChannels, newSampleRate);
    }
    ~EnvelopeFollower() = default;

    // No copy or move semantics
    EnvelopeFollower(const EnvelopeFollower&) = delete;
    EnvelopeFollower& operator=(const EnvelopeFollower&) = delete;
    EnvelopeFollower(EnvelopeFollower&&) = delete;
    EnvelopeFollower& operator=(EnvelopeFollower&&) = delete;

    /**
     * @brief Prepare the envelope follower for processing.
     * @param numChannels Number of channels
     * @param sampleRate Sample rate in Hz
     */
    void prepare(size_t numNewChannels, T newSampleRate, T smoothingTimeMs = T(20.0)) {
        numChannels = numNewChannels;
        sampleRate = newSampleRate;
        attackCoeff.prepare(numChannels, sampleRate, smoothingTimeMs);
        releaseCoeff.prepare(numChannels, sampleRate, smoothingTimeMs);
        envelope.assign(numChannels, T(0));

        // default values
        attackTime = T(10.0); // default attack time in ms
        releaseTime = T(100.0); // default release time in ms
        updateCoefficients(true);
    }

    /**
     * @brief Reset the envelope follower state.
     * @param value Initial envelope value after reset
     */
    void reset(T value = T(0))
    {
        std::fill(envelope.begin(), envelope.end(), value);
    }

    /**
     * @brief Process a single sample for a given channel.
     * @param ch Channel index
     * @param input Input sample
     * @return Envelope value
     */
    T processSample(size_t ch, T input)
    {
        T squared = input * input; // square for RMS follower
        T& env = envelope[ch]; // reference to current envelope value
        T coeff = (squared > env) ? attackCoeff.getNextValue(ch) : releaseCoeff.getNextValue(ch); // choose coefficient based on attack/release
        env += coeff * (squared - env); // exponential smoothing
        return std::sqrt(env); // return RMS value
    }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input sample pointers (one per channel)
     * @param output Output sample pointers (one per channel)
     * @param numSamples Number of samples to process
     */
    void processBlock(const T* const* input, T* const* output, size_t numSamples)
    {
        for (size_t ch = 0; ch < numChannels; ++ch) 
            for (size_t n = 0; n < numSamples; ++n) {
            {
                output[ch][n] = processSample(ch, input[ch][n]);
            }
        }
    }

    /**
     * @brief Set the attack time in milliseconds.
     * @param ms Attack time in milliseconds
     */
    void setAttackTime(T attackTimeMs, bool skipSmoothing = false)
    {
        attackTime = attackTimeMs;
        updateCoefficients(skipSmoothing);
    }
    /**
     * @brief Set the release time in milliseconds.
     * @param ms Release time in milliseconds
     */
    void setReleaseTime(T releaseTimeMs, bool skipSmoothing = false)
    {
        releaseTime = releaseTimeMs;
        updateCoefficients(skipSmoothing);
    }

    size_t getNumChannels() const { return numChannels; }
    T getSampleRate() const { return sampleRate; }

private:

    // Global parameters
    size_t numChannels = 0;
    T sampleRate = T(44100);

    // State variables
    std::vector<T> envelope;

    // User Parameters
    T attackTime;
    T releaseTime;
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