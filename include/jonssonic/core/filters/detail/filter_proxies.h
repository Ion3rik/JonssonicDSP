// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Proxy classes for filter channel and section parameter setting
// SPDX-License-Identifier: MIT

#pragma once
#include <cassert>
#include <cstddef>

namespace jnsc {

// Forward declarations
template <typename T, typename Topology>
class Filter;
template <typename T, typename Topology>
class ChannelSectionProxy;

/**
 * @brief Proxy for accessing a specific channel across all sections.
 * @tparam T Sample data type (e.g., float, double)
 * @tparam Topology Filter topology type (e.g., BiquadDF1)
 */
template <typename T, typename Topology>
class ChannelProxy {
  public:
    /// Type aliases for easier access to response type and parent filter type
    using Response = typename Topology::Design::Response;
    using FilterType = Filter<T, Topology>;

    /**
     * @brief Construct a ChannelProxy for a specific channel index.
     * @param filter Reference to the parent filter
     * @param channelIdx Channel index to access
     * @note Asserts if channelIdx is out of bounds based on the filter's number of channels.
     */
    ChannelProxy(FilterType& filter, size_t channelIdx) : filter(filter), channelIdx(channelIdx) {
        assert(channelIdx < filter.getNumChannels() && "Channel index out of bounds");
    }

    /// Copy constructor
    ChannelProxy(const ChannelProxy&) = default;

    /**
     * @brief Access a specific section within this channel.
     * @param sectionIdx Section index to access
     * @return A proxy object for the specified channel-section pair
     */
    ChannelSectionProxy<T, Topology> section(size_t sectionIdx);

    /**
     * @brief Set the filter response type for all sections of this channel.
     * @param newResponse Desired filter magnitude response type
     */
    void setResponse(Response newResponse) {
        // Compute new design coefficients based on the new response type
        filter.topology.design.setResponse(newResponse);

        // Set the new coefficients for all sections of this channel
        for (size_t s = 0; s < filter.getNumSections(); ++s)
            filter.topology.applyDesignToEngine(channelIdx, s);
    }

    /**
     * @brief Set the filter gain for all sections of this channel.
     * @param newGain Desired gain value.
     */
    void setGain(Gain<T> newGain) {
        // Compute new design coefficients based on the new gain
        filter.topology.design.setGain(newGain);

        // Set the new coefficients for all sections of this channel
        for (size_t s = 0; s < filter.getNumSections(); ++s)
            filter.topology.applyDesignToEngine(channelIdx, s);
    }

    /**
     * @brief Set the filter frequency for all sections of this channel.
     * @param newFreq Desired frequency value.
     */
    void setFrequency(Frequency<T> newFreq) {
        // Compute new design coefficients based on the new frequency
        filter.topology.design.setFrequency(newFreq);

        // Set the new coefficients for all sections of this channel
        for (size_t s = 0; s < filter.getNumSections(); ++s)
            filter.topology.applyDesignToEngine(channelIdx, s);
    }
    /**
     * @brief Set the filter Q factor for all sections of this channel.
     * @param newQ Desired Q factor value.
     */
    void setQ(T newQ) {
        // Compute new design coefficients based on the new Q factor
        filter.topology.design.setQ(newQ);

        // Set the new coefficients for all sections of this channel
        for (size_t s = 0; s < filter.getNumSections(); ++s)
            filter.topology.applyDesignToEngine(channelIdx, s);
    }

  private:
    FilterType& filter;
    size_t channelIdx;
};

/**
 * @brief Proxy for accessing a specific section across all channels.
 * @tparam T Sample data type (e.g., float, double)
 * @tparam Topology Filter topology type (e.g., BiquadDF1)
 */
template <typename T, typename Topology>
class SectionProxy {
  public:
    /// Type aliases for easier access to response type and parent filter type
    using Response = typename Topology::Design::Response;
    using FilterType = Filter<T, Topology>;

    /**
     * @brief Construct a SectionProxy for a specific section index.
     * @param filter Reference to the parent filter
     * @param sectionIdx Section index to access
     * @note Asserts if sectionIdx is out of bounds based on the filter's number of sections.
     */
    SectionProxy(FilterType& filter, size_t sectionIdx) : filter(filter), sectionIdx(sectionIdx) {
        assert(sectionIdx < filter.getNumSections() && "Section index out of bounds");
    }

    /// Copy constructor
    SectionProxy(const SectionProxy&) = default;

    /**
     * @brief Access a specific channel within this section.
     * @param channelIdx Channel index to access
     * @return A proxy object for the specified channel-section pair
     */
    ChannelSectionProxy<T, Topology> channel(size_t channelIdx);

    /**
     * @brief Set the filter response type for all channels of this section.
     * @param newResponse Desired filter magnitude response type
     */
    void setResponse(Response newResponse) {
        filter.topology.design.setResponse(newResponse);
        for (size_t ch = 0; ch < filter.getNumChannels(); ++ch)
            filter.topology.applyDesignToEngine(ch, sectionIdx);
    }

    /**
     * @brief Set the filter gain for all channels of this section.
     * @param newGain Desired gain value.
     */
    void setGain(Gain<T> newGain) {
        filter.topology.design.setGain(newGain);
        for (size_t ch = 0; ch < filter.getNumChannels(); ++ch)
            filter.topology.applyDesignToEngine(ch, sectionIdx);
    }

    /**
     * @brief Set the filter frequency for all channels of this section.
     * @param newFreq Desired frequency value.
     */
    void setFrequency(Frequency<T> newFreq) {
        filter.topology.design.setFrequency(newFreq);
        for (size_t ch = 0; ch < filter.getNumChannels(); ++ch)
            filter.topology.applyDesignToEngine(ch, sectionIdx);
    }

    /**
     * @brief Set the filter Q factor for all channels of this section.
     * @param newQ Desired Q factor value.
     */
    void setQ(T newQ) {
        filter.topology.design.setQ(newQ);
        for (size_t ch = 0; ch < filter.getNumChannels(); ++ch)
            filter.topology.applyDesignToEngine(ch, sectionIdx);
    }

  private:
    FilterType& filter;
    size_t sectionIdx;
};

/**
 * @brief Proxy class for accessing a specific channel-section pair.
 * @tparam T Sample data type (e.g., float, double)
 * @tparam Topology Filter topology type (e.g., BiquadDF1)
 */
template <typename T, typename Topology>
class ChannelSectionProxy {
  public:
    /// Type aliases for easier access to response type and parent filter type.
    using Response = typename Topology::Design::Response;
    using FilterType = Filter<T, Topology>;

    /**
     * @brief Construct a ChannelSectionProxy for a specific channel and section index.
     * @param filter Reference to the parent filter
     * @param channelIdx Channel index to access
     * @param sectionIdx Section index to access
     * @note Asserts if channelIdx or sectionIdx are out of bounds based on the filter's number of channels and
     * sections.
     */
    ChannelSectionProxy(FilterType& filter, size_t channelIdx, size_t sectionIdx)
        : filter(filter), channelIdx(channelIdx), sectionIdx(sectionIdx) {
        assert(channelIdx < filter.getNumChannels() && "Channel index out of bounds");
        assert(sectionIdx < filter.getNumSections() && "Section index out of bounds");
    }

    /// Copy constructor.
    ChannelSectionProxy(const ChannelSectionProxy&) = default;

    /**
     * @brief Set the filter response type for specific channel and section.
     * @param newResponse Desired filter magnitude response type
     */
    void setResponse(Response newResponse) {
        filter.topology.design.setResponse(newResponse);
        filter.topology.applyDesignToEngine(channelIdx, sectionIdx);
    }

    /**
     * @brief Set the filter gain for specific channel and section.
     * @param newGain Desired gain value.
     */
    void setGain(Gain<T> newGain) {
        filter.topology.design.setGain(newGain);
        filter.topology.applyDesignToEngine(channelIdx, sectionIdx);
    }

    /**
     * @brief Set the filter frequency for specific channel and section.
     * @param newFreq Desired frequency value.
     */
    void setFrequency(Frequency<T> newFreq) {
        filter.topology.design.setFrequency(newFreq);
        filter.topology.applyDesignToEngine(channelIdx, sectionIdx);
    }
    /**
     * @brief Set the filter Q factor for specific channel and section.
     * @param newQ Desired Q factor value.
     */
    void setQ(T newQ) {
        filter.topology.design.setQ(newQ);
        filter.topology.applyDesignToEngine(channelIdx, sectionIdx);
    }

  private:
    FilterType& filter;
    size_t channelIdx;
    size_t sectionIdx;
};

/**
 * @brief Implementation of ChannelProxy::section() to return a ChannelSectionProxy for the specified section index.
 * @tparam T Sample data type (e.g., float, double)
 * @tparam Topology Filter topology type (e.g., BiquadDF1)
 * @param sectionIdx Section index to access
 * @return A ChannelSectionProxy for the specified channel and section index
 */
template <typename T, typename Topology>
ChannelSectionProxy<T, Topology> ChannelProxy<T, Topology>::section(size_t sectionIdx) {
    return ChannelSectionProxy<T, Topology>(filter, channelIdx, sectionIdx);
}

/**
 * @brief Implementation of SectionProxy::channel() to return a ChannelSectionProxy for the specified channel index.
 * @tparam T Sample data type (e.g., float, double)
 * @tparam Topology Filter topology type (e.g., BiquadDF1)
 * @param channelIdx Channel index to access
 * @return A ChannelSectionProxy for the specified channel and section index
 */
template <typename T, typename Topology>
ChannelSectionProxy<T, Topology> SectionProxy<T, Topology>::channel(size_t channelIdx) {
    return ChannelSectionProxy<T, Topology>(filter, channelIdx, sectionIdx);
}

} // namespace jnsc
