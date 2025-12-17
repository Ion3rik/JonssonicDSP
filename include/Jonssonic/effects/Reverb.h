// Jonssonic - A C++ audio DSP library
// Delay line class header file
// SPDX-License-Identifier: MIT

#pragma once
#include "../core/delays/DelayLine.h" 
#include "../core/common/DspParam.h"
#include "../core/filters/DampingFilter.h"
#include "../core/filters/BiquadFilter.h"
#include "../core/mixing/MixingMatrix.h"


namespace Jonssonic
{
/**
 * @brief Reverberation effect implmented with Feedback Delay Network (FDN)
 * @tparam T Sample data type (e.g., float, double)
 */

template<typename T, size_t FDN_SIZE = 8>
class Reverb
{
public:
    // Tunable constants
    /**
     * @brief Smoothing time for parameter changes in milliseconds.
     */
    static constexpr int SMOOTHING_TIME_MS = 100;   

    /**
     * @brief Maximum pre-delay time in milliseconds.
     */
    static constexpr T MAX_PRE_DELAY_MS = T(500.0); 
    
    /**
     * @brief Delay line minimum length scale factor.
     */
    static constexpr T MIN_DELAY_SCALE = T(0.2);

    /**
     * @brief Delay line maximum length scale factor.
     */
    static constexpr T MAX_DELAY_SCALE = T(2.0);

    /**
     * @brief Damping filter minimum cutoff frequency in Hz.
     */
    static constexpr T MIN_DAMPING_HZ = T(1000.0);

    /**
     * @brief Damping filter maximum cutoff frequency in Hz.
     */
    static constexpr T MAX_DAMPING_HZ = T(20000.0);           
    
    /**
     * @brief Coprime base delay lengths in milliseconds for the FDN.
     *        Actual lengths will be scaled based on size parameter.
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
        static constexpr int values[4] = {
            149, 211, 263, 293
        };
    };
    template <>
    struct FDNBaseDelays<8> {
        static constexpr int values[8] = {
            1493, 1789, 2131, 2467, 
            2927, 3253, 3697, 4211

        };
    };
    template <>
    struct FDNBaseDelays<16> {
        static constexpr int values[16] = {
            887,  1039,  1217,  1429,
            1667,  1951,  2281,  2663,
            3109,  3631,  4231,  4937,
            5779,  6761,  7907,  9241
        };
    };
    template <>
    struct FDNBaseDelays<32> {
        static constexpr int values[32] = {
            149, 211, 263, 293, 337, 379, 421, 463,
            509, 557, 601, 647, 691, 733, 787, 829,
            877, 919, 967, 1009, 1061, 1103, 1151, 1201,
            1237, 1291, 1327, 1381, 1423, 1459, 1511, 1543
        };
    };

    // Constructors and Destructor
    Reverb() = default;
    Reverb(size_t newNumChannels, T newSampleRate) {
        prepare(newNumChannels, newSampleRate);
    }
    ~Reverb() = default;

    // No copy or move semantics
    Reverb(const Reverb&) = delete;
    Reverb& operator=(const Reverb&) = delete;
    Reverb(Reverb&&) = delete;
    Reverb& operator=(Reverb&&) = delete;

    /**
     * @brief Prepare the reverb for processing.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     */
    void prepare(size_t newNumChannels, T newSampleRate)
    {
        // Store global parameters
        numChannels = newNumChannels;
        sampleRate = newSampleRate;

        // Compute maximum delay time for FDN delay lines
        T maxDelaySamples = T(0);
        for (size_t d = 0; d < FDN_SIZE; ++d)
            maxDelaySamples = std::max(maxDelaySamples, MAX_DELAY_SCALE * static_cast<T>(FDNBaseDelays<FDN_SIZE>::values[d]));

        // Prepare DSP components
        fdnDelays.prepare(FDN_SIZE, sampleRate, msToSamples(maxDelaySamples, sampleRate));
        preDelay.prepare(numChannels, sampleRate, MAX_PRE_DELAY_MS);

        dampingFilter.prepare(FDN_SIZE, sampleRate);

        lowCutFilter.prepare(numChannels, sampleRate);
        lowCutFilter.setType(BiquadType::Highpass);

        // Prepare mixing matrices and decay gains
        A.resize(FDN_SIZE); // Feedback matrix
        B.resize(numChannels, FDN_SIZE); // Input mixing matrix
        C.resize(FDN_SIZE, numChannels); // Output mixing matrix

        // Prepare state variables
        inputFrame.assign(numChannels, T(0));
        outputFrame.assign(numChannels, T(0));
        fdnInput.assign(FDN_SIZE, T(0));
        fdnState.assign(FDN_SIZE, T(0));

        // Initialize parameters to defaults
        setReverbTimeLowS(T(2.0), true);
        setReverbTimeHighS(T(1.0), true);
        setSize(T(0.5), true);
        setPreDelayTimeMs(T(0.0), true);
        setLowCutFreqHz(T(1000.0));


        togglePrepared = true;
    }

    /**
     * @brief Reset the effect state and clear buffers.
     */
    void reset()
    {
        fdnDelays.reset();
        preDelay.reset();
        dampingFilter.reset();
        lowCutFilter.reset();
        std::fill(inputFrame.begin(), inputFrame.end(), T(0));
        std::fill(outputFrame.begin(), outputFrame.end(), T(0));
        std::fill(fdnInput.begin(), fdnInput.end(), T(0));
        std::fill(fdnState.begin(), fdnState.end(), T(0));
    }

    /**
     * @brief Process a block of audio samples.
     * @param input Input sample pointers (one per channel)
     * @param output Output sample pointers (one per channel)
     * @param numSamples Number of samples to process
     */
    void processBlock(const T* const* input, T* const* output, size_t numSamples)
    {   
        
        // FDN PROCESSING LOOP
        for (size_t n = 0; n < numSamples; ++n) {
            // Gather input for this sample
            for (size_t ch = 0; ch < numChannels; ++ch)
                inputFrame[ch] = input[ch][n];

            // Input mixing: inputFrame (numChannels) -> fdnInput (FDN_SIZE)
            B.mix(inputFrame.data(), fdnInput.data());

            // Read FDN delay line outputs and apply damping into fdnState
            for (size_t d = 0; d < FDN_SIZE; ++d)
            {
                fdnState[d] = fdnDelays.readSample(d);
                fdnState[d] = dampingFilter.processSample(d, fdnState[d]);
            }

            // Feedback mixing: fdnState -> fdnFeedback
            A.mix(fdnState.data(), fdnState.data());
      
            // Write input + feedback scaled with decay gains to the delay lines
            for (size_t d = 0; d < FDN_SIZE; ++d)
                fdnDelays.writeSample(d, fdnInput[d] + fdnState[d]);

            // Output mixing: fdnState -> outputFrame (numChannels)
            C.mix(fdnState.data(), outputFrame.data());

            // Write to output buffers
            for (size_t ch = 0; ch < numChannels; ++ch)
                output[ch][n] = outputFrame[ch];
        }

        // Process pre-delay
        preDelay.processBlock(output, output, numSamples);

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
    void setReverbTimeLowS(T timeInSeconds, bool skipSmoothing = false)
    {
        // Set reverb time parameter
        T60_DC = std::clamp(timeInSeconds, T(0.1), T(20.0));
        updateFDNparams(skipSmoothing);
    }

    /**
     * @brief Set the reverb time at Nyquist in seconds.
     */
    void setReverbTimeHighS(T timeInSeconds, bool skipSmoothing = false)
    {
        // Set reverb time parameter
        T60_NYQ = std::clamp(timeInSeconds, T(0.1), T(20.0));
        updateFDNparams(skipSmoothing);
    }

    /**
     * @brief Set the size of the reverb space.
     *        In practise scales the FDN delay lengths.
     * @param sizeNormalized Size parameter normalized [0.0, 1.0]
     * @param skipSmoothing If true, skip smoothing of parameter change
     */
    void setSize(T sizeNormalized, bool skipSmoothing = false)
    {
        // Set size parameter
        reverbSize = std::clamp(sizeNormalized, T(0.0), T(1.0));
        updateFDNparams(skipSmoothing);
    }

    /**
     * @brief Set the pre delay time in milliseconds.
     * @param timeInMs Pre-delay time in milliseconds
     * @param skipSmoothing If true, skip smoothing of parameter change
     */
    void setPreDelayTimeMs(T timeInMs, bool skipSmoothing = false)
    {
        // Set pre-delay time parameter
        T clampedTime = std::clamp(timeInMs, T(0), MAX_PRE_DELAY_MS);
        preDelay.setDelayMs(clampedTime, skipSmoothing);
    }

    /**
     * @brief Set the low cut frequency for the reverb input.
     * @param freqHz Low cut frequency in Hz
     */
    void setLowCutFreqHz(T freqHz)
    {
        lowCutFilter.setFreq(freqHz);
    }


    //==============================================================================
    // GETTERS
    //==============================================================================
    /**
     * @brief Get the number of channels.
     */
    size_t getNumChannels() const { return numChannels; }

    /**
     * @brief Get the sample rate in Hz.
     */
    T getSampleRate() const { return sampleRate; }

    bool isPrepared() const { return togglePrepared; }


private:
    // Global parameters
    size_t numChannels = 0;
    T sampleRate = T(44100);
    bool togglePrepared = false;

    // Core processor components
    DelayLine<T, LinearInterpolator<T>, SmootherType::OnePole, 1, SMOOTHING_TIME_MS> fdnDelays; // FDN delay lines
    DelayLine<T, LinearInterpolator<T>, SmootherType::OnePole, 1, SMOOTHING_TIME_MS> preDelay; // Pre-delay line
    DampingFilter<T, DampingType::OnePole> dampingFilter; // Loop filter for damping
    BiquadFilter<T> lowCutFilter; // Highpass filter for low cut

    // FDN Matrices
    MixingMatrix<T, MixingMatrixType::Hadamard> A; // FDN feedback matrix
    MixingMatrix<T, MixingMatrixType::DecorrelatedSum> B; // Input mixing matrix
    MixingMatrix<T, MixingMatrixType::DecorrelatedSum> C; // Output mixing matrix

    // State variables
    std::vector<T> fdnState;        // FDN state (size FDN_SIZE)
    std::vector<T> fdnInput;        // FDN input (size FDN_SIZE)
    std::vector<T> inputFrame;      // input frame accross audio input channels
    std::vector<T> outputFrame;     // output frame accross audio output channels

    // User parameters
    T reverbSize;       // Size parameter normalized [0 .. 1]
    T T60_DC;           // Reverb time at DC in seconds
    T T60_NYQ;          // Reverb time at Nyquist in seconds

    /**
     * @brief Update FDN parameters based on size and reverb time.
     * @param skipSmoothing If true, skip smoothing of parameter changes
     * @note The smoothers of the delay line and the gains should use same smoothing
     *       to avoid artifacts.
     */
    void updateFDNparams(bool skipSmoothing = false)
    {
        if (!togglePrepared) return;
        // Map the size parameter to delay length scale (MIN_DELAY_SCALE .. MAX_DELAY_SCALE)
        T sizeScale =std::clamp(reverbSize * (MAX_DELAY_SCALE - MIN_DELAY_SCALE) + MIN_DELAY_SCALE, MIN_DELAY_SCALE, MAX_DELAY_SCALE);

        for (size_t d = 0; d < FDN_SIZE; ++d) {
            // Compute scaled delay length
            int delaySamples = static_cast<int>(FDNBaseDelays<FDN_SIZE>::values[d] * sizeScale);
            fdnDelays.setDelaySamples(d, delaySamples, skipSmoothing);

            // Update the damping filter based on delay length and desired reverb time at DC and Nyquist
            dampingFilter.setByT60(d, T60_DC, T60_NYQ, delaySamples);
        }
    }

};

} // namespace Jonssonic