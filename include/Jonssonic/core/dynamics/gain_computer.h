// Jonssonic - A Modular Realtime C++ Audio DSP Library
// GainComputer class header file
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/utils/detail/config_utils.h"
#include <cmath>
#include <jonssonic/core/common/dsp_param.h>
#include <jonssonic/core/common/quantities.h>
#include <jonssonic/utils/math_utils.h>
#include <limits>

namespace jonssonic::core::dynamics {

enum class GainType {
    Compressor
    // ExpanderDownward,
    // ExpanderUpward,
    // Limiter,
    // Gate
    //  Add more types as needed
};

// =============================================================================
// Template Declaration
// =============================================================================
template <typename T, GainType Type = GainType::Compressor> class GainComputer;

// =============================================================================
// Compressor Gain Computer Specialization
// =============================================================================
template <typename T> class GainComputer<T, GainType::Compressor> {
    /// Type aliases for convenience, readability, and future-proofing
    using DspParamType = common::DspParam<T>;

  public:
    /// Default constructor.
    GainComputer() = default;
    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param numChannels Number of channels
     * @param sampleRate Sample rate in Hz
     */
    GainComputer(size_t newNumChannels, T newSampleRate) { prepare(newNumChannels, newSampleRate); }

    /// Default destructor.
    ~GainComputer() = default;

    /// No copy nor move semantics
    GainComputer(const GainComputer &) = delete;
    GainComputer &operator=(const GainComputer &) = delete;
    GainComputer(GainComputer &&) = delete;
    GainComputer &operator=(GainComputer &&) = delete;

    /**
     * @brief Prepare the gain computer for processing.
     * @param numChannels Number of channels
     * @param sampleRate Sample rate in Hz
     */
    void prepare(size_t numNewChannels, T newSampleRate) {
        // Clamp and store number of channels and sample rate
        numChannels = utils::detail::clampChannels(numNewChannels);
        sampleRate = utils::detail::clampSampleRate(newSampleRate);

        // Prepare parameters
        threshold.prepare(numChannels, sampleRate);
        ratio.prepare(numChannels, sampleRate);
        knee.prepare(numChannels, sampleRate);
    }

    /**
     * @brief Process a single sample for a given channel.
     * @param ch Channel index
     * @param input Input signal sample
     * @return Computed dB gain
     */
    T processSample(size_t ch, T input) {
        // convert input to dB
        T inputDb = utils::mag2dB(input);
        T thresholdVal = threshold.getNextValue(ch);
        T ratioVal = ratio.getNextValue(ch);
        T kneeVal = knee.getNextValue(ch);

        T over = inputDb - thresholdVal;                   // amount over threshold
        T halfKnee = kneeVal * T(0.5);                     // half knee width
        T kneePos = over + halfKnee;                       // position within knee
        T oneMinusInvRatio = (T(1.0) - T(1.0) / ratioVal); // 1 - 1/ratio

        // Soft knee branchless
        T inSoftKnee = static_cast<T>(kneeVal > 0);     // mask for soft knee
        T below = static_cast<T>(over <= -halfKnee);    // mask for below knee
        T above = static_cast<T>(over >= halfKnee);     // mask for above knee
        T within = inSoftKnee * (T(1) - below - above); // mask for within knee

        constexpr T epsilon = std::numeric_limits<T>::epsilon();
        T isZeroKnee = static_cast<T>(kneeVal == T(0));
        T safeKnee = kneeVal + isZeroKnee * epsilon; // avoid division by zero
        T gainDb_soft = below * T(0) + above * ((thresholdVal - inputDb) * oneMinusInvRatio) +
                        within * (-(oneMinusInvRatio) * (kneePos * kneePos) / (T(2.0) * safeKnee));

        // Hard knee branchless
        T aboveHard = static_cast<T>(inputDb > thresholdVal);
        T gainDb_hard = aboveHard * ((thresholdVal - inputDb) * oneMinusInvRatio);

        // Select soft or hard knee
        return inSoftKnee * gainDb_soft + (T(1) - inSoftKnee) * gainDb_hard;
    }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input sample pointers (one per channel)
     * @param output Output sample pointers (one per channel)
     * @param numSamples Number of samples to process
     */
    void processBlock(const T *const *input, T *const *output, size_t numSamples) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t n = 0; n < numSamples; ++n) {
                output[ch][n] = processSample(ch, input[ch][n]);
            }
        }
    }

    /**
     * @brief Set the control smoothing time in various units.
     * @param time Smoothing time struct.
     */
    void setControlSmoothingTime(Time<T> time) {
        threshold.setSmoothingTime(time);
        ratio.setSmoothingTime(time);
        knee.setSmoothingTime(time);
    }

    /**
     * @brief Set threshold level.
     * @param newThreshold Threshold in dB
     * @param skipSmoothing If true, update coefficients immediately
     */
    void setThreshold(T newThresholdDb, bool skipSmoothing = false) {
        threshold.setTarget(newThresholdDb, skipSmoothing);
    }

    /**
     * @brief Set ratio.
     * @param newRatioDb Compression ratio
     * @param skipSmoothing If true, update coefficients immediately
     */
    void setRatio(T newRatio, bool skipSmoothing = false) {
        newRatio = std::max(newRatio, T(1.0)); // prevent ratio < 1
        ratio.setTarget(newRatio, skipSmoothing);
    }

    /**
     * @brief Set knee width in dB.
     * @param newKneeDb Knee width in dB
     * @param skipSmoothing If true, update coefficients immediately
     */
    void setKnee(T newKneeDb, bool skipSmoothing = false) {
        newKneeDb = std::max(newKneeDb, T(0)); // prevent negative knee
        knee.setTarget(newKneeDb, skipSmoothing);
    }

    size_t getNumChannels() const { return numChannels; }
    T getSampleRate() const { return sampleRate; }

  private:
    // Global parameters
    size_t numChannels = 0;
    T sampleRate = T(44100);

    // User Parameters
    DspParamType threshold; // threshold in dB
    DspParamType ratio;     // Compression ratio
    DspParamType knee;      // knee width in dB
};
} // namespace jonssonic::core::dynamics