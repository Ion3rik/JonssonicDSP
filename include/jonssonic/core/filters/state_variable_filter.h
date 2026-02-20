// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// State Variable Filter class header file
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/core/filters/detail/svf_design.h"
#include "jonssonic/core/filters/detail/tpt_svf_topology.h"
#include "jonssonic/core/filters/routing.h"

namespace jnsc {
/**
 * @brief StateVariableFilter class for multi-channel, multi-section state variable filtering.
 * @tparam T Sample data type (e.g., float, double)
 * @tparam Topology Filter topology type (e.g., TPTSVFTopology<T>)
 * @tparam Design Filter design type (e.g., SVFDesign<T>)
 * @tparam RoutingType Filter routing type (Series or Parallel)
 */
template <typename T,
          typename Topology = detail::TPTSVFTopology<T>,
          typename Design = detail::SVFDesign<T>,
          Routing RoutingType = Routing::Series>
class StateVariableFilter {
  public:
    /// Type alias for response types from the design
    using Response = typename Design::Response;

    /// Default constructor
    StateVariableFilter() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     * @param newNumSections Number of second-order sections (default is 1)
     */
    StateVariableFilter(size_t newNumChannels, T newSampleRate, size_t newNumSections = 1) {
        prepare(newNumChannels, newSampleRate, newNumSections);
    }

    /// Default destructor
    ~StateVariableFilter() = default;

    /// No copy semantics nor move semantics
    StateVariableFilter(const StateVariableFilter&) = delete;
    const StateVariableFilter& operator=(const StateVariableFilter&) = delete;
    StateVariableFilter(StateVariableFilter&&) = delete;
    const StateVariableFilter& operator=(StateVariableFilter&&) = delete;

    /// Reset the filter state
    void reset() { topology.reset(); }

    /**
     * Prepare the filter engine for processing.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     * @param newNumSections Number of second-order sections (default is 1)
     * @note Prepares both the topology and design components of the engine.
     */
    void prepare(size_t newNumChannels, T newSampleRate, size_t newNumSections = 1) {
        topology.prepare(newNumChannels, newNumSections);
        design.prepare(newNumChannels, newSampleRate, newNumSections);
    }

    /**
     * @brief Process a single sample for a specific channel through all sections of the filter.
     * @param ch Channel index.
     * @param input Input sample.
     * @return Output sample.
     * @note Must call @ref prepare before processing.
     */
    T processSample(size_t ch, T input) {
        // Init output variables for multi-mode processing
        T output, hp, bp, lp, ap;

        // SERIES ROUTING
        if constexpr (RoutingType == Routing::Series) {
            output = input;
            for (size_t s = 0; s < topology.getNumSections(); ++s) {
                // Get current parameters for this channel and section
                T g = design.getNextG(ch, s);
                T twoR = design.getNextTwoR(ch, s);

                // Process single sample through the topology for this channel and section.
                topology.processSample(ch, s, output, g, twoR, hp, bp, lp);

                // Select output based on the design's response mask for this channel and section.
                const auto& responseMask = design.getResponseMask(ch, s);
                output = hp * responseMask[static_cast<size_t>(Response::Highpass)] +
                         bp * responseMask[static_cast<size_t>(Response::Bandpass)] +
                         lp * responseMask[static_cast<size_t>(Response::Lowpass)];
            }
        }

        // PARALLEL ROUTING
        if constexpr (RoutingType == Routing::Parallel) {
            for (size_t s = 0; s < topology.getNumSections(); ++s) {
                // Get current parameters for this channel and section
                T g = design.getNextG(ch, s);
                T twoR = design.getNextTwoR(ch, s);

                // Process single sample through the topology for this channel and section.
                topology.processSample(ch, s, input, g, twoR, hp, bp, lp);

                // Select output based on the design's response mask for this channel and section.
                const auto& responseMask = design.getResponseMask(ch, s);
                output += hp * responseMask[static_cast<size_t>(Response::Highpass)] +
                          bp * responseMask[static_cast<size_t>(Response::Bandpass)] +
                          lp * responseMask[static_cast<size_t>(Response::Lowpass)];
            }
        }
        return output;
    }

    /**
     * @brief Process a block of samples for all channels.
     * @param input input pointers for each channel [channel][sample].
     * @param output output pointers for each channel [channel][sample].
     * @param numSamples Number of samples in the block.
     * @note Must call @ref prepare before processing.
     */
    void processBlock(const T* const* input, T* const* output, size_t numSamples) {
        for (size_t ch = 0; ch < topology.getNumChannels(); ++ch)
            for (size_t n = 0; n < numSamples; ++n)
                output[ch][n] = processSample(ch, input[ch][n]);
    }

    /**
     * @brief Set the filter response type for all channels and sections.
     * @param newResponse Desired filter response type.
     */
    void setResponse(Response newResponse) {
        for (size_t ch = 0; ch < topology.getNumChannels(); ++ch) {
            for (size_t section = 0; section < topology.getNumSections(); ++section) {
                design.setResponse(ch, section, newResponse);
            }
        }
    }

    /**
     * @brief Set the filter frequency for all channels and sections.
     * @param newFreq Frequency struct.
     */
    void setFrequency(Frequency<T> newFreq) {
        for (size_t ch = 0; ch < topology.getNumChannels(); ++ch) {
            for (size_t section = 0; section < topology.getNumSections(); ++section) {
                design.setFrequency(ch, section, newFreq);
            }
        }
    }

    /**
     * @brief Set the quality factor (Q) for all channels and sections.
     * @param newQ Quality factor.
     */
    void setQ(T newQ) {
        for (size_t ch = 0; ch < topology.getNumChannels(); ++ch) {
            for (size_t section = 0; section < topology.getNumSections(); ++section) {
                design.setQ(ch, section, newQ);
            }
        }
    }

    /**
     * @brief Set the gain for all channels and sections.
     * @param newGain Gain struct.
     */
    void setGain(Gain<T> newGain) {
        for (size_t ch = 0; ch < topology.getNumChannels(); ++ch) {
            for (size_t section = 0; section < topology.getNumSections(); ++section) {
                design.setGain(ch, section, newGain);
            }
        }
    }

    /// Get reference to the topology for direct access (e.g., for testing)
    const Topology& getTopology() const { return topology; }
    /// Get reference to the design for direct access (e.g., for testing)
    const Design& getDesign() const { return design; }
    /// Check if the filter is prepared
    bool isPrepared() const { return topology.isPrepared(); }
    /// Get the number of channels
    size_t getNumChannels() const { return topology.getNumChannels(); }
    /// Get the sample rate from the design
    T getSampleRate() const { return design.getSampleRate(); }
    /// Get the number of sections
    size_t getNumSections() const { return topology.getNumSections(); }

    /// Forward declaration of proxy classes for channel and section parameter setting.
    class ChannelSectionProxy;

    ///  Proxy class for setting parameters on a specific channel across all sections.
    class ChannelProxy {
      public:
        ChannelProxy(StateVariableFilter& bqf, size_t ch) : bqf(bqf), ch(ch) {
            assert(ch < bqf.topology.getNumChannels() && "Channel index out of bounds");
        }

        // Access specific section of this channel
        ChannelSectionProxy section(size_t sectionIdx) { return ChannelSectionProxy(bqf, ch, sectionIdx); }

        void setResponse(Response newResponse) {
            for (size_t s = 0; s < bqf.topology.getNumSections(); ++s) {
                bqf.design.setResponse(ch, s, newResponse);
            }
        }

        void setFrequency(Frequency<T> newFreq) {
            for (size_t s = 0; s < bqf.topology.getNumSections(); ++s) {
                bqf.design.setFrequency(ch, s, newFreq);
            }
        }

        void setQ(T newQ) {
            for (size_t s = 0; s < bqf.topology.getNumSections(); ++s) {
                bqf.design.setQ(ch, s, newQ);
            }
        }

      private:
        StateVariableFilter& bqf;
        size_t ch;
    };

    /// Proxy class for setting parameters on a specific section across all channels.
    class SectionProxy {
      public:
        SectionProxy(StateVariableFilter& bqf, size_t section) : bqf(bqf), section(section) {}

        // Access specific channel of this section
        ChannelSectionProxy channel(size_t channelIdx) { return ChannelSectionProxy(bqf, channelIdx, section); }

        void setResponse(Response newResponse) {
            for (size_t ch = 0; ch < bqf.topology.getNumChannels(); ++ch) {
                bqf.design.setResponse(ch, section, newResponse);
            }
        }

        void setFrequency(Frequency<T> newFreq) {
            for (size_t ch = 0; ch < bqf.topology.getNumChannels(); ++ch) {
                bqf.design.setFrequency(ch, section, newFreq);
            }
        }

        void setQ(T newQ) {
            for (size_t ch = 0; ch < bqf.topology.getNumChannels(); ++ch) {
                bqf.design.setQ(ch, section, newQ);
            }
        }

      private:
        StateVariableFilter& bqf;
        size_t section;
    };

    /// Proxy class for setting parameters on a specific channel and section.
    class ChannelSectionProxy {
      public:
        ChannelSectionProxy(StateVariableFilter& bqf, size_t ch, size_t section) : bqf(bqf), ch(ch), section(section) {
            assert(ch < bqf.topology.getNumChannels() && "Channel index out of bounds");
            assert(section < bqf.topology.getNumSections() && "Section index out of bounds");
        }

        void setResponse(Response newResponse) { bqf.design.setResponse(ch, section, newResponse); }

        void setFrequency(Frequency<T> newFreq) { bqf.design.setFrequency(ch, section, newFreq); }

        void setQ(T newQ) { bqf.design.setQ(ch, section, newQ); }

      private:
        StateVariableFilter& bqf;
        size_t ch;
        size_t section;
    };

    /**
     * @brief Access a specific channel proxy for setting parameters across all sections of that channel.
     * @param ch Channel index.
     * @return ChannelProxy instance for the specified channel.
     */
    ChannelProxy channel(size_t ch) { return ChannelProxy(*this, ch); }

    /**
     * @brief Access a specific section proxy for setting parameters across all channels of that section.
     * @param section Section index.
     * @return SectionProxy instance for the specified section.
     */
    SectionProxy section(size_t section) { return SectionProxy(*this, section); }

  private:
    // Topology and design instances
    Topology topology;
    Design design;
};

} // namespace jnsc
