// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Biquad filter class combining design and topology for multi-channel, multi-section filtering
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/core/filters/detail/bilinear_biquad_design.h"
#include "jonssonic/core/filters/detail/df1_biquad_topology.h"
#include "jonssonic/core/filters/detail/df2t_biquad_topology.h"
#include "jonssonic/core/filters/routing.h"

namespace jnsc {
/**
 * @brief BiquadFilter class that combines filter design and topology to implement a multi-channel,
 * multi-section biquad filter.
 * @tparam T Sample data type (e.g., float, double)
 * @tparam Topology Filter topology type (e.g., DF2TBiquadTopology<T>)
 * @tparam Design Filter design type (e.g., BilinearBiquadDesign<T>)
 * @tparam RoutingType Filter routing type (Series or Parallel)
 */
template <typename T,
          typename Topology = detail::DF2TBiquadTopology<T>,
          typename Design = detail::BilinearBiquadDesign<T>,
          Routing RoutingType = Routing::Series>
class BiquadFilter {
  public:
    /// Type alias for response types from the design
    using Response = typename Design::Response;

    /// Default constructor
    BiquadFilter() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     * @param newNumSections Number of second-order sections (default is 1)
     */
    BiquadFilter(size_t newNumChannels, T newSampleRate, size_t newNumSections = 1) {
        prepare(newNumChannels, newSampleRate, newNumSections);
    }

    /// Default destructor
    ~BiquadFilter() = default;

    /// No copy semantics nor move semantics
    BiquadFilter(const BiquadFilter&) = delete;
    const BiquadFilter& operator=(const BiquadFilter&) = delete;
    BiquadFilter(BiquadFilter&&) = delete;
    const BiquadFilter& operator=(BiquadFilter&&) = delete;

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
        for (size_t ch = 0; ch < topology.getNumChannels(); ++ch) {
            for (size_t section = 0; section < topology.getNumSections(); ++section) {
                design.setResponse(ch, section, newResponse);
                applyDesignToTopology(ch, section);
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
                applyDesignToTopology(ch, section);
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
                applyDesignToTopology(ch, section);
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
                applyDesignToTopology(ch, section);
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
        ChannelProxy(BiquadFilter& bqf, size_t ch) : bqf(bqf), ch(ch) {
            assert(ch < bqf.topology.getNumChannels() && "Channel index out of bounds");
        }

        // Access specific section of this channel
        ChannelSectionProxy section(size_t sectionIdx) { return ChannelSectionProxy(bqf, ch, sectionIdx); }

        void setFrequency(Frequency<T> newFreq) {
            for (size_t s = 0; s < bqf.topology.getNumSections(); ++s) {
                bqf.design.setFrequency(ch, s, newFreq);
                bqf.applyDesignToTopology(ch, s);
            }
        }

        void setQ(T newQ) {
            for (size_t s = 0; s < bqf.topology.getNumSections(); ++s) {
                bqf.design.setQ(ch, s, newQ);
                bqf.applyDesignToTopology(ch, s);
            }
        }

        void setGain(Gain<T> newGain) {
            for (size_t s = 0; s < bqf.topology.getNumSections(); ++s) {
                bqf.design.setGain(ch, s, newGain);
                bqf.applyDesignToTopology(ch, s);
            }
        }

      private:
        BiquadFilter& bqf;
        size_t ch;
    };

    /// Proxy class for setting parameters on a specific section across all channels.
    class SectionProxy {
      public:
        SectionProxy(BiquadFilter& bqf, size_t section) : bqf(bqf), section(section) {}

        void setFrequency(Frequency<T> newFreq) {
            for (size_t ch = 0; ch < bqf.topology.getNumChannels(); ++ch) {
                bqf.design.setFrequency(ch, section, newFreq);
                bqf.applyDesignToTopology(ch, section);
            }
        }

        // Access specific channel of this section
        ChannelSectionProxy channel(size_t channelIdx) { return ChannelSectionProxy(bqf, channelIdx, section); }

        void setQ(T newQ) {
            for (size_t ch = 0; ch < bqf.topology.getNumChannels(); ++ch) {
                bqf.design.setQ(ch, section, newQ);
                bqf.applyDesignToTopology(ch, section);
            }
        }

        void setGain(Gain<T> newGain) {
            for (size_t ch = 0; ch < bqf.topology.getNumChannels(); ++ch) {
                bqf.design.setGain(ch, section, newGain);
                bqf.applyDesignToTopology(ch, section);
            }
        }

      private:
        BiquadFilter& bqf;
        size_t section;
    };

    /// Proxy class for setting parameters on a specific channel and section.
    class ChannelSectionProxy {
      public:
        ChannelSectionProxy(BiquadFilter& bqf, size_t ch, size_t section) : bqf(bqf), ch(ch), section(section) {
            assert(ch < bqf.topology.getNumChannels() && "Channel index out of bounds");
            assert(section < bqf.topology.getNumSections() && "Section index out of bounds");
        }

        void setFrequency(Frequency<T> newFreq) {
            bqf.design.setFrequency(ch, section, newFreq);
            bqf.applyDesignToTopology(ch, section);
        }

        void setQ(T newQ) {
            bqf.design.setQ(ch, section, newQ);
            bqf.applyDesignToTopology(ch, section);
        }

        void setGain(Gain<T> newGain) {
            bqf.design.setGain(ch, section, newGain);
            bqf.applyDesignToTopology(ch, section);
        }

      private:
        BiquadFilter& bqf;
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

    // Function to apply design coefficients to the topology for a specific channel and section
    void applyDesignToTopology(size_t ch, size_t section) {
        T b0, b1, b2, a1, a2;
        design.computeCoeffs(ch, section, b0, b1, b2, a1, a2);
        topology.setCoeffs(ch, section, b0, b1, b2, a1, a2);
    }
};

} // namespace jnsc
