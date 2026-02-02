// Jonssonic - A Modular Realtime C++ Audio DSP Library
// Envelope Follower class template declaration and specializations
// SPDX-License-Identifier: MIT

#pragma once

#include "jonssonic/utils/detail/config_utils.h"
#include <cassert>
#include <cmath>
#include <jonssonic/core/common/dsp_param.h>
#include <jonssonic/core/common/quantities.h>
#include <vector>

namespace jnsc {

/// Envelope follower type enumeration
enum class EnvelopeType {
    Peak,
    RMS
    // Add more types as needed
};

// =============================================================================
// Template Declaration
// =============================================================================
/// Envelope Follower class template
template <typename T, EnvelopeType Type>
class EnvelopeFollower;

// =============================================================================
// Peak Envelope Follower Specialization
// =============================================================================
/// Peak Envelope Follower Specialization
template <typename T>
class EnvelopeFollower<T, EnvelopeType::Peak> {
    /// Envelope parameter struct
    struct EnvelopeParams {
        T attackTimeSec;
        T releaseTimeSec;
    };

  public:
    /// Default constructor.
    EnvelopeFollower() = default;
    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     */
    EnvelopeFollower(size_t newNumChannels, T newSampleRate) {
        prepare(newNumChannels, newSampleRate);
    }
    /// Default destructor.
    ~EnvelopeFollower() = default;

    /// No copy nor move semantics
    EnvelopeFollower(const EnvelopeFollower&) = delete;
    EnvelopeFollower& operator=(const EnvelopeFollower&) = delete;
    EnvelopeFollower(EnvelopeFollower&&) = delete;
    EnvelopeFollower& operator=(EnvelopeFollower&&) = delete;

    /**
     * @brief Prepare the envelope follower for processing.
     * @param numChannels Number of channels
     * @param sampleRate Sample rate in Hz
     */
    void prepare(size_t numNewChannels, T newSampleRate) {
        numChannels = utils::detail::clampChannels(numNewChannels);
        sampleRate = utils::detail::clampSampleRate(newSampleRate);
        attackCoeff.prepare(numChannels, sampleRate);
        releaseCoeff.prepare(numChannels, sampleRate);
        envelope.assign(numChannels, T(0));
        togglePrepared = true;
    }

    /**
     * @brief Reset the envelope follower state.
     * @param value Initial envelope value after reset
     */
    void reset(T value = T(0)) { std::fill(envelope.begin(), envelope.end(), value); }

    /**
     * @brief Process a single sample for a given channel.
     * @param ch Channel index
     * @param input Input sample
     * @return Envelope value
     */
    T processSample(size_t ch, T input) {
        // Rectify input (i.e. find peak level)
        T rectified = std::abs(input);

        // What stage are we in?
        T inAttack = static_cast<T>(rectified > envelope[ch]);
        T inRelease = T(1) - inAttack;

        // Compute the smoothing coefficient
        T coeff =
            inAttack * attackCoeff.getNextValue(ch) + inRelease * releaseCoeff.getNextValue(ch);

        // Apply smoothing to the rectified value
        envelope[ch] += coeff * (rectified - envelope[ch]);

        // Return the envelope value
        return envelope[ch];
    }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input (signal) sample pointers (one per channel)
     * @param output Output (envelope) sample pointers (one per channel)
     * @param numSamples Number of samples to process
     */
    void processBlock(const T* const* input, T* const* output, size_t numSamples) {
        for (size_t ch = 0; ch < numChannels; ++ch)
            for (size_t n = 0; n < numSamples; ++n) {
                {
                    output[ch][n] = processSample(ch, input[ch][n]);
                }
            }
    }
    /**
     * @brief Set control smoothing time in various units.
     * @param time Smoothing time struct.
     * @note Not to be confused with attack/release times.
     */
    void setControlSmoothingTime(Time<T> time) {
        attackCoeff.setSmoothingTime(time);
        releaseCoeff.setSmoothingTime(time);
        updateCoefficients(true);
    }
    /**
     * @brief Set the attack time in various units.
     * @param time Attack time struct.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setAttackTime(Time<T> time, bool skipSmoothing = false) {
        // Early exit if not prepared
        if (!togglePrepared)
            return;

        params.attackTimeSec = time.toSeconds(sampleRate);
        updateCoefficients(skipSmoothing);
    }
    /**
     * @brief Set the release time in various units.
     * @param time Release time struct.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setReleaseTime(Time<T> time, bool skipSmoothing = false) {
        // Early exit if not prepared
        if (!togglePrepared)
            return;

        params.releaseTimeSec = time.toSeconds(sampleRate);
        updateCoefficients(skipSmoothing);
    }

    /// Get number of channels.
    size_t getNumChannels() const { return numChannels; }

    /// Get sample rate.
    T getSampleRate() const { return sampleRate; }

    /// Check if prepared.
    bool isPrepared() const { return togglePrepared; }

    /// Get the envelope state
    std::vector<T> getState() const { return envelope; }

    /// Set the envelope state
    void setState(const std::vector<T>& newState) {
        assert(newState.size() == numChannels && "State size must match number of channels");
        envelope = newState;
    }

    /// Get parameters
    EnvelopeParams getParams() const { return params; }

    /// Set parameters
    void setParams(const EnvelopeParams& newParams, bool skipSmoothing = false) {
        params = newParams;
        updateCoefficients(skipSmoothing);
    }

  private:
    // Global parameters
    bool togglePrepared = false;
    size_t numChannels = 0;
    T sampleRate = T(44100);

    // State variables
    std::vector<T> envelope;

    // User Parameters
    EnvelopeParams params;

    // DSP Parameters
    DspParam<T> attackCoeff;
    DspParam<T> releaseCoeff;

    void updateCoefficients(bool skipSmoothing) {

        // Set target coefficients based on current attack and release times
        attackCoeff.setTarget(1.0 - std::exp(-1.0 / (params.attackTimeSec * sampleRate)),
                              skipSmoothing);
        releaseCoeff.setTarget(1.0 - std::exp(-1.0 / (params.releaseTimeSec * sampleRate)),
                               skipSmoothing);
    }
};

// =============================================================================
// RMS Envelope Follower Specialization
// =============================================================================
template <typename T>
class EnvelopeFollower<T, EnvelopeType::RMS> {
  public:
    // Constructors and Destructor
    EnvelopeFollower() = default;
    EnvelopeFollower(size_t newNumChannels, T newSampleRate) {
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
    void prepare(size_t numNewChannels, T newSampleRate) {
        numChannels = utils::detail::clampChannels(numNewChannels);
        sampleRate = utils::detail::clampSampleRate(newSampleRate);
        attackCoeff.prepare(numChannels, sampleRate);
        releaseCoeff.prepare(numChannels, sampleRate);
        envelope.assign(numChannels, T(0));
        togglePrepared = true;
    }

    /**
     * @brief Reset the envelope follower state.
     * @param value Initial envelope value after reset
     */
    void reset(T value = T(0)) { std::fill(envelope.begin(), envelope.end(), value); }

    /**
     * @brief Process a single sample for a given channel.
     * @param ch Channel index
     * @param input Input sample
     * @return Envelope value
     */
    T processSample(size_t ch, T input) {
        // Square the input for RMS calculation
        T squared = input * input;

        // What stage are we in?
        T inAttack = static_cast<T>(squared > envelope[ch]);
        T inRelease = T(1) - inAttack;

        // Compute the smoothing coefficient
        T coeff =
            inAttack * attackCoeff.getNextValue(ch) + inRelease * releaseCoeff.getNextValue(ch);

        // Apply smoothing to the squared value
        envelope[ch] += coeff * (squared - envelope[ch]);

        // Return the square root of the envelope for RMS
        return std::sqrt(envelope[ch]);
    }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input sample pointers (one per channel)
     * @param output Output sample pointers (one per channel)
     * @param numSamples Number of samples to process
     */
    void processBlock(const T* const* input, T* const* output, size_t numSamples) {
        for (size_t ch = 0; ch < numChannels; ++ch)
            for (size_t n = 0; n < numSamples; ++n) {
                {
                    output[ch][n] = processSample(ch, input[ch][n]);
                }
            }
    }
    /**
     * @brief Set control smoothing time in various units.
     * @param time Smoothing time struct.
     * @note Not to be confused with attack/release times.
     */
    void setControlSmoothingTime(Time<T> time) {
        attackCoeff.setSmoothingTime(time);
        releaseCoeff.setSmoothingTime(time);
    }

    /**
     * @brief Set the attack time in various units.
     * @param time Attack time struct.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setAttackTime(Time<T> time, bool skipSmoothing = false) {
        // Early exit if not prepared
        if (!togglePrepared)
            return;

        attackTimeSec = time.toSeconds(sampleRate);
        updateCoefficients(skipSmoothing);
    }
    /**
     * @brief Set the release time in various units.
     * @param time Release time struct.
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setReleaseTime(Time<T> time, bool skipSmoothing = false) {
        // Early exit if not prepared
        if (!togglePrepared)
            return;

        releaseTimeSec = time.toSeconds(sampleRate);
        updateCoefficients(skipSmoothing);
    }

    size_t getNumChannels() const { return numChannels; }
    T getSampleRate() const { return sampleRate; }

  private:
    // Global parameters
    bool togglePrepared = false;
    size_t numChannels = 0;
    T sampleRate = T(44100);

    // State variables
    std::vector<T> envelope;

    // User Parameters
    T attackTimeSec;
    T releaseTimeSec;
    DspParam<T> attackCoeff;
    DspParam<T> releaseCoeff;

    void updateCoefficients(bool skipSmoothing) {
        // Set target coefficients based on current attack and release times
        attackCoeff.setTarget(1.0 - std::exp(-1.0 / (attackTimeSec * sampleRate)), skipSmoothing);
        releaseCoeff.setTarget(1.0 - std::exp(-1.0 / (releaseTimeSec * sampleRate)), skipSmoothing);
    }
};

} // namespace jnsc