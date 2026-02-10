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
template <typename T, typename Engine>
class ChannelProxy;
template <typename T, typename Engine>
class SectionProxy;
template <typename T, typename Engine>
class ChannelSectionProxy;

template <typename T, typename Engine>
class Filter {
  public:
    /// Grant proxy classes access to private members
    friend class ChannelProxy<T, Engine>;
    friend class SectionProxy<T, Engine>;
    friend class ChannelSectionProxy<T, Engine>;

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
     * @brief Prepare the biquad filter for processing.
     * @param newNumChannels Number of channels
     * @param newNumSections Number of second-order sections
     * @param newSampleRate Sample rate
     * @note Must be called before processing.
     */
    void prepare(size_t newNumChannels, size_t newNumSections, T newSampleRate) {
        // Prepare the filter engine
        engine.topology.prepare(newNumChannels, newNumSections);
        // Prepare the filter design
        engine.design.prepare(newSampleRate);
    }

    /// Reset the filter state
    void reset() { engine.topology.reset(); }

    /**
     * @brief Set the filter response type for all channels and sections.
     * @param newResponse Desired filter magnitude response type
     */
    void setResponse(Response newResponse) {
        engine.design.setResponse(newResponse);
        for (size_t ch = 0; ch < engine.topology.getNumChannels(); ++ch)
            for (size_t section = 0; section < engine.topology.getNumSections(); ++section)
                engine.applyDesignToTopology(ch, section);
    }

    /**
     * @brief Set frequency for all channels and sections.
     * @param newFreq Frequency struct.
     */
    void setFrequency(Frequency<T> newFreq) {
        engine.design.setFrequency(newFreq);
        for (size_t ch = 0; ch < engine.topology.getNumChannels(); ++ch)
            for (size_t section = 0; section < engine.topology.getNumSections(); ++section)
                engine.applyDesignToTopology(ch, section);
    }

    /**
     * @brief Set Q factor for all channels and sections.
     * @param newQ Q factor value.
     */
    void setQ(T newQ) {
        engine.design.setQ(newQ);
        for (size_t ch = 0; ch < engine.topology.getNumChannels(); ++ch)
            for (size_t section = 0; section < engine.topology.getNumSections(); ++section)
                engine.applyDesignToTopology(ch, section);
    }

    /**
     * @brief Set filter gain for all channels and sections.
     * @param newGain Gain struct.
     */
    void setGain(Gain<T> newGain) {
        engine.design.setGain(newGain);
        for (size_t ch = 0; ch < engine.topology.getNumChannels(); ++ch)
            for (size_t section = 0; section < engine.topology.getNumSections(); ++section)
                engine.applyDesignToTopology(ch, section);
    }

    /// Get number of prepared channels
    size_t getNumChannels() const { return engine.topology.getNumChannels(); }
    /// Get number of prepared sections
    size_t getNumSections() const { return engine.topology.getNumSections(); }
    /// Get sample rate
    T getSampleRate() const { return engine.design.getSampleRate(); }
    /// Check if the filter is prepared
    bool isPrepared() const { return engine.topology.isPrepared(); }
    /// Get reference to the filter topology (for testing purposes)
    const typename Engine::Topology& getTopology() const { return engine.topology; }
    /// Get reference to the filter design (for testing purposes)
    const typename Engine::Design& getDesign() const { return engine.design; }

    /**
     * @brief Access a specific channel across all sections.
     * @param ch Channel index to access
     * @return A ChannelProxy for the specified channel index
     */
    ChannelProxy<T, Engine> channel(size_t ch) { return ChannelProxy<T, Engine>(*this, ch); }

    /**
     * @brief Access a specific section across all channels.
     * @param sec Section index to access
     * @return A SectionProxy for the specified section index
     */
    SectionProxy<T, Engine> section(size_t sec) { return SectionProxy<T, Engine>(*this, sec); }

  private:
    Engine engine;
};
} // namespace jnsc