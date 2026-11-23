// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Program-dependent compressor model
// SPDX-License-Identifier: MIT

#pragma once

#include "jonssonic/utils/detail/config_utils.h"
#include <algorithm>
#include <jonssonic/core/common/quantities.h>
#include <jonssonic/core/dynamics/envelope_follower.h>
#include <jonssonic/core/dynamics/gain_computer.h>
#include <jonssonic/core/dynamics/gain_smoother.h>
#include <vector>

namespace jnsc::models {
/**
 * @brief Feedforward compressor whose attack, release, ratio, and knee can
 *        depend on its current gain reduction.
 * @tparam T Sample type
 * @tparam EnvelopeType Type of envelope follower (default: RMS)
 */
template <typename T, EnvelopeType EnvelopeType = EnvelopeType::RMS>
class ProgramDependentCompressor {
    static constexpr T CONTROL_SMOOTH_TIME_MS = T(50);
    static constexpr T GAIN_SMOOTH_ATTACK_MS = T(0.1);
    static constexpr T GAIN_SMOOTH_RELEASE_MS = T(5);

  public:
    /// Default constructor.
    ProgramDependentCompressor() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     */
    ProgramDependentCompressor(size_t newNumChannels, T newSampleRate) { prepare(newNumChannels, newSampleRate); }

    /// No copy nor move semantics.
    ProgramDependentCompressor(const ProgramDependentCompressor&) = delete;
    ProgramDependentCompressor& operator=(const ProgramDependentCompressor&) = delete;
    ProgramDependentCompressor(ProgramDependentCompressor&&) = delete;
    ProgramDependentCompressor& operator=(ProgramDependentCompressor&&) = delete;

    /**
     * @brief Prepare the compressor for processing.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     */
    void prepare(size_t newNumChannels, T newSampleRate) {
        numChannels = utils::detail::clampChannels(newNumChannels);
        sampleRate = utils::detail::clampSampleRate(newSampleRate);

        envelopeFollower.prepare(numChannels, sampleRate);
        gainComputer.prepare(numChannels, sampleRate);
        gainSmoother.prepare(numChannels, sampleRate);
        setControlSmoothingTime(Time<T>::Milliseconds(CONTROL_SMOOTH_TIME_MS));
        gainSmoother.setAttackTime(Time<T>::Milliseconds(GAIN_SMOOTH_ATTACK_MS), true);
        gainSmoother.setReleaseTime(Time<T>::Milliseconds(GAIN_SMOOTH_RELEASE_MS), true);

        detectorEnvelope.assign(numChannels, T(0));
        gainReductionDb.assign(numChannels, T(0));
        maxGainReduction.assign(numChannels, T(1));
        lastAttackMs.assign(numChannels, T(-1));
        lastReleaseMs.assign(numChannels, T(-1));
        lastRatio.assign(numChannels, T(-1));
        lastKneeDb.assign(numChannels, T(-1));

        gainComputer.setThreshold(thresholdDb, true);
        gainComputer.setRatio(ratioMin, true);
        gainComputer.setKnee(kneeMinDb, true);
        envelopeFollower.setAttackTime(Time<T>::Milliseconds(attackMinMs), true);
        envelopeFollower.setReleaseTime(Time<T>::Milliseconds(releaseMinMs), true);
        togglePrepared = true;
    }

    /// Reset detector, gain, and metering state.
    void reset() {
        envelopeFollower.reset();
        gainSmoother.reset();
        std::fill(detectorEnvelope.begin(), detectorEnvelope.end(), T(0));
        std::fill(gainReductionDb.begin(), gainReductionDb.end(), T(0));
        std::fill(maxGainReduction.begin(), maxGainReduction.end(), T(1));
    }

    /**
     * @brief Process one feedforward sample.
     * @param ch Channel index
     * @param input Audio input sample
     * @param detectorInput Detector input sample
     * @return Compressed sample
     */
    T processSample(size_t ch, T input, T detectorInput) {
        applyProgramDependentControls(ch, gainReductionDb[ch]);

        detectorEnvelope[ch] = envelopeFollower.processSample(ch, detectorInput);
        gainReductionDb[ch] = gainComputer.processSample(ch, detectorEnvelope[ch]);
        const T gain = gainSmoother.processSample(ch, gainReductionDb[ch]);
        maxGainReduction[ch] = std::min(maxGainReduction[ch], gain);
        return input * gain;
    }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Audio input pointers
     * @param detectorInput Detector input pointers
     * @param output Output pointers
     * @param numSamples Samples per channel
     * @param gainReductionOutput Optional per-channel minimum linear gain
     */
    void processBlock(const T* const* input,
                      const T* const* detectorInput,
                      T* const* output,
                      size_t numSamples,
                      T* gainReductionOutput = nullptr) {
        std::fill(maxGainReduction.begin(), maxGainReduction.end(), T(1));
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t n = 0; n < numSamples; ++n)
                output[ch][n] = processSample(ch, input[ch][n], detectorInput[ch][n]);
            if (gainReductionOutput)
                gainReductionOutput[ch] = maxGainReduction[ch];
        }
    }

    /// Set threshold in dB.
    void setThreshold(T newThresholdDb, bool skipSmoothing = false) {
        thresholdDb = newThresholdDb;
        if (togglePrepared)
            gainComputer.setThreshold(thresholdDb, skipSmoothing);
    }

    /// Set fixed attack time in milliseconds.
    void setAttackTime(T timeMs) { setAttackTimeRange(timeMs, timeMs); }

    /// Set program-dependent attack time limits in milliseconds.
    void setAttackTimeRange(T minimumMs, T maximumMs) {
        attackMinMs = std::max(std::min(minimumMs, maximumMs), T(0.1));
        attackMaxMs = std::max(std::max(minimumMs, maximumMs), T(0.1));
    }

    /// Set fixed release time in milliseconds.
    void setReleaseTime(T timeMs) { setReleaseTimeRange(timeMs, timeMs); }

    /// Set program-dependent release time limits in milliseconds.
    void setReleaseTimeRange(T minimumMs, T maximumMs) {
        releaseMinMs = std::max(std::min(minimumMs, maximumMs), T(1));
        releaseMaxMs = std::max(std::max(minimumMs, maximumMs), T(1));
    }

    /// Set fixed compression ratio.
    void setRatio(T newRatio, bool skipSmoothing = false) { setRatioRange(newRatio, newRatio, skipSmoothing); }

    /// Set program-dependent compression ratio limits.
    void setRatioRange(T minimumRatio, T maximumRatio, bool skipSmoothing = false) {
        ratioMin = std::max(std::min(minimumRatio, maximumRatio), T(1));
        ratioMax = std::max(std::max(minimumRatio, maximumRatio), T(1));
        if (togglePrepared)
            gainComputer.setRatio(ratioMin, skipSmoothing);
    }

    /// Set fixed knee width in dB.
    void setKnee(T newKneeDb, bool skipSmoothing = false) { setKneeRange(newKneeDb, newKneeDb, skipSmoothing); }

    /// Set program-dependent knee width limits in dB.
    void setKneeRange(T minimumDb, T maximumDb, bool skipSmoothing = false) {
        kneeMinDb = std::max(std::min(minimumDb, maximumDb), T(0));
        kneeMaxDb = std::max(std::max(minimumDb, maximumDb), T(0));
        if (togglePrepared)
            gainComputer.setKnee(kneeMinDb, skipSmoothing);
    }

    /**
     * @brief Set the gain-reduction range used to morph between parameter limits.
     * @param minimumDb Gain reduction in dB where minimum settings apply (positive magnitude)
     * @param maximumDb Gain reduction in dB where maximum settings apply (positive magnitude)
     */
    void setModulationRange(T minimumDb, T maximumDb) {
        modulationMinDb = std::max(std::min(minimumDb, maximumDb), T(0));
        modulationMaxDb = std::max(std::max(minimumDb, maximumDb), modulationMinDb + T(0.001));
    }

    /// Set smoothing time for envelope and compressor controls.
    void setControlSmoothingTime(Time<T> time) {
        envelopeFollower.setControlSmoothingTime(time);
        gainComputer.setControlSmoothingTime(time);
        gainSmoother.setControlSmoothingTime(time);
    }

    /// Set gain smoother attack time.
    void setGainSmootherAttackTime(Time<T> time, bool skipSmoothing = false) {
        gainSmoother.setAttackTime(time, skipSmoothing);
    }

    /// Set gain smoother release time.
    void setGainSmootherReleaseTime(Time<T> time, bool skipSmoothing = false) {
        gainSmoother.setReleaseTime(time, skipSmoothing);
    }

    size_t getNumChannels() const { return numChannels; }
    T getSampleRate() const { return sampleRate; }
    bool isPrepared() const { return togglePrepared; }

  private:
    // Config variables
    size_t numChannels = 0;
    T sampleRate = T(44100);
    bool togglePrepared = false;
    T thresholdDb = T(-24);
    T attackMinMs = T(10);
    T attackMaxMs = T(10);
    T releaseMinMs = T(100);
    T releaseMaxMs = T(100);
    T ratioMin = T(4);
    T ratioMax = T(4);
    T kneeMinDb = T(6);
    T kneeMaxDb = T(6);
    T modulationMinDb = T(0);
    T modulationMaxDb = T(12);

    // Components
    EnvelopeFollower<T, EnvelopeType> envelopeFollower;
    GainComputer<T, CompressorPolicy<T>> gainComputer;
    GainSmoother<T, GainSmootherType::AttackRelease> gainSmoother;

    // State variables
    std::vector<T> detectorEnvelope;
    std::vector<T> gainReductionDb;
    std::vector<T> maxGainReduction;
    std::vector<T> lastAttackMs;
    std::vector<T> lastReleaseMs;
    std::vector<T> lastRatio;
    std::vector<T> lastKneeDb;

    void applyProgramDependentControls(size_t ch, T currentGainReductionDb) {
        const T reductionDb = std::max(-currentGainReductionDb, T(0));
        const T amount = std::clamp((reductionDb - modulationMinDb) / (modulationMaxDb - modulationMinDb), T(0), T(1));
        const auto interpolate = [amount](T minimum, T maximum) { return minimum + amount * (maximum - minimum); };

        const T attackMs = interpolate(attackMinMs, attackMaxMs);
        if (attackMs != lastAttackMs[ch]) {
            envelopeFollower.setAttackTime(Time<T>::Milliseconds(attackMs));
            lastAttackMs[ch] = attackMs;
        }

        const T releaseMs = interpolate(releaseMinMs, releaseMaxMs);
        if (releaseMs != lastReleaseMs[ch]) {
            envelopeFollower.setReleaseTime(Time<T>::Milliseconds(releaseMs));
            lastReleaseMs[ch] = releaseMs;
        }

        const T ratio = interpolate(ratioMin, ratioMax);
        if (ratio != lastRatio[ch]) {
            gainComputer.setRatio(ratio);
            lastRatio[ch] = ratio;
        }

        const T kneeDb = interpolate(kneeMinDb, kneeMaxDb);
        if (kneeDb != lastKneeDb[ch]) {
            gainComputer.setKnee(kneeDb);
            lastKneeDb[ch] = kneeDb;
        }
    }
};

} // namespace jnsc::models
