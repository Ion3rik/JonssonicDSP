// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// GainControlStage class header file
// SPDX-License-Identifier: MIT

#pragma once
#include <jonssonic/core/common/quantities.h>
#include <jonssonic/core/dynamics/_dynamics.h>

namespace jonssonic::models::dynamics {
/**
 * @brief GainControlStage combines an Envelope Follower, Gain Computer, and Gain Smoother into a
 * single processing stage.
 * @tparam T Sample type
 * @tparam EnvelopeType Type of envelope follower (default: RMS)
 * @tparam GainType Type of gain computer (default: Compressor)
 * @tparam GainSmootherType Type of gain smoother (default: AttackRelease)
 */
template <typename T,
          EnvelopeType EnvelopeType = EnvelopeType::RMS,
          GainType GainType = GainType::Compressor,
          GainSmootherType GainSmootherType = GainSmootherType::AttackRelease>
class GainControlStage {
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

  private:
    // Components
};
} // namespace jonssonic::models::dynamics