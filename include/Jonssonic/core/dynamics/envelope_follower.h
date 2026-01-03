// Jonssonic - A Modular Realtime C++ Audio DSP Library
// Envelope Follower class template declaration and specializations
// SPDX-License-Identifier: MIT

#pragma once

#include "jonssonic/utils/detail/config_utils.h"
#include <cmath>
#include <jonssonic/core/common/dsp_param.h>
#include <jonssonic/core/common/quantities.h>
#include <vector>

namespace jonssonic::core::dynamics {

enum class EnvelopeType {
    Peak,
    RMS
    // Add more types as needed
};
// =============================================================================
// Template Declaration
// =============================================================================
template <typename T, EnvelopeType Type> class EnvelopeFollower;

// =============================================================================
// Peak Envelope Follower Specialization
// =============================================================================
/// Peak Envelope Follower Specialization
template <typename T> class EnvelopeFollower<T, EnvelopeType::Peak> {
    /// Type aliases for convenience, readability and future-proofing
    using DspParamType = jonssonic::core::common::DspParam<T>;

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
    EnvelopeFollower(const EnvelopeFollower &) = delete;
    EnvelopeFollower &operator=(const EnvelopeFollower &) = delete;
    EnvelopeFollower(EnvelopeFollower &&) = delete;
    EnvelopeFollower &operator=(EnvelopeFollower &&) = delete;

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
        T rectified = std::abs(input); // abs for peak follower
        T &env = envelope[ch];         // reference to current envelope value
        T coeff = (rectified > env)
                      ? attackCoeff.getNextValue(ch)
                      : releaseCoeff.getNextValue(ch); // choose coefficient based on attack/release
        env += coeff * (rectified - env);              // exponential smoothing
        return env;
    }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input (signal) sample pointers (one per channel)
     * @param output Output (envelope) sample pointers (one per channel)
     * @param numSamples Number of samples to process
     */
    void processBlock(const T *const *input, T *const *output, size_t numSamples) {
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
    DspParamType attackCoeff;
    DspParamType releaseCoeff;

    void updateCoefficients(bool skipSmoothing) {

        // Set target coefficients based on current attack and release times
        attackCoeff.setTarget(1.0 - std::exp(-1.0 / (attackTimeSec * sampleRate)), skipSmoothing);
        releaseCoeff.setTarget(1.0 - std::exp(-1.0 / (releaseTimeSec * sampleRate)), skipSmoothing);
    }
};

// =============================================================================
// RMS Envelope Follower Specialization
// =============================================================================
template <typename T> class EnvelopeFollower<T, EnvelopeType::RMS> {
    /// Type aliases for convenience, readability and future-proofing
    using DspParamType = jonssonic::core::common::DspParam<T>;

  public:
    // Constructors and Destructor
    EnvelopeFollower() = default;
    EnvelopeFollower(size_t newNumChannels, T newSampleRate) {
        prepare(newNumChannels, newSampleRate);
    }
    ~EnvelopeFollower() = default;

    // No copy or move semantics
    EnvelopeFollower(const EnvelopeFollower &) = delete;
    EnvelopeFollower &operator=(const EnvelopeFollower &) = delete;
    EnvelopeFollower(EnvelopeFollower &&) = delete;
    EnvelopeFollower &operator=(EnvelopeFollower &&) = delete;

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
        T squared = input * input; // square for RMS follower
        T &env = envelope[ch];     // reference to current envelope value
        T coeff = (squared > env)
                      ? attackCoeff.getNextValue(ch)
                      : releaseCoeff.getNextValue(ch); // choose coefficient based on attack/release
        env += coeff * (squared - env);                // exponential smoothing
        return std::sqrt(env);                         // return RMS value
    }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input sample pointers (one per channel)
     * @param output Output sample pointers (one per channel)
     * @param numSamples Number of samples to process
     */
    void processBlock(const T *const *input, T *const *output, size_t numSamples) {
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
        attackCoeff.setSmoothingTimeMs(time);
        releaseCoeff.setSmoothingTimeMs(time);
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
    DspParamType attackCoeff;
    DspParamType releaseCoeff;

    void updateCoefficients(bool skipSmoothing) {
        // Set target coefficients based on current attack and release times
        attackCoeff.setTarget(1.0 - std::exp(-1.0 / (attackTimeSec * sampleRate)), skipSmoothing);
        releaseCoeff.setTarget(1.0 - std::exp(-1.0 / (releaseTimeSec * sampleRate)), skipSmoothing);
    }
};

} // namespace jonssonic::core::dynamics