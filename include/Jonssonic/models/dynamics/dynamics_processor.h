// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// GainControlStage class header file
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/utils/detail/config_utils.h"
#include <jonssonic/core/common/quantities.h>
#include <jonssonic/core/dynamics/_dynamics.h>
#include <jonssonic/core/filters/biquad_filter.h>

namespace jonssonic::models::dynamics {
/// Detector type enumeration (feedforward or feedback)
enum class DetectorType { Feedforward, Feedback };

// =============================================================================
/**
 * @brief GainControlStage combines an Envelope Follower, Gain Computer, and Gain Smoother into a
 * single processing stage.
 * @tparam T Sample type
 * @tparam EnvelopeType Type of envelope follower (default: RMS)
 * @tparam GainPolicy Type of gain computer (default: Compressor)
 * @tparam GainSmootherType Type of gain smoother (default: AttackRelease)
 * @tparam DetectorType Type of detector (default: Feedforward)
 * @tparam Toggle SideChainFilter If true, enables side-chain filtering (default: false)
 */
template <typename T,
          EnvelopeType EnvelopeType = EnvelopeType::RMS,
          typename GainPolicy = CompressorPolicy<T>,
          GainSmootherType GainSmootherType = GainSmootherType::AttackRelease,
          DetectorType DetectorType = DetectorType::Feedforward,
          bool SideChainFilter = false>
class GainControlStage {
    /// Type aliases for convenience
    using EnvelopeFollowerType = core::dynamics::EnvelopeFollower<T, EnvelopeType>;
    using GainComputerType = core::dynamics::GainComputer<T, GainPolicy>;
    using GainSmootherTypeAlias = core::dynamics::GainSmoother<T, GainSmootherType>;
    using SideChainFilterType = core::filters::BiquadFilter<T>;

  public:
    /// Default constructor.
    GainControlStage() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param numChannels Number of channels
     * @param sampleRate Sample rate in Hz
     */
    GainControlStage(size_t numChannels, T sampleRate) { prepare(numChannels, sampleRate); }

    /// Default destructor.
    ~GainControlStage() = default;

    /// No copy nor move semantics
    GainControlStage(const GainControlStage &) = delete;
    GainControlStage &operator=(const GainControlStage &) = delete;
    GainControlStage(GainControlStage &&) = delete;
    GainControlStage &operator=(GainControlStage &&) = delete;

    /**
     * @brief Prepare the gain control stage for processing.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     */
    void prepare(size_t newNumChannels, T newSampleRate) {
        // Store config
        numChannels = utils::detail::clampChannels(newNumChannels);
        sampleRate = utils::detail::clampSampleRate(newSampleRate);

        // Prepare main components
        envelopeFollower.prepare(numChannels, sampleRate);
        gainComputer.prepare(numChannels, sampleRate);
        gainSmoother.prepare(numChannels, sampleRate);

        // Prepare side-chain filter if enabled
        if constexpr (SideChainFilter) {
            sideChainFilter.prepare(numChannels, sampleRate);
        }
        // Resize state variables for feedback detector
        if constexpr (DetectorType == DetectorType::Feedback) {
            previousGainLin.resize(numChannels, T(1));
        }
    }

    /**
     * @brief Reset the gain control stage state.
     */
    void reset() {
        envelopeFollower.reset();
        gainSmoother.reset();
        if constexpr (SideChainFilter) {
            sideChainFilter.reset();
        }
        if constexpr (DetectorType == DetectorType::Feedback) {
            std::fill(previousGainLin.begin(), previousGainLin.end(), T(1));
        }
    }

    /**
     * @brief Process a single sample for a given channel.
     * @param ch Channel index.
     * @param input Input signal sample.
     * @param detectorInput Detector input sample (either same as input or from side-chain).
     * @return Processed output sample.
     * @note Must call @ref prepare before processing.
     */
    T processSample(size_t ch, T input, T detectorInput) {
        // FEEDFORWARD DETECTOR
        if constexpr (DetectorType == DetectorType::Feedforward) {
            // Side chain filtering if exists
            if constexpr (SideChainFilter) {
                detectorInput = sideChainFilter.processSample(ch, detectorInput);
            }
            // Route detector input to envelope follower
            T env = envelopeFollower.processSample(ch, detectorInput);

            // Gain computation
            T gainDb = gainComputer.processSample(ch, env);

            // Gain smoothing
            T smoothGainLin = gainSmoother.processSample(ch, gainDb);

            // Apply gain
            return input * smoothGainLin;
        }
        // FEEDBACK DETECTOR
        else if constexpr (DetectorType == DetectorType::Feedback) {
            // Apply feedback gain to detector input
            T detectorSignal = detectorInput * previousGainLin[ch];

            // Side chain filtering if exists
            if constexpr (SideChainFilter) {
                detectorSignal = sideChainFilter.processSample(ch, detectorSignal);
            }

            // Envelope follower
            T env = envelopeFollower.processSample(ch, detectorSignal);

            // Gain computation
            T gainDb = gainComputer.processSample(ch, env);

            // Gain smoothing
            T smoothGainLin = gainSmoother.processSample(ch, gainDb);

            // Apply gain and update feedback state
            previousGainLin[ch] = smoothGainLin;
            return input * smoothGainLin;
        }
    }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input (signal) sample pointers (one per channel)
     * @param detectorInput Detector input sample pointers (one per channel)
     * @param output Output (processed signal) sample pointers (one per channel)
     * @param numSamples Number of samples to process
     * @note Must call @ref prepare before processing.
     */
    void processBlock(const T *const *input,
                      const T *const *detectorInput,
                      T *const *output,
                      size_t numSamples) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t n = 0; n < numSamples; ++n) {
                output[ch][n] = processSample(ch, input[ch][n], detectorInput[ch][n]);
            }
        }
    }

    /**
     * @brief Set the control parameter smoothing time.
     * @param time Smoothing time
     * @note Applies to all internal components.
     */
    void setControlSmoothingTime(Time<T> time) {
        envelopeFollower.setControlSmoothingTime(time);
        gainComputer.setControlSmoothingTime(time);
        gainSmoother.setControlSmoothingTime(time);
    }

    /**
     * @brief Set the envelope follower attack time.
     * @param time Attack time
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */

    void setEnvelopeAttackTime(Time<T> time, bool skipSmoothing = false) {
        envelopeFollower.setAttackTime(time, skipSmoothing);
    }

    /**
     * @brief Set the envelope follower release time.
     * @param time Release time
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setEnvelopeReleaseTime(Time<T> time, bool skipSmoothing = false) {
        envelopeFollower.setReleaseTime(time, skipSmoothing);
    }

    /**
     * @brief Set threshold in dB.
     * @param newThresholdDb New threshold value in dB
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setThreshold(T newThresholdDb, bool skipSmoothing = false) {
        gainComputer.setThreshold(newThresholdDb, skipSmoothing);
    }

    /**
     * @brief Set compression/expansion ratio.
     * @param newRatio New ratio value
     * @param skipSmoothing If true, skip smoothing and set immediately.
     * @note Applies only if GainPolicy supports ratio (i.e., compression or expansion).
     */
    void setRatio(T newRatio, bool skipSmoothing = false) {
        gainComputer.setRatio(newRatio, skipSmoothing);
    }

    /**
     * @brief Set knee width in dB.
     * @param newKneeDb New knee width value in dB
     * @param skipSmoothing If true, skip smoothing and set immediately.
     * @note Applies only if GainPolicy supports knee (i.e., compression or expansion).
     */
    void setKnee(T newKneeDb, bool skipSmoothing = false) {
        gainComputer.setKnee(newKneeDb, skipSmoothing);
    }

    /**
     * @brief Set gain smoother attack time.
     * @param time Attack time
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setGainSmootherAttackTime(Time<T> time, bool skipSmoothing = false) {
        gainSmoother.setAttackTime(time, skipSmoothing);
    }

    /**
     * @brief Set gain smoother release time.
     * @param time Release time
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setGainSmootherReleaseTime(Time<T> time, bool skipSmoothing = false) {
        gainSmoother.setReleaseTime(time, skipSmoothing);
    }

    /// Get the number of channels.
    size_t getNumChannels() const { return numChannels; }

    /// Get the sample rate.
    T getSampleRate() const { return sampleRate; }

  private:
    // Config variables
    size_t numChannels = 0;
    T sampleRate = T(44100);

    // Components
    EnvelopeFollowerType envelopeFollower;
    GainComputerType gainComputer;
    GainSmootherTypeAlias gainSmoother;
    SideChainFilterType sideChainFilter;

    // State variables
    std::vector<T> previousGainLin; // Needed for feedback detector
};
} // namespace jonssonic::models::dynamics