// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Direct Form I biquad filter engine header file
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/core/filters/detail/df1_topology.h"
#include "jonssonic/core/filters/detail/df2t_topology.h"
#include "jonssonic/core/filters/detail/parametric_biquad_design.h"

namespace jnsc::detail {
/// Routing enum for filter output selection
enum class Routing { Series, Parallel };

/**
 * @brief ParametricBiquadEngine class that combines filter design and topology to implement a multi-channel,
 * multi-section biquad filter.
 * @tparam T Sample data type (e.g., float, double)
 * @tparam Topology Filter topology type (e.g., DF1Topology, DF2TTopology)
 * @tparam RoutingType Filter routing type (Series or Parallel)
 */
template <typename T, typename Topology = DF2TTopology<T>, Routing RoutingType = Routing::Series>
class ParametricBiquadEngine {
  public:
    /// Type alias for the design component
    using Design = detail::ParametricBiquadDesign<T>;

    /// Type alias for response types from the design
    using Response = typename Design::Response;

    /**
     * Prepare the filter engine for processing.
     * @param newNumChannels Number of channels
     * @param newNumSections Number of second-order sections
     * @param newSampleRate Sample rate in Hz
     * @note Prepares both the topology and design components of the engine.
     */
    void prepare(size_t newNumChannels, size_t newNumSections, T newSampleRate) {
        topology.prepare(newNumChannels, newNumSections);
        design.prepare(newSampleRate);
    }

    /// Reset the filter state
    void reset() { topology.reset(); }

    /**
     * @brief Process a single sample for a specific channel through all sections of the filter.
     * @param ch Channel index.
     * @param input Input sample.
     * @return Output sample.
     * @note Must call @ref prepare before processing.
     */
    float processSample(size_t ch, T input) {
        // SERIES ROUTING
        if constexpr (RoutingType == Routing::Series) {
            T output = input;
            for (size_t s = 0; s < topology.getNumSections(); ++s) {
                output = topology.processSample(ch, s, output);
            }
            return output;
        }
        // PARALLEL ROUTING
        if constexpr (RoutingType == Routing::Parallel) {
            T output = T(0);
            for (size_t s = 0; s < topology.getNumSections(); ++s) {
                output += topology.processSample(ch, s, input);
            }
            return output;
        }
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
        design.setResponse(newResponse);
        for (size_t ch = 0; ch < topology.getNumChannels(); ++ch)
            for (size_t section = 0; section < topology.getNumSections(); ++section)
                applyDesignToTopology(ch, section);
    }

    /**
     * @brief Set the filter frequency for all channels and sections.
     * @param newFreq Frequency struct.
     */
    void setFrequency(Frequency<T> newFreq) {
        design.setFrequency(newFreq);
        for (size_t ch = 0; ch < topology.getNumChannels(); ++ch)
            for (size_t section = 0; section < topology.getNumSections(); ++section)
                applyDesignToTopology(ch, section);
    }

    /**
     * @brief Set the quality factor (Q) for all channels and sections.
     * @param newQ Quality factor.
     */
    void setQ(T newQ) {
        design.setQ(newQ);
        for (size_t ch = 0; ch < topology.getNumChannels(); ++ch)
            for (size_t section = 0; section < topology.getNumSections(); ++section)
                applyDesignToTopology(ch, section);
    }

    /**
     * @brief Set the gain for all channels and sections.
     * @param newGain Gain struct.
     */
    void setGain(Gain<T> newGain) {
        design.setGain(newGain);
        for (size_t ch = 0; ch < topology.getNumChannels(); ++ch)
            for (size_t section = 0; section < topology.getNumSections(); ++section)
                applyDesignToTopology(ch, section);
    }

    /// Get reference to the topology for direct access (e.g., for testing)
    const Topology& getTopology() const { return topology; }
    /// Get reference to the design for direct access (e.g., for testing)
    const Design& getDesign() const { return design; }

    class ChannelSectionProxy;

    /// @brief Proxy class for setting parameters on a specific channel across all sections.
    class ChannelProxy {
      public:
        ChannelProxy(ParametricBiquadEngine& eng, size_t ch) : eng(eng), ch(ch) {
            assert(ch < eng.topology.getNumChannels() && "Channel index out of bounds");
        }

        // Access specific section of this channel
        ChannelSectionProxy section(size_t sectionIdx) { return ChannelSectionProxy(eng, ch, sectionIdx); }

        void setFrequency(Frequency<T> newFreq) {
            eng.design.setFrequency(newFreq);
            for (size_t s = 0; s < eng.topology.getNumSections(); ++s)
                eng.applyDesignToTopology(ch, s);
        }

        void setQ(T newQ) {
            eng.design.setQ(newQ);
            for (size_t s = 0; s < eng.topology.getNumSections(); ++s)
                eng.applyDesignToTopology(ch, s);
        }

        void setGain(Gain<T> newGain) {
            eng.design.setGain(newGain);
            for (size_t s = 0; s < eng.topology.getNumSections(); ++s)
                eng.applyDesignToTopology(ch, s);
        }

      private:
        ParametricBiquadEngine& eng;
        size_t ch;
    };

    /// Proxy class for setting parameters on a specific section across all channels.
    class SectionProxy {
      public:
        SectionProxy(ParametricBiquadEngine& eng, size_t section) : eng(eng), section(section) {}

        void setFrequency(Frequency<T> newFreq) {
            eng.design.setFrequency(newFreq);
            for (size_t ch = 0; ch < eng.topology.getNumChannels(); ++ch)
                eng.applyDesignToTopology(ch, section);
        }

        // Access specific channel of this section
        ChannelSectionProxy channel(size_t channelIdx) { return ChannelSectionProxy(eng, channelIdx, section); }

        void setQ(T newQ) {
            eng.design.setQ(newQ);
            for (size_t ch = 0; ch < eng.topology.getNumChannels(); ++ch)
                eng.applyDesignToTopology(ch, section);
        }

        void setGain(Gain<T> newGain) {
            eng.design.setGain(newGain);
            for (size_t ch = 0; ch < eng.topology.getNumChannels(); ++ch)
                eng.applyDesignToTopology(ch, section);
        }

      private:
        ParametricBiquadEngine& eng;
        size_t section;
    };

    /// Proxy class for setting parameters on a specific channel and section.
    class ChannelSectionProxy {
      public:
        ChannelSectionProxy(ParametricBiquadEngine& eng, size_t ch, size_t section)
            : eng(eng), ch(ch), section(section) {
            assert(ch < eng.topology.getNumChannels() && "Channel index out of bounds");
            assert(section < eng.topology.getNumSections() && "Section index out of bounds");
        }

        void setFrequency(Frequency<T> newFreq) {
            eng.design.setFrequency(newFreq);
            eng.applyDesignToTopology(ch, section);
        }

        void setQ(T newQ) {
            eng.design.setQ(newQ);
            eng.applyDesignToTopology(ch, section);
        }

        void setGain(Gain<T> newGain) {
            eng.design.setGain(newGain);
            eng.applyDesignToTopology(ch, section);
        }

      private:
        ParametricBiquadEngine& eng;
        size_t ch;
        size_t section;
    };

    /// Access a specific channel proxy
    ChannelProxy channel(size_t ch) { return ChannelProxy(*this, ch); }
    /// Access a specific section proxy
    SectionProxy section(size_t section) { return SectionProxy(*this, section); }

  private:
    // Topology and design instances
    Topology topology;
    Design design;

    // Function to apply design coefficients to the topology for a specific channel and section
    void applyDesignToTopology(size_t ch, size_t section) {
        topology.setCoeffs(ch, section, design.getB0(), design.getB1(), design.getB2(), design.getA1(), design.getA2());
    }
};

} // namespace jnsc::detail
