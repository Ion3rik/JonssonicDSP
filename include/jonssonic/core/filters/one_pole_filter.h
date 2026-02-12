// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Biquad filter class combining design and topology for multi-channel, multi-section filtering
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/core/filters/detail/filter_limits.h"
#include "jonssonic/utils/detail/config_utils.h"
#include <jonssonic/core/filters/detail/bilinear_one_pole_design.h>
#include <jonssonic/core/filters/detail/df1_one_pole_topology.h>
#include <jonssonic/core/filters/routing.h>

namespace jnsc {
/**
 * @brief OnePoleFilter class that combines filter design and topology to implement a multi-channel,
 * multi-section one-pole filter.
 * @tparam T Sample data type (e.g., float, double)
 * @tparam Topology Filter topology type (e.g., DF1OnePoleTopology<T>)
 * @tparam Design Filter design type (e.g., BilinearOnePoleDesign<T>)
 * @tparam RoutingType Filter routing type (Series or Parallel)
 */
template <typename T,
          typename Topology = detail::DF1OnePoleTopology<T>,
          typename Design = detail::BilinearOnePoleDesign<T>,
          Routing RoutingType = Routing::Series>
class OnePoleFilter {
  public:
    /// Type alias for response types from the design
    using Response = typename Design::Response;

    /// Default constructor
    OnePoleFilter() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     * @param newNumSections Number of second-order sections (default is 1)
     */
    OnePoleFilter(size_t newNumChannels, T newSampleRate, size_t newNumSections = 1) {
        prepare(newNumChannels, newSampleRate, newNumSections);
    }

    /// Default destructor
    ~OnePoleFilter() = default;

    /// No copy semantics nor move semantics
    OnePoleFilter(const OnePoleFilter&) = delete;
    const OnePoleFilter& operator=(const OnePoleFilter&) = delete;
    OnePoleFilter(OnePoleFilter&&) = delete;
    const OnePoleFilter& operator=(OnePoleFilter&&) = delete;

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
        // Clamp sample rate, number of channels, and sections to allowed limits
        sampleRate = utils::detail::clampSampleRate(newSampleRate);
        numChannels = utils::detail::clampChannels(newNumChannels);
        numSections = detail::FilterLimits<T>::clampSections(newNumSections);

        // Prepare topology and design with the new configuration
        topology.prepare(numChannels, numSections);
        design.prepare(numChannels, sampleRate, numSections);
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
    bool isPrepared() const { return topology.isPrepared() && design.isPrepared(); }
    /// Get the number of channels
    size_t getNumChannels() const { return numChannels; }
    /// Get the sample rate from the design
    T getSampleRate() const { return sampleRate; }
    /// Get the number of sections
    size_t getNumSections() const { return numSections; }

    /// Forward declaration of proxy classes for channel and section parameter setting.
    class ChannelSectionProxy;

    ///  Proxy class for setting parameters on a specific channel across all sections.
    class ChannelProxy {
      public:
        ChannelProxy(OnePoleFilter& opf, size_t ch) : opf(opf), ch(ch) {
            assert(ch < opf.getNumChannels() && "Channel index out of bounds");
        }

        // Access specific section of this channel
        ChannelSectionProxy section(size_t sectionIdx) { return ChannelSectionProxy(opf, ch, sectionIdx); }

        /// Set frequency for this channel across all sections.
        void setFrequency(Frequency<T> newFreq) {
            for (size_t s = 0; s < opf.topology.getNumSections(); ++s) {
                opf.design.setFrequency(ch, s, newFreq);
                opf.applyDesignToTopology(ch, s);
            }
        }

        /// Set gain for this channel across all sections.
        void setGain(Gain<T> newGain) {
            for (size_t s = 0; s < opf.topology.getNumSections(); ++s) {
                opf.design.setGain(ch, s, newGain);
                opf.applyDesignToTopology(ch, s);
            }
        }

      private:
        OnePoleFilter& opf;
        size_t ch;
    };

    /// Proxy class for setting parameters on a specific section across all channels.
    class SectionProxy {
      public:
        SectionProxy(OnePoleFilter& opf, size_t section) : opf(opf), section(section) {}

        // Access specific channel of this section.
        ChannelSectionProxy channel(size_t channelIdx) { return ChannelSectionProxy(opf, channelIdx, section); }

        // Set frequency for this section across all channels.
        void setFrequency(Frequency<T> newFreq) {
            for (size_t ch = 0; ch < opf.topology.getNumChannels(); ++ch) {
                opf.design.setFrequency(ch, section, newFreq);
                opf.applyDesignToTopology(ch, section);
            }
        }

        // Set gain for this section across all channels.
        void setGain(Gain<T> newGain) {
            for (size_t ch = 0; ch < opf.topology.getNumChannels(); ++ch) {
                opf.design.setGain(ch, section, newGain);
                opf.applyDesignToTopology(ch, section);
            }
        }

      private:
        OnePoleFilter& opf;
        size_t section;
    };

    /// Proxy class for setting parameters on a specific channel and section.
    class ChannelSectionProxy {
      public:
        ChannelSectionProxy(OnePoleFilter& opf, size_t ch, size_t section) : opf(opf), ch(ch), section(section) {
            assert(ch < opf.getNumChannels() && "Channel index out of bounds");
            assert(section < opf.getNumSections() && "Section index out of bounds");
        }

        /// Set frequency for this specific channel and section.
        void setFrequency(Frequency<T> newFreq) {
            opf.design.setFrequency(ch, section, newFreq);
            opf.applyDesignToTopology(ch, section);
        }

        /// Set gain for this specific channel and section.
        void setGain(Gain<T> newGain) {
            opf.design.setGain(ch, section, newGain);
            opf.applyDesignToTopology(ch, section);
        }

      private:
        OnePoleFilter& opf;
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
    // Config variables
    T sampleRate = T(44100);
    size_t numChannels = 0;
    size_t numSections = 0;

    // Topology and design instances
    Topology topology;
    Design design;

    // Function to apply design coefficients to the topology for a specific channel and section
    void applyDesignToTopology(size_t ch, size_t section) {
        assert(ch < numChannels && "Channel index out of bounds");
        assert(section < numSections && "Section index out of bounds");
        T b0, b1, a1;
        design.computeCoeffs(ch, section, b0, b1, a1);
        topology.setCoeffs(ch, section, b0, b1, a1);
    }
};

} // namespace jnsc
