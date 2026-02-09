// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Filter class header file
// SPDX-License-Identifier: MIT

#pragma once

#include "jonssonic/utils/detail/config_utils.h"
#include <jonssonic/core/common/dsp_param.h>
#include <jonssonic/core/common/quantities.h>
#include <jonssonic/core/filters/detail/filter_limits.h>
#include <jonssonic/core/filters/detail/filter_proxies.h>
#include <jonssonic/core/filters/filter_topologies.h>

namespace jnsc {
/// Forward declarations for proxy classes
template <typename T, typename Topology>
class ChannelProxy;
template <typename T, typename Topology>
class SectionProxy;
template <typename T, typename Topology>
class ChannelSectionProxy;

template <typename T, typename Topology>
class Filter {
  public:
    /// Grant proxy classes access to private members
    friend class ChannelProxy<T, Topology>;
    friend class SectionProxy<T, Topology>;
    friend class ChannelSectionProxy<T, Topology>;

    /// Filter response type alias for easier access
    using Response = typename Topology::Design::Response;

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
     * @brief Prepare the biquad filter for processing.
     * @param newNumChannels Number of channels
     * @param newNumSections Number of second-order sections
     * @param newSampleRate Sample rate
     * @note Must be called before processing.
     */
    void prepare(size_t newNumChannels, size_t newNumSections, T newSampleRate) {
        // Prepare the filter engine
        topology.engine.prepare(newNumChannels, newNumSections);
        // Prepare the filter design
        topology.design.prepare(newSampleRate);
    }

    /// Reset the filter state
    void reset() { topology.engine.reset(); }

    /**
     * @brief Process a single sample for a given channel.
     * @param ch Channel index.
     * @param input Input sample.
     * @return Output sample.
     * @note Must call @ref prepare before processing.
     */
    T processSample(size_t ch, T input) { return topology.engine.processSample(ch, input); }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input sample pointers (one per channel).
     * @param output Output sample pointers (one per channel).
     * @param numSamples Number of samples to process
     * @note Must call @ref prepare before processing.
     */
    void processBlock(const T* const* input, T* const* output, size_t numSamples) {
        topology.engine.processBlock(input, output, numSamples);
    }

    /**
     * @brief Set the filter response type for all channels and sections.
     * @param newResponse Desired filter magnitude response type
     */
    void setResponse(Response newResponse) {
        topology.design.setResponse(newResponse);
        for (size_t ch = 0; ch < topology.engine.getNumChannels(); ++ch)
            for (size_t section = 0; section < topology.engine.getNumSections(); ++section)
                topology.applyDesignToEngine(ch, section);
    }

    /**
     * @brief Set frequency for all channels and sections.
     * @param newFreq Frequency struct.
     */
    void setFrequency(Frequency<T> newFreq) {
        topology.design.setFrequency(newFreq);
        for (size_t ch = 0; ch < topology.engine.getNumChannels(); ++ch)
            for (size_t section = 0; section < topology.engine.getNumSections(); ++section)
                topology.applyDesignToEngine(ch, section);
    }

    /**
     * @brief Set Q factor for all channels and sections.
     * @param newQ Q factor value.
     */
    void setQ(T newQ) {
        topology.design.setQ(newQ);
        for (size_t ch = 0; ch < topology.engine.getNumChannels(); ++ch)
            for (size_t section = 0; section < topology.engine.getNumSections(); ++section)
                topology.applyDesignToEngine(ch, section);
    }

    /**
     * @brief Set filter gain for all channels and sections.
     * @param newGain Gain struct.
     */
    void setGain(Gain<T> newGain) {
        topology.design.setGain(newGain);
        for (size_t ch = 0; ch < topology.engine.getNumChannels(); ++ch)
            for (size_t section = 0; section < topology.engine.getNumSections(); ++section)
                topology.applyDesignToEngine(ch, section);
    }

    /// Get number of prepared channels
    size_t getNumChannels() const { return topology.engine.getNumChannels(); }
    /// Get number of prepared sections
    size_t getNumSections() const { return topology.engine.getNumSections(); }
    /// Get sample rate
    T getSampleRate() const { return topology.design.getSampleRate(); }
    /// Check if the filter is prepared
    bool isPrepared() const { return topology.engine.isPrepared(); }
    /// Get reference to the filter engine (for testing purposes)
    const typename Topology::Engine& getEngine() const { return topology.engine; }
    /// Get reference to the filter design (for testing purposes)
    const typename Topology::Design& getDesign() const { return topology.design; }

    /**
     * @brief Access a specific channel across all sections.
     * @param ch Channel index to access
     * @return A ChannelProxy for the specified channel index
     */
    ChannelProxy<T, Topology> channel(size_t ch) { return ChannelProxy<T, Topology>(*this, ch); }

    /**
     * @brief Access a specific section across all channels.
     * @param sec Section index to access
     * @return A SectionProxy for the specified section index
     */
    SectionProxy<T, Topology> section(size_t sec) { return SectionProxy<T, Topology>(*this, sec); }

  private:
    Topology topology;
};
} // namespace jnsc