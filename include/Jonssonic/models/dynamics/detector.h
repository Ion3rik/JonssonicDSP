// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Detector class header file
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/utils/detail/config_utils.h"
#include <jonssonic/core/common/quantities.h>
#include <jonssonic/core/dynamics/envelope_follower.h>
#include <jonssonic/core/nonlinear/wave_shaper.h>
#include <variant>

namespace jonssonic::models::dynamics {

/**
 * @brief Detector class template.
 * Wraps the @ref EnvelopeFollower with runtime-selectable type and optional program dependent
 * release.
 * @tparam T Sample type
 */

template <typename T>
class Detector {
    /// Type alias for convenience
    using EnvelopeType = core::dynamics::EnvelopeType;
    using PeakFollowerType = core::dynamics::EnvelopeFollower<T, EnvelopeType::Peak>;
    using RMSFollowerType = core::dynamics::EnvelopeFollower<T, EnvelopeType::RMS>;

    /// Envelope follower variant
    using EnvelopeFollowerVariant = std::variant<PeakFollowerType, RMSFollowerType>;

  public:
    /// Default constructor.
    Detector() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param numChannels Number of channels
     * @param sampleRate Sample rate in Hz
     */
    Detector(size_t numChannels, T sampleRate) { prepare(numChannels, sampleRate); }

    /// Default destructor.
    ~Detector() = default;

    /// No copy nor move semantics
    Detector(const Detector&) = delete;
    Detector& operator=(const Detector&) = delete;
    Detector(Detector&&) = delete;
    Detector& operator=(Detector&&) = delete;

    /**
     * @brief Prepare the dynamics processor for processing.
     * @param numChannels Number of channels
     * @param sampleRate Sample rate in Hz
     */
    void prepare(size_t numChannels, T sampleRate) {
        envelopeFollower.prepare(numChannels, sampleRate);
    }

    /**
     * @brief Reset the dynamics processor state.
     */
    void reset() { envelopeFollower.reset(); }

    /**
     * @brief Process a single sample for a given channel.
     * @param ch Channel index.
     * @param input Input signal sample.
     * @return Detected envelope sample.
     * @note Must call @ref prepare before processing.
     */
    T processSample(size_t ch, T input) {
        return std::visit([&](auto& follower) { return follower.processSample(ch, input); },
                          envelopeFollower);
    }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input (signal) sample pointers (one per channel)
     * @param output Output (processed signal) sample pointers (one per channel)
     * @param numSamples Number of samples to process
     * @note Must call @ref prepare before processing.
     */
    void processBlock(const T* const* input, T* const* output, size_t numSamples) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t n = 0; n < numSamples; ++n) {
                output[ch][n] = processSample(ch, input[ch][n]);
            }
        }
    }

    /**
     * @brief Set control parameter smoothing time.
     * @param time Smoothing time
     */
    void setControlSmoothingTime(Time<T> time) { envelopeFollower.setControlSmoothingTime(time); }

    /**
     * @brief Set mode of the detector.
     * @param type Envelope follower type
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setMode(EnvelopeType type, bool skipSmoothing = false) {
        // Store old state and params
        auto envelopeState = envelopeFollower.getState();
        auto envelopeParams = envelopeFollower.getParams();

        // Recreate the variant with the selected type
        switch (type) {
        case EnvelopeType::Peak:
            envelopeFollower = PeakFollowerType();
            break;
        case EnvelopeType::RMS:
            envelopeFollower = RMSFollowerType();
            break;
        default:
            assert(false && "Unknown EnvelopeType");
            break;
        }
        // Restore state and params
        envelopeFollower.setState(envelopeState);
        envelopeFollower.setParams(envelopeParams);
    }

    /**
     * @brief Set the attack time.
     * @param time Attack time
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setAttackTime(Time<T> time, bool skipSmoothing = false) {
        envelopeFollower.setAttackTime(time, skipSmoothing);
    }

    /**
     * @brief Set the release time.
     * @param time Release time
     * @param skipSmoothing If true, skip smoothing and set immediately.
     */
    void setReleaseTime(Time<T> time, bool skipSmoothing = false) {
        envelopeFollower.setReleaseTime(time, skipSmoothing);
    }

    /// Get the number of channels.
    size_t getNumChannels() const { return envelopeFollower.getNumChannels(); }

    /// Get the sample rate.
    T getSampleRate() const { return envelopeFollower.getSampleRate(); }

  private:
    // Components
    EnvelopeFollowerVariant envelopeFollower;
};
} // namespace jonssonic::models::dynamics