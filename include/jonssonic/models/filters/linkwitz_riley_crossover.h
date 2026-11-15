// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// LinkwitzRileyCrossover class header file
// SPDX-License-Identifier: MIT

#pragma once
#include <array>
#include <cassert>
#include <jonssonic/core/filters/biquad_filter.h>

namespace jnsc::models {
/**
 * @brief Two-band, second-order Linkwitz-Riley crossover (12 dB/octave).
 *
 * Uses independent lowpass and highpass sections of one biquad filter,
 * with Q = 0.5 at a shared frequency.
 * The high output is polarity-inverted so that low + high has unity magnitude.
 * The sum is allpass, not an unchanged copy of the input waveform.
 * @tparam T Sample data type (e.g., float, double).
 */
template <typename T>
class LinkwitzRileyCrossover {
  public:
    /// Default constructor.
    LinkwitzRileyCrossover() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels.
     * @param newSampleRate Sample rate in Hz.
     */
    LinkwitzRileyCrossover(size_t newNumChannels, T newSampleRate) { prepare(newNumChannels, newSampleRate); }

    /// Default destructor.
    ~LinkwitzRileyCrossover() = default;

    /// No copy nor move semantics.
    LinkwitzRileyCrossover(const LinkwitzRileyCrossover&) = delete;
    LinkwitzRileyCrossover& operator=(const LinkwitzRileyCrossover&) = delete;
    LinkwitzRileyCrossover(LinkwitzRileyCrossover&&) = delete;
    LinkwitzRileyCrossover& operator=(LinkwitzRileyCrossover&&) = delete;

    /**
     * @brief Prepare both branches and clear their state.
     * @param newNumChannels Number of channels, clamped to the library limits.
     * @param newSampleRate Sample rate in Hz, clamped to the library limits.
     * @note Allocates all processing state and initializes the crossover to 1000 Hz.
     */
    void prepare(size_t newNumChannels, T newSampleRate) {
        utils::detail::requireSupportedType<T>();
        using Response = typename BiquadFilter<T>::Response;
        filter.prepare(newNumChannels, newSampleRate, 2);
        filter.section(0).setResponse(Response::Lowpass);
        filter.section(1).setResponse(Response::Highpass);
        filter.setQ(T(0.5));
        setFrequency(Frequency<T>::Hertz(T(1000)));
        reset();
    }

    /// Clear both branches' state without changing the crossover frequency.
    void reset() { filter.reset(); }

    /**
     * @brief Set the shared crossover frequency for all channels.
     * @param newFrequency Frequency with units.
     * @note Must call @ref prepare first. Uses the biquad limits of
     * 0.0001 to 0.45 times the sample rate. Updates immediately without smoothing.
     */
    void setFrequency(Frequency<T> newFrequency) {
        assert(isPrepared() && "Crossover must be prepared before setting frequency");
        filter.setFrequency(newFrequency);
    }

    /**
     * @brief Split one sample into low and polarity-corrected high outputs.
     * @param ch Channel index.
     * @param input Input sample.
     * @return Outputs in the order {low, high}. Add them directly to recombine.
     * @note Must call @ref prepare before processing.
     */
    std::array<T, 2> processSample(size_t ch, T input) {
        std::array<T, 2> outputs;
        filter.processSample(ch, input, outputs.data());
        outputs[1] = -outputs[1];
        return outputs;
    }

    /**
     * @brief Split a block for all channels.
     * @param input Input pointers, one per channel.
     * @param lowOutput Low-band output pointers, one per channel.
     * @param highOutput Polarity-corrected high-band output pointers, one per channel.
     * @param numSamples Number of samples per channel.
     * @note Must call @ref prepare first. Input may alias either output for the
     * same channel; the two output buffers must not overlap. Zero-length blocks
     * may use null pointers.
     */
    void processBlock(const T* const* input, T* const* lowOutput, T* const* highOutput, size_t numSamples) {
        T* const* const outputs[] = {lowOutput, highOutput};
        filter.processBlock(input, outputs, numSamples);
        for (size_t ch = 0; ch < getNumChannels(); ++ch)
            for (size_t n = 0; n < numSamples; ++n)
                highOutput[ch][n] = -highOutput[ch][n];
    }

    /// Check whether both branches are prepared.
    bool isPrepared() const { return filter.isPrepared(); }
    /// Get the number of channels.
    size_t getNumChannels() const { return filter.getNumChannels(); }
    /// Get the sample rate in Hz.
    T getSampleRate() const { return filter.getSampleRate(); }
    /// Get the clamped crossover frequency.
    Frequency<T> getFrequency() const { return filter.getDesign().getFrequency(); }

  private:
    Parallel<BiquadFilter<T>> filter;
};

} // namespace jnsc::models
