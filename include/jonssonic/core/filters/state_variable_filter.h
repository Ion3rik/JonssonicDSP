// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// State Variable Filter class header file
// SPDX-License-Identifier: MIT

#pragma once

#include "jonssonic/utils/detail/config_utils.h"
#include <jonssonic/utils/math_utils.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <jonssonic/core/filters/detail/filter_limits.h>
#include <jonssonic/core/filters/routing.h>
#include <vector>

namespace jnsc {
/**
 * @brief Multi-channel TPT state variable filter with sample-by-sample frequency and Q inputs.
 * Supports multiple second-order sections with series or parallel routing.
 * Parameter storage and smoothing belong to the caller.
 * @tparam T Sample data type (e.g., float, double).
 * @tparam RoutingType Filter routing type (Series or Parallel).
 */
template <typename T, Routing RoutingType = Routing::Series>
class StateVariableFilter {
  public:
    /// Default constructor.
    StateVariableFilter() = default;

    /// Parameterized constructor that calls prepare.
    StateVariableFilter(size_t newNumChannels, T newSampleRate, size_t newNumSections = 1) {
        prepare(newNumChannels, newSampleRate, newNumSections);
    }

    /// Default destructor.
    ~StateVariableFilter() = default;

    /// No copy nor move semantics.
    StateVariableFilter(const StateVariableFilter&) = delete;
    StateVariableFilter& operator=(const StateVariableFilter&) = delete;
    StateVariableFilter(StateVariableFilter&&) = delete;
    StateVariableFilter& operator=(StateVariableFilter&&) = delete;

    /// Prepare channels and sections, clearing integrator states. Sample rate is in Hz.
    void prepare(size_t newNumChannels, T newSampleRate, size_t newNumSections = 1) {
        numChannels = utils::detail::clampChannels(newNumChannels);
        sampleRate = utils::detail::clampSampleRate(newSampleRate);
        numSections = detail::FilterLimits<T>::clampSections(newNumSections);
        state.assign(numChannels * numSections, {});
    }

    /// Clear the integrator states for all channels.
    void reset() {
        for (auto& sectionState : state)
            for (auto& responseState : sectionState)
                responseState.fill(T(0));
    }

    /**
     * @brief Process one sample, returning {lowpass, highpass, bandpass, allpass}.
     * @param ch Channel index.
     * @param input Input sample.
     * @param f Frequency in Hz, clamped to the filter's supported range.
     * @param Q Quality factor, clamped to the filter's supported range.
     * @note Call prepare before processing. Parameters apply to all sections immediately without smoothing.
     * Series routing cascades each response independently. Parallel routing sums matching responses.
     */
    std::array<T, 4> processSample(size_t ch, T input, T f, T Q) {
        f = std::clamp(f,
                       sampleRate * detail::FilterLimits<T>::MIN_FREQ_NORM,
                       sampleRate * detail::FilterLimits<T>::MAX_FREQ_NORM);
        Q = detail::FilterLimits<T>::clampQ(Q);
        T g = std::tan(utils::pi<T> * f / sampleRate);
        T twoR = T(1) / Q;
        T d = T(1) / (T(1) + twoR * g + g * g);

        std::array<T, 4> output{};
        const size_t stateBase = ch * numSections;
        if constexpr (RoutingType == Routing::Series) {
            // The first section shares its integrators across all responses.
            output = processSection(input, g, twoR, d, state[stateBase][0]);
            // Later sections need independent state for each response cascade.
            for (size_t section = 1; section < numSections; ++section)
                for (size_t response = 0; response < output.size(); ++response)
                    output[response] = processSection(output[response], g, twoR, d,
                                                      state[stateBase + section][response])[response];
        }
        if constexpr (RoutingType == Routing::Parallel) {
            for (size_t section = 0; section < numSections; ++section) {
                const auto sectionOutput = processSection(input, g, twoR, d, state[stateBase + section][0]);
                for (size_t response = 0; response < output.size(); ++response)
                    output[response] += sectionOutput[response];
            }
        }
        return output;
    }

    /**
     * @brief Process a block, writing all four responses.
     * @param input Input pointers [channel][sample].
     * @param f Frequency pointers in Hz [channel][sample].
     * @param Q Quality factor pointers [channel][sample].
     * @param lowpass Lowpass output pointers [channel][sample].
     * @param highpass Highpass output pointers [channel][sample].
     * @param bandpass Bandpass output pointers [channel][sample].
     * @param allpass Allpass output pointers [channel][sample].
     * @param numSamples Number of samples to process.
     * @note Output buffers must be distinct. Any one output may alias input.
     */
    void processBlock(const T* const* input,
                      const T* const* f,
                      const T* const* Q,
                      T* const* lowpass,
                      T* const* highpass,
                      T* const* bandpass,
                      T* const* allpass,
                      size_t numSamples) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t n = 0; n < numSamples; ++n) {
                const auto outputs = processSample(ch, input[ch][n], f[ch][n], Q[ch][n]);
                lowpass[ch][n] = outputs[0];
                highpass[ch][n] = outputs[1];
                bandpass[ch][n] = outputs[2];
                allpass[ch][n] = outputs[3];
            }
        }
    }

    /// Check if the filter is prepared.
    bool isPrepared() const { return !state.empty(); }
    /// Get the number of channels.
    size_t getNumChannels() const { return numChannels; }
    /// Get the number of sections per channel.
    size_t getNumSections() const { return numSections; }
    /// Get the sample rate in Hz.
    T getSampleRate() const { return sampleRate; }

  private:
    // Process one section with the selected cascade's integrator state.
    static std::array<T, 4> processSection(T input, T g, T twoR, T d, std::array<T, 2>& sectionState) {
        T hp = (input - sectionState[1] - (twoR + g) * sectionState[0]) * d;
        T v0 = g * hp;
        T bp = v0 + sectionState[0];
        T v1 = g * bp;
        T lp = v1 + sectionState[1];
        T ap = hp + lp - twoR * bp;
        sectionState[0] = bp + v0;
        sectionState[1] = lp + v1;
        return {lp, hp, bp, ap};
    }

    size_t numChannels = 0;
    size_t numSections = 0;
    T sampleRate = T(44100);
    // Two integrator states per response cascade, per channel and section.
    std::vector<std::array<std::array<T, 2>, 4>> state;
};

} // namespace jnsc
