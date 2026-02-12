// Jonssonic - A Modular Realtime C++ Audio DSP Library
// Reverb effect class header file
// SPDX-License-Identifier: MIT

#pragma once
#include <jonssonic/core/filters/biquad_filter.h>
#include <jonssonic/models/generators/filtered_noise.h>
#include <jonssonic/models/reverb/feedback_delay_network.h>

namespace jnsc::effects {
/**
 * @brief Reverberation effect implmented with a Feedback Delay Network (FDN)
 * @tparam T Sample data type (e.g., float, double)
 */

template <typename T>
class Reverb {
    /**
     * @brief Tunable constants
     * @param FDN_SIZE Size of the Feedback Delay Network
     * @param SMOOTHING_TIME_MS Time in milliseconds for parameter smoothing
     * @param MAX_PRE_DELAY_MS Maximum pre-delay time in milliseconds
     * @param MIN_DELAY_SCALE Minimum scaling factor for FDN base delays
     * @param MAX_DELAY_SCALE Maximum scaling factor for FDN base delays
     * @param MAX_RELATIVE_MODULATION_DEPTH Maximum relative modulation depth for delay lines
     */
    static constexpr size_t FDN_SIZE = 16;
    static constexpr int SMOOTHING_TIME_MS = 50;
    static constexpr T MAX_PRE_DELAY_MS = T(200.0);
    static constexpr T MIN_DELAY_SCALE = T(0.9);
    static constexpr T MAX_DELAY_SCALE = T(3.0);
    static constexpr T MAX_RELATIVE_MODULATION_DEPTH = T(0.1);

    /**
     * @brief Coprime base delay lengths in samples for the FDN.
     *        Actual lengths will be scaled based on diffusion parameter.
     */
    template <size_t N>
    struct FDNBaseDelays;

    template <>
    struct FDNBaseDelays<2> {
        static constexpr int values[2] = {
            1, 2 // ONLY FOR TESTING PURPOSES
        };
    };

    template <>
    struct FDNBaseDelays<4> {
        static constexpr int values[4] = {443, 601, 809, 1031};
    };
    template <>
    struct FDNBaseDelays<8> {
        static constexpr int values[8] = {1493, 1789, 2131, 2467, 2927, 3253, 3697, 4211

        };
    };
    template <>
    struct FDNBaseDelays<16> {
        static constexpr int values[16] = {
            1601, 547, 2371, 947, 3187, 503, 1231, 2749, 587, 2053, 3677, 829, 1423, 631, 1069, 1823

        };
    };
    template <>
    struct FDNBaseDelays<32> {
        static constexpr int values[32] = {673,  809,  887,  1039, 1217, 1429, 1667, 1951, 2027, 2089, 2137,
                                           2203, 2269, 2339, 2411, 2477, 2539, 2593, 2657, 2719, 2789, 2851,
                                           2909, 2971, 3109, 3631, 4231, 4937, 5779, 6761, 7907, 9241};
    };

  public:
    /// Default constructor.
    Reverb() = default;
    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     */
    Reverb(size_t newNumChannels, T newSampleRate) { prepare(newNumChannels, newSampleRate); }

    /// Default destructor.
    ~Reverb() = default;

    /// No copy nor move semantics
    Reverb(const Reverb&) = delete;
    Reverb& operator=(const Reverb&) = delete;
    Reverb(Reverb&&) = delete;
    Reverb& operator=(Reverb&&) = delete;

    /**
     * @brief Prepare the reverb for processing.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     */
    void prepare(size_t newNumChannels, T newSampleRate) {
        // Store global parameters
        numChannels = utils::detail::clampChannels(newNumChannels);
        sampleRate = utils::detail::clampSampleRate(newSampleRate);

        // Compute maximum delay time for FDN delay lines
        T maxDelaySamples = T(0);
        for (size_t d = 0; d < FDN_SIZE; ++d)
            maxDelaySamples =
                std::max(maxDelaySamples, MAX_DELAY_SCALE * static_cast<T>(FDNBaseDelays<FDN_SIZE>::values[d]));

        // Prepare DSP components
        preDelay.prepare(numChannels, sampleRate, Time<T>::Milliseconds(MAX_PRE_DELAY_MS));
        fdn.prepare(numChannels, sampleRate, Time<T>::Samples(maxDelaySamples));
        lowCutFilter.prepare(numChannels, sampleRate);
        lowCutFilter.setResponse(BiquadFilter<T>::Response::Highpass);

        // Set fixed parameters
        fdn.setControlSmoothingTime(Time<T>::Milliseconds(SMOOTHING_TIME_MS));

        // Initialize parameters to defaults
        setReverbTimeLowS(T(2.0), true);
        setReverbTimeHighS(T(1.0), true);
        setDiffusion(T(0.5), true);
        setPreDelayTimeMs(T(0.0), true);
        setLowCutFreqHz(T(1000.0));
        setModulationRateHz(T(1.0));
        setModulationDepth(T(0.1));
    }

    /// Reset the reverb state.
    void reset() {
        fdn.reset();
        preDelay.reset();
        lowCutFilter.reset();
    }

    /**
     * @brief Process a block of audio samples.
     * @param input Input sample pointers (one per channel)
     * @param output Output sample pointers (one per channel)
     * @param numSamples Number of samples to process
     */
    void processBlock(const T* const* input, T* const* output, size_t numSamples) {

        // Process pre-delay
        preDelay.processBlock(input, output, numSamples);

        // Process FDN
        fdn.processBlock(output, output, numSamples);

        // Process highpass filter for low cut
        lowCutFilter.processBlock(output, output, numSamples);
    }

    //==============================================================================
    // SETTERS
    //==============================================================================

    /**
     * @brief Set the reverb time at DC in seconds.
     * @param timeInSeconds Reverb time in seconds
     * @param skipSmoothing If true, skip smoothing of parameter change
     */
    void setReverbTimeLowS(T timeInSeconds, bool skipSmoothing = false) {
        fdn.setDecayLow(Time<T>::Seconds(timeInSeconds));
    }

    /**
     * @brief Set the reverb time at crossover frequency.
     * @param crossOverFreqHz Crossover frequency in Hz.
     */
    void setDampingCrossoverFreqHz(T crossOverFreqHz) {
        fdn.setDampingCrossoverFreq(Frequency<T>::Hertz(crossOverFreqHz));
    }

    /**
     * @brief Set the reverb time at Nyquist in seconds.
     */
    void setReverbTimeHighS(T timeInSeconds, bool skipSmoothing = false) {
        fdn.setDecayHigh(Time<T>::Seconds(timeInSeconds));
    }

    /**
     * @brief Set the diffusion amount.
     *        In practise scales the FDN delay lengths.
     *        High diffusion -> shorter delays, Low diffusion -> longer delays
     * @param diffusionAmount Diffusion amount [0.0, 1.0]
     * @param skipSmoothing If true, skip smoothing of parameter change
     */
    void setDiffusion(T diffusionAmount, bool skipSmoothing = false) {
        // Map the diffusion parameter to delay length scale (MIN_DELAY_SCALE .. MAX_DELAY_SCALE)
        T scaledDiffusion = std::clamp((1 - diffusionAmount) * (MAX_DELAY_SCALE - MIN_DELAY_SCALE) + MIN_DELAY_SCALE,
                                       MIN_DELAY_SCALE,
                                       MAX_DELAY_SCALE);

        for (size_t m = 0; m < FDN_SIZE; ++m) {
            // Compute scaled delay lengths
            size_t delaySamples = static_cast<size_t>(FDNBaseDelays<FDN_SIZE>::values[m] * scaledDiffusion);
            fdn.setDelay(m, Time<T>::Samples(delaySamples), skipSmoothing);
        }
    }

    /**
     * @brief Set the pre delay time in milliseconds.
     * @param timeInMs Pre-delay time in milliseconds
     * @param skipSmoothing If true, skip smoothing of parameter change
     */
    void setPreDelayTimeMs(T timeInMs, bool skipSmoothing = false) {
        T clampedTime = std::clamp(timeInMs, T(0), MAX_PRE_DELAY_MS);
        preDelay.setDelay(Time<T>::Milliseconds(clampedTime), skipSmoothing);
    }

    /**
     * @brief Set the low cut frequency.
     * @param freqHz Low cut frequency in Hz.
     */
    void setLowCutFreqHz(T freqHz) { lowCutFilter.setFrequency(Frequency<T>::Hertz(freqHz)); }

    /**
     * @brief Set the modulation rate in Hz.
     * @param modRateHz Modulation rate in Hz.
     */
    void setModulationRateHz(T modRateHz) { fdn.setNoiseModulationCutoff(Frequency<T>::Hertz(modRateHz)); }

    /**
     * @brief Set the modulation depth as a fraction of the maximum depth.
     * @param modDepth Modulation depth [0.0, 1.0]
     */
    void setModulationDepth(T modDepth) {
        T clampedDepth = std::clamp(modDepth, T(0), T(1));
        fdn.setRelativeModulationDepth(clampedDepth * MAX_RELATIVE_MODULATION_DEPTH);
    }

    //==============================================================================
    // GETTERS
    //==============================================================================
    /// Get number of channels
    size_t getNumChannels() const { return numChannels; }

    /// Get sample rate
    T getSampleRate() const { return sampleRate; }

  private:
    // Global parameters
    size_t numChannels = 0;
    T sampleRate = T(44100);

    // Core processor components
    DelayLine<T, LinearInterpolator<T>> preDelay;
    models::FeedbackDelayNetwork<T,
                                 FDN_SIZE,
                                 MixingMatrixType::Householder,
                                 MixingMatrixType::DecorrelatedSum,
                                 MixingMatrixType::DecorrelatedSum,
                                 models::Shelf1Decay<T>,
                                 models::FilteredNoise<T>,
                                 LinearInterpolator<T>>
        fdn;
    BiquadFilter<T> lowCutFilter;
};

} // namespace jnsc::effects