// Jonssonic - A Modular Realtime C++ Audio DSP Library
// GainSmoother class header file
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/utils/detail/config_utils.h"
#include <cmath>
#include <jonssonic/core/common/dsp_param.h>
#include <jonssonic/core/common/quantities.h>
#include <jonssonic/utils/math_utils.h>

namespace jonssonic::core::dynamics {

/**
 * @brief Gain smoother type enumeration.
 */

enum class GainSmootherType {
    AttackRelease
    // OnePole,
    // Linear
};

// =============================================================================
// Template Declaration
// =============================================================================
template <typename T, GainSmootherType Type = GainSmootherType::AttackRelease>
class GainSmoother;

// =============================================================================
// Attack-Release Gain Smoother Specialization
// =============================================================================
template <typename T>
class GainSmoother<T, GainSmootherType::AttackRelease> {
    /// Type aliases for convenience, readability and future-proofing
    using DspParamType = jonssonic::core::common::DspParam<T>;

  public:
    /// Default constructor.
    GainSmoother() = default;
    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param numChannels Number of channels
     * @param sampleRate Sample rate in Hz
     */
    GainSmoother(size_t newNumChannels, T newSampleRate) { prepare(newNumChannels, newSampleRate); }

    /// Default destructor.
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
    void prepare(size_t numNewChannels, T newSampleRate) {
        // Clamp and store number of channels and sample rate
        numChannels = utils::detail::clampChannels(numNewChannels);
        sampleRate = utils::detail::clampSampleRate(newSampleRate);

        // Prepare coefficient smoothers and gain state
        attackCoeff.prepare(numChannels, sampleRate);
        releaseCoeff.prepare(numChannels, sampleRate);
        gainDb.assign(numChannels, T(0.0));

        togglePrepared = true;
    }

    /**
     * @brief Reset the gain smoother state.
     * @param gainValueDb Initial gain value after reset
     */
    void reset(T gainValueDb = T(0.0)) { std::fill(gainDb.begin(), gainDb.end(), gainValueDb); }

    /**
     * @brief Process a single sample for a given channel.
     * @param ch Channel index
     * @param targetGainDb Target dB gain
     * @return Smoothed linear gain value
     * @note The smoothing is performed in dB domain for better perceptual results.
     */
    T processSample(size_t ch, T targetGainDb) {
        // What stage are we in?
        T inAttack = static_cast<T>(targetGainDb < gainDb[ch]);
        T inRelease = T(1) - inAttack;

        // Compute the smoothing coefficient
        T coeff =
            inAttack * attackCoeff.getNextValue(ch) + inRelease * releaseCoeff.getNextValue(ch);
        gainDb[ch] += coeff * (targetGainDb - gainDb[ch]);
        return utils::dB2Mag(gainDb[ch]); // convert smoothed dB gain to linear
    }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input (target gain in dB) sample pointers (one per channel)
     * @param output Output (smoothed gain in linear) sample pointers (one per channel)
     * @param numSamples Number of samples to process
     */

    void processBlock(const T* const* input, T* const* output, size_t numSamples) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t n = 0; n < numSamples; ++n) {
                output[ch][n] = processSample(ch, input[ch][n]);
            }
        }
    }

    /**
     * @brief Set parameter control smoothing time in various units.
     * @param time Smoothing time struct.
     * @note Not to be confused with attack/release times.
     */
    void setControlSmoothingTime(Time<T> time) {
        attackCoeff.setSmoothingTime(time);
        releaseCoeff.setSmoothingTime(time);
    }

    /**
     * @brief Set attack time in various units.
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
     * @brief Set release time in various units.
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

    /// Get the number of channels.
    size_t getNumChannels() const { return numChannels; }

    /// @brief  Get the sample rate.
    T getSampleRate() const { return sampleRate; }

  private:
    // Global parameters
    bool togglePrepared = false;
    size_t numChannels = 0;
    T sampleRate = T(44100);

    // State variables
    std::vector<T> gainDb;

    // User Parameters
    T attackTimeSec;
    T releaseTimeSec;

    // Coefficient smoothers
    DspParamType attackCoeff;
    DspParamType releaseCoeff;

    void updateCoefficients(bool skipSmoothing = false) {
        // Set target coefficients based on current attack and release times
        attackCoeff.setTarget(1.0 - std::exp(-1.0 / (attackTimeSec * sampleRate)), skipSmoothing);
        releaseCoeff.setTarget(1.0 - std::exp(-1.0 / (releaseTimeSec * sampleRate)), skipSmoothing);
    }
};

} // namespace jonssonic::core::dynamics