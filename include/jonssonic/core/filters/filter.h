// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Filter class header file
// SPDX-License-Identifier: MIT

#pragma once

#include "detail/parametric_biquad_engine.h"

namespace jnsc {
/**
 * @brief Type aliases for common filter engine configurations
 * @typedef SeriesBiquadDF1 Series biquad filter in Direct Form I
 * @typedef ParallelBiquadDF1 Parallel biquad filter in Direct Form I
 * @typedef SeriesBiquadDF2T Series biquad filter in Direct Form II Transposed
 * @typedef ParallelBiquadDF2T Parallel biquad filter in Direct Form II Transposed
 */
template <typename T>
using SeriesBiquadDF1 = detail::ParametricBiquadEngine<T, detail::DF1Topology<T>, detail::Routing::Series>;
template <typename T>
using ParallelBiquadDF1 = detail::ParametricBiquadEngine<T, detail::DF1Topology<T>, detail::Routing::Parallel>;
template <typename T>
using SeriesBiquadDF2T = detail::ParametricBiquadEngine<T, detail::DF2TTopology<T>, detail::Routing::Series>;
template <typename T>
using ParallelBiquadDF2T = detail::ParametricBiquadEngine<T, detail::DF2TTopology<T>, detail::Routing::Parallel>;

/**
 * @brief Filter class implementing a multi-channel, multi-section filter.
 * @tparam T Sample data type (e.g., float, double)
 * @tparam Engine Filter engine type (e.g., ParametricBiquadEngine)
 */
template <typename T, typename Engine = SeriesBiquadDF2T<T>>
class Filter {
  public:
    /// Filter response type alias for easier access
    using Response = typename Engine::Design::Response;

    /// Default constructor
    Filter() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels
     * @param newNumSections Number of second-order sections
     * @param newSampleRate Sample rate
     */
    Filter(size_t newNumChannels, size_t newNumSections, T newSampleRate) {
        prepare(newNumChannels, newNumSections, newSampleRate);
    }
    /// Default destructor
    ~Filter() = default;

    /// No copy or move semantics
    Filter(const Filter&) = delete;
    Filter& operator=(const Filter&) = delete;
    Filter(Filter&&) = delete;
    Filter& operator=(Filter&&) = delete;

    /**
     * @brief Prepare the biquad Filter for processing.
     * @param newNumChannels Number of channels
     * @param newNumSections Number of second-order sections
     * @param newSampleRate Sample rate
     * @note Must be called before processing.
     */
    void prepare(size_t newNumChannels, size_t newNumSections, T newSampleRate) {
        engine.prepare(newNumChannels, newNumSections, newSampleRate);
    }

    /// Reset the Filter state
    void reset() { engine.reset(); }

    /// Get number of prepared channels
    size_t getNumChannels() { return engine.getTopology().getNumChannels(); }
    /// Get number of prepared sections
    size_t getNumSections() { return engine.getTopology().getNumSections(); }
    /// Get sample rate
    T getSampleRate() { return engine.getDesign().getSampleRate(); }
    /// Check if the Filter is prepared
    bool isPrepared() { return engine.getTopology().isPrepared(); }
    /// Get reference to the Filter engine (read-only)
    const Engine& getEngine() const { return engine; }
    /// Get reference to the Filter engine (read-write)
    Engine& getEngine() { return engine; }

  private:
    Engine engine;
};
} // namespace jnsc