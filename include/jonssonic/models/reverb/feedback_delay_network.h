// Jonssonic - A Modular Realtime C++ Audio DSP Library
// Feedback Delay Network header file
// SPDX-License-Identifier: MIT

#pragma once
#include <jonssonic/core/common/dsp_param.h>
#include <jonssonic/core/delays/delay_line.h>
#include <jonssonic/core/filters/biquad_filter.h>
#include <jonssonic/core/filters/damping_filter.h>
#include <jonssonic/core/mixing/mixing_matrix.h>
#include <jonssonic/models/generators/filtered_noise.h>

namespace jnsc::models {
/**
 * @brief Feedback Delay Network (FDN) for artificial reverberation and other feedback-based effects.
 * @tparam T Sample data type (e.g., float, double)
 * @tparam M Size of the Feedback Delay Network (supported sizes: 2, 4, 8, 16, 32)
 * @tparam FeedbackMatrixType Type of mixing matrix for feedback (default: Householder)
 * @tparam InputMatrixType Type of mixing matrix for input (default: DecorrelatedSum)
 * @tparam OutputMatrixType Type of mixing matrix for output (default: DecorrelatedSum)
 * @tparam DampingType Type of damping filter used in the feedback loop (default: FirstOrderShelf)
 * @tparam UseModulation If true, enable delay line modulation (default: true)
 * @tparam Interpolator Interpolator type for fractional delay support (default: LinearInterpolator)
 */

template <typename T,
          size_t M,
          MixingMatrixType FeedbackMatrixType = MixingMatrixType::Householder,
          MixingMatrixType InputMatrixType = MixingMatrixType::DecorrelatedSum,
          MixingMatrixType OutputMatrixType = MixingMatrixType::DecorrelatedSum,
          DampingType DampingType = DampingType::FirstOrderShelf,
          typename ModulationSourceType = FilteredNoise<T>,
          typename InterpolatorType = LinearInterpolator<T>>
class FeedbackDelayNetwork {
  public:
    /// Default constructor
    FeedbackDelayNetwork() = default;

    /**
     * @brief Parameterized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     *
     */
    FeedbackDelayNetwork(size_t newNumChannels, T newSampleRate, Time<T> newMaxDelay) {
        prepare(newNumChannels, newSampleRate, newMaxDelay);
    }

    /// Default destructor
    ~FeedbackDelayNetwork() = default;

    /// No copy nor move semantics
    FeedbackDelayNetwork(const FeedbackDelayNetwork&) = delete;
    FeedbackDelayNetwork& operator=(const FeedbackDelayNetwork&) = delete;
    FeedbackDelayNetwork(FeedbackDelayNetwork&&) = delete;
    FeedbackDelayNetwork& operator=(FeedbackDelayNetwork&&) = delete;

    /**
     * @brief Prepare the FeedbackDelayNetwork for processing.
     * @param newNumChannels Number of channels.
     * @param newSampleRate Sample rate in Hz.
     * @param newMaxDelay Maximum delay time struct.
     */
    void prepare(size_t newNumChannels, T newSampleRate, Time<T> newMaxDelay) {
        // Clamp and store config variables
        numChannels = utils::detail::clampChannels(newNumChannels);
        sampleRate = utils::detail::clampSampleRate(newSampleRate);

        // Prepare DSP components
        Dm.prepare(M, sampleRate, newMaxDelay);
        G.prepare(M, sampleRate);
        modSource.prepare(M, sampleRate);

        // Prepare mixing matrices and decay gains
        A.resize(M);              // Feedback matrix
        B.resize(numChannels, M); // Input mixing matrix
        C.resize(M, numChannels); // Output mixing matrix

        // Prepare state variables
        inputFrame.assign(numChannels, T(0));
        outputFrame.assign(numChannels, T(0));
        x.assign(M, T(0));
        s.assign(M, T(0));

        // Prepare parameters
        modDepthSamples.prepare(M, sampleRate);
        modDepthSamples.setBounds(T(0), T(1));

        togglePrepared = true;
    }

    /**
     * @brief Reset the FDN state and clear buffers.
     */
    void reset() {
        Dm.reset();
        G.reset();
        modSource.reset();
        modDepthSamples.reset();
        std::fill(inputFrame.begin(), inputFrame.end(), T(0));
        std::fill(outputFrame.begin(), outputFrame.end(), T(0));
        std::fill(x.begin(), x.end(), T(0));
        std::fill(s.begin(), s.end(), T(0));
    }

    /**
     * @brief Process block of samples with the FDN without modulation.
     * @param input Input sample pointers (one per channel)
     * @param output Output sample pointers (one per channel)
     * @param numSamples Number of samples to process
     */
    void processBlock(const T* const* input, T* const* output, size_t numSamples) {
        for (size_t n = 0; n < numSamples; ++n) {
            // Gather audio inputs for this sample
            for (size_t ch = 0; ch < numChannels; ++ch)
                inputFrame[ch] = input[ch][n];

            // Input mixing: inputFrame (numChannels) -> x (M)
            B.mix(inputFrame.data(), x.data());

            // Read FDN delay line outputs with modulation and apply damping
            for (size_t m = 0; m < M; ++m) {
                T scaledMod = modDepthSamples.getNextValue(m) * Dm.getTargetDelay(m).toSamples(sampleRate) *
                              modSource.processSample(m);
                s[m] = Dm.readSample(m, scaledMod);
                s[m] = G.processSample(m, s[m]);
            }

            // Feedback mixing: s -> s
            A.mix(s.data(), s.data());

            // Write input + feedback to delay lines
            for (size_t m = 0; m < M; ++m)
                Dm.writeSample(m, x[m] + s[m]);

            // Output mixing: s -> outputFrame (numChannels)
            C.mix(s.data(), outputFrame.data());

            // Write to audio outputs
            for (size_t ch = 0; ch < numChannels; ++ch)
                output[ch][n] = outputFrame[ch];
        }
    }
    //==============================================================================
    // SETTERS
    //==============================================================================

    /**
     * @brief Set control smoothing time.
     * @param time Smoothing time struct.
     */
    void setControlSmoothingTime(Time<T> time) { Dm.setControlSmoothingTime(time); }

    /**
     * @brief Set delay time for a specific delay line.
     * @param m Index of the delay line.
     * @param newDelayTime Delay time struct.
     * @param skipSmoothing If true, skip smoothing of parameter change
     * @note Index must be in range [0 .. FDN_SIZE-1].
     */
    void setDelay(size_t m, Time<T> newDelayTime, bool skipSmoothing = false) {
        assert(m < M && "Delay line index out of range");

        if (!togglePrepared)
            return;
        Dm.setDelay(m, newDelayTime, skipSmoothing);
        updateDampingFilter();
    }

    /**
     * @brief Set decay time at low frequencies.
     * @param newDecayTime Decay time struct.
     */
    void setDecayLow(Time<T> newDecayTime) {
        if (!togglePrepared)
            return;
        RT60_LO = newDecayTime;
        updateDampingFilter();
    }

    /**
     * @brief Set decay time at high frequencies.
     * @param newDecayTime Decay time struct.
     */
    void setDecayHigh(Time<T> newDecayTime) {
        if (!togglePrepared)
            return;
        RT60_HI = newDecayTime;
        updateDampingFilter();
    }

    /**
     * @brief Set crossover frequency for damping filter.
     * @param newCrossOverFreq Crossover frequency struct.
     * @note Only applicable for FirstOrderShelf and BiquadShelf damping types.
     */
    void setDampingCrossoverFreq(Frequency<T> newCrossOverFreq) {
        if (!togglePrepared)
            return;
        Fc = newCrossOverFreq;
        updateDampingFilter();
    }

    /**
     * @brief Set relative modulation depth as a fraction of delay times.
     * @param newDepth New relative modulation depth [0..1]
     * @param skipSmoothing If true, skip smoothing of parameter change
     * @note The modulation signal in the processBlock is multiplied by (depth * delayTime) for each delay line.
     */
    void setRelativeModulationDepth(T newDepth, bool skipSmoothing = false) {
        if (!togglePrepared)
            return;
        for (size_t m = 0; m < M; ++m)
            modDepthSamples.setTarget(m, newDepth * Dm.getTargetDelay(m).toSamples(sampleRate), skipSmoothing);
    }

    /**
     * @brief Set modulation source cutoff frequency (if applicable).
     * @param newCutoffFreq New cutoff frequency struct.
     * @note Only valid for FilteredNoise modulation source.
     */
    void setNoiseModulationCutoff(Frequency<T> newCutoffFreq) {
        static_assert(std::is_same<ModulationSourceType, FilteredNoise<T>>::value,
                      "setModulationCutoffFreq is only valid when using FilteredNoise as ModulationSourceType");
        if (!togglePrepared)
            return;
        modSource.setCutoff(newCutoffFreq);
    }

    /// Get number of channels
    size_t getNumChannels() const { return numChannels; }

    /// Get sample rate
    T getSampleRate() const { return sampleRate; }

    /// Check if prepared
    bool isPrepared() const { return togglePrepared; }

  private:
    // Config variables
    size_t numChannels = 0;
    T sampleRate = T(44100);
    bool togglePrepared = false;

    // Processor components
    DelayLine<T, InterpolatorType> Dm;
    DampingFilter<T, DampingType> G;
    ModulationSourceType modSource;

    // FDN Matrices
    MixingMatrix<T, FeedbackMatrixType> A;
    MixingMatrix<T, InputMatrixType> B;
    MixingMatrix<T, OutputMatrixType> C;

    // State variables
    std::vector<T> s;           // FDN state across delay lines (FDN_SIZE)
    std::vector<T> x;           // FDN input across delay lines (FDN_SIZE)
    std::vector<T> inputFrame;  // input frame accross audio input channels
    std::vector<T> outputFrame; // output frame accross audio output channels

    // Parameters
    DspParam<T> modDepthSamples;                    // Modulation depth in samples per delay line
    Time<T> RT60_LO = Time<T>::Seconds(T(1.0));     // Reverb time at low frequencies
    Time<T> RT60_HI = Time<T>::Seconds(T(1.0));     // Reverb time at high frequencies
    Frequency<T> Fc = Frequency<T>::Hertz(T(2000)); // Crossover frequency for damping filter

    // Helper method
    void updateDampingFilter() {
        if constexpr (DampingType == DampingType::FirstOrderShelf || DampingType == DampingType::BiquadShelf)
            for (size_t m = 0; m < M; ++m)
                G.setByT60(m, Fc, RT60_LO, RT60_HI, Dm.getTargetDelay(m));

        if constexpr (DampingType == DampingType::OnePole)
            for (size_t m = 0; m < M; ++m)
                G.setByT60(m, RT60_LO, RT60_HI, Dm.getTargetDelay(m));
    }
};

} // namespace jnsc::models