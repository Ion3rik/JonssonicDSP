// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// GainComputer policy classes
// SPDX-License-Identifier: MIT

#pragma once
#include <jonssonic/core/common/dsp_param.h>
#include <jonssonic/core/common/quantities.h>
#include <jonssonic/utils/math_utils.h>

namespace jnsc::detail {

// =============================================================================
// Compressor Policy
// =============================================================================
/**
 * @brief Compressor policy for GainComputer.
 *        Reduces gain when the input signal exceeds the threshold.
 * @tparam T Sample type
 */
template <typename T>
class CompressorPolicy {
  public:
    void prepare(size_t numChannels, T sampleRate) {
        // Prepare DSP parameters
        threshold.prepare(numChannels, sampleRate);
        ratio.prepare(numChannels, sampleRate);
        knee.prepare(numChannels, sampleRate);

        // Set safety bounds
        ratio.setBounds(T(1), std::numeric_limits<T>::max()); // ratio >= 1
        knee.setBounds(T(0), std::numeric_limits<T>::max());  // knee >= 0
    }

    T processSample(size_t ch, T input) {
        // Convert input magnitude to dB
        T inputDb = utils::mag2dB(input);

        // Get current control values
        T thresholdVal = threshold.getNextValue(ch);
        T ratioVal = ratio.getNextValue(ch);
        T kneeVal = knee.getNextValue(ch);

        // Precompute values
        T over = inputDb - thresholdVal;
        T halfKnee = kneeVal * T(0.5);
        T oneMinusInvRatio = T(1) - T(1) / ratioVal;

        // Region masks (branchless)
        T hasKnee = static_cast<T>(kneeVal > T(0));
        T inKnee = static_cast<T>(over > -halfKnee) * static_cast<T>(over < halfKnee);
        T aboveKnee = static_cast<T>(over >= halfKnee);

        // Soft knee gain
        T kneePos = over + halfKnee;
        T safeKnee = kneeVal + std::numeric_limits<T>::epsilon();
        T softKneeGain = -(oneMinusInvRatio) * (kneePos * kneePos) / (T(2) * safeKnee);

        // Hard knee gain
        T hardKneeGain = -oneMinusInvRatio * over;

        // Combine (branchless)
        return hasKnee * inKnee * softKneeGain + aboveKnee * hardKneeGain;
    }

    void setControlSmoothingTime(Time<T> time) {
        threshold.setSmoothingTime(time);
        ratio.setSmoothingTime(time);
        knee.setSmoothingTime(time);
    }
    void setThreshold(T newThresholdDb, bool skipSmoothing = false) {
        threshold.setTarget(newThresholdDb, skipSmoothing);
    }
    void setRatio(T newRatio, bool skipSmoothing = false) {
        newRatio = std::max(newRatio, T(1.0));
        ratio.setTarget(newRatio, skipSmoothing);
    }
    void setKnee(T newKneeDb, bool skipSmoothing = false) {
        newKneeDb = std::max(newKneeDb, T(0));
        knee.setTarget(newKneeDb, skipSmoothing);
    }

    bool hasKneeParam() const { return true; }
    bool hasRatioParam() const { return true; }

  private:
    DspParam<T> threshold; // threshold in dB
    DspParam<T> ratio;     // Compression ratio (ratio:1)
    DspParam<T> knee;      // knee width in dB
};

// =============================================================================
// Expander Down Policy
// =============================================================================
/**
 * @brief Expander downward policy for GainComputer.
 *        Reduces gain when the input signal is below the threshold.
 * @tparam T Sample type
 */
template <typename T>
class ExpanderDownPolicy {
  public:
    void prepare(size_t numChannels, T sampleRate) {
        // Prepare DSP parameters
        threshold.prepare(numChannels, sampleRate);
        ratio.prepare(numChannels, sampleRate);
        knee.prepare(numChannels, sampleRate);

        // Set safety bounds
        ratio.setBounds(T(1), std::numeric_limits<T>::max()); // ratio >= 1
        knee.setBounds(T(0), std::numeric_limits<T>::max());  // knee >= 0
    }

    T processSample(size_t ch, T input) {
        // Convert input magnitude to dB
        T inputDb = utils::mag2dB(input);

        // Get current control values
        T thresholdVal = threshold.getNextValue(ch);
        T ratioVal = ratio.getNextValue(ch);
        T kneeVal = knee.getNextValue(ch);

        // Precompute values
        T under = thresholdVal - inputDb;
        T halfKnee = kneeVal * T(0.5);
        T oneMinusInvRatio = T(1) - T(1) / ratioVal;

        // Region masks (branchless)
        T hasKnee = static_cast<T>(kneeVal > T(0));
        T inKnee = static_cast<T>(under > -halfKnee) * static_cast<T>(under < halfKnee);
        T aboveKnee = static_cast<T>(under >= halfKnee);

        // Soft knee gain
        T kneePos = under + halfKnee;
        T safeKnee = kneeVal + std::numeric_limits<T>::epsilon();
        T softKneeGain = -(oneMinusInvRatio) * (kneePos * kneePos) / (T(2) * safeKnee);

        // Hard knee gain
        T hardKneeGain = -oneMinusInvRatio * under;

        // Combine (branchless)
        return hasKnee * inKnee * softKneeGain + aboveKnee * hardKneeGain;
    }

    void setControlSmoothingTime(Time<T> time) {
        threshold.setSmoothingTime(time);
        ratio.setSmoothingTime(time);
        knee.setSmoothingTime(time);
    }
    void setThreshold(T newThresholdDb, bool skipSmoothing = false) {
        threshold.setTarget(newThresholdDb, skipSmoothing);
    }
    void setRatio(T newRatio, bool skipSmoothing = false) {
        newRatio = std::max(newRatio, T(1.0));
        ratio.setTarget(newRatio, skipSmoothing);
    }
    void setKnee(T newKneeDb, bool skipSmoothing = false) {
        newKneeDb = std::max(newKneeDb, T(0));
        knee.setTarget(newKneeDb, skipSmoothing);
    }

    bool hasKneeParam() const { return true; }
    bool hasRatioParam() const { return true; }

  private:
    DspParam<T> threshold; // threshold in dB
    DspParam<T> ratio;     // Expansion ratio (1:ratio)
    DspParam<T> knee;      // knee width in dB
};

// =============================================================================
// Expander Up Policy
// =============================================================================
/**
 * @brief Expander upward policy for GainComputer.
 *        Increases gain when the input signal exceeds the threshold.
 * @tparam T Sample type
 */
template <typename T>
class ExpanderUpPolicy {
  public:
    void prepare(size_t numChannels, T sampleRate) {
        // Prepare DSP parameters
        threshold.prepare(numChannels, sampleRate);
        ratio.prepare(numChannels, sampleRate);
        knee.prepare(numChannels, sampleRate);

        // Set safety bounds
        ratio.setBounds(T(1), std::numeric_limits<T>::max()); // ratio >= 1
        knee.setBounds(T(0), std::numeric_limits<T>::max());  // knee >= 0
    }

    T processSample(size_t ch, T input) {
        // Convert input magnitude to dB
        T inputDb = utils::mag2dB(input);

        // Get current control values
        T thresholdVal = threshold.getNextValue(ch);
        T ratioVal = ratio.getNextValue(ch);
        T kneeVal = knee.getNextValue(ch);

        // Precompute values
        T over = inputDb - thresholdVal;
        T halfKnee = kneeVal * T(0.5);
        T oneMinusInvRatio = T(1) - T(1) / ratioVal;

        // Region masks (branchless)
        T hasKnee = static_cast<T>(kneeVal > T(0));
        T inKnee = static_cast<T>(over > -halfKnee) * static_cast<T>(over < halfKnee);
        T aboveKnee = static_cast<T>(over >= halfKnee);

        // Soft knee gain
        T kneePos = over + halfKnee;
        T safeKnee = kneeVal + std::numeric_limits<T>::epsilon();
        T softKneeGain = (oneMinusInvRatio) * (kneePos * kneePos) / (T(2) * safeKnee);

        // Hard knee gain
        T hardKneeGain = oneMinusInvRatio * over;

        // Combine (branchless)
        return hasKnee * inKnee * softKneeGain + aboveKnee * hardKneeGain;
    }

    void setControlSmoothingTime(Time<T> time) {
        threshold.setSmoothingTime(time);
        ratio.setSmoothingTime(time);
        knee.setSmoothingTime(time);
    }
    void setThreshold(T newThresholdDb, bool skipSmoothing = false) {
        threshold.setTarget(newThresholdDb, skipSmoothing);
    }
    void setRatio(T newRatio, bool skipSmoothing = false) {
        newRatio = std::max(newRatio, T(1.0));
        ratio.setTarget(newRatio, skipSmoothing);
    }
    void setKnee(T newKneeDb, bool skipSmoothing = false) {
        newKneeDb = std::max(newKneeDb, T(0));
        knee.setTarget(newKneeDb, skipSmoothing);
    }

    bool hasKneeParam() const { return true; }
    bool hasRatioParam() const { return true; }

  private:
    DspParam<T> threshold; // threshold in dB
    DspParam<T> ratio;     // Expansion ratio (1:ratio)
    DspParam<T> knee;      // knee width in dB
};

// =============================================================================
// Limiter Policy
// =============================================================================
/**
 * @brief Limiter policy for GainComputer.
 *        Hard limits the gain to not exceed the threshold.
 * @tparam T Sample type
 */
template <typename T>
class LimiterPolicy {
  public:
    void prepare(size_t numChannels, T sampleRate) {
        // Prepare DSP parameters
        threshold.prepare(numChannels, sampleRate);
    }
    T processSample(size_t ch, T input) {
        // Convert input magnitude to dB
        T inputDb = utils::mag2dB(input);

        // Get current threshold value
        T thresholdVal = threshold.getNextValue(ch);

        // Compute gain reduction
        T gr = thresholdVal - inputDb;       // gain reduction
        T above = static_cast<T>(gr < T(0)); // are we above threshold?
        return above * gr;                   // if above, limit to threshold
    }

    void setControlSmoothingTime(Time<T> time) { threshold.setSmoothingTime(time); }
    void setThreshold(T newThresholdDb, bool skipSmoothing = false) {
        threshold.setTarget(newThresholdDb, skipSmoothing);
    }

    bool hasKneeParam() const { return false; }
    bool hasRatioParam() const { return false; }

  private:
    DspParam<T> threshold; // threshold in dB
};

// =============================================================================
// Gate Policy
// =============================================================================
/**
 * @brief Gate policy for GainComputer.
 *        Mutes the signal when it is below the threshold.
 * @tparam T Sample type
 */
template <typename T>
class GatePolicy {
  public:
    void prepare(size_t numChannels, T sampleRate) {
        // Prepare DSP parameters
        threshold.prepare(numChannels, sampleRate);
    }
    T processSample(size_t ch, T input) {
        // Convert input magnitude to dB
        T inputDb = utils::mag2dB(input);

        // Get current threshold value
        T thresholdVal = threshold.getNextValue(ch);

        // Compute gain reduction
        T below = static_cast<T>(inputDb < thresholdVal); // are we below threshold?
        return below * (T(-100.0));                       // if below, mute (-100 dB)
    }

    void setControlSmoothingTime(Time<T> time) { threshold.setSmoothingTime(time); }

    void setThreshold(T newThresholdDb, bool skipSmoothing = false) {
        threshold.setTarget(newThresholdDb, skipSmoothing);
    }

    bool hasKneeParam() const { return false; }
    bool hasRatioParam() const { return false; }

  private:
    DspParam<T> threshold; // threshold in dB
};
} // namespace jnsc::detail

// =============================================================================
// Type Aliases for public API
// =============================================================================
namespace jnsc {
template <typename T>
using CompressorPolicy = detail::CompressorPolicy<T>;
template <typename T>
using ExpanderDownPolicy = detail::ExpanderDownPolicy<T>;
template <typename T>
using ExpanderUpPolicy = detail::ExpanderUpPolicy<T>;
template <typename T>
using LimiterPolicy = detail::LimiterPolicy<T>;
template <typename T>
using GatePolicy = detail::GatePolicy<T>;

} // namespace jnsc