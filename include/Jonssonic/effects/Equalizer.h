// Jonssonic - A C++ audio DSP library
// Equalizer effect class header file
// SPDX-License-Identifier: MIT

#pragma once
#include <jonssonic/core/filters/biquad_chain.h>
#include <jonssonic/core/nonlinear/wave_shaper.h>
#include <jonssonic/utils/buffer_utils.h>
#include <jonssonic/core/oversampling/oversampler.h>
#include <jonssonic/core/common/audio_buffer.h>

namespace jonssonic::effects {
/**
 * @brief Variable Q Equalizer with highpass, high and low mid peaks, and highshelf filters.
 * @param T Sample data type (e.g., float, double)
 */
template<typename T>
class Equalizer {
public:
    // TUNABLE CONSTANTS
    /**
     * @brief Weighting factor for variable Q calculation (higher = more Q change per dB).
     */
    static constexpr T VARIABLE_Q_WEIGHT = T(0.1);

    /**
     * @brief Base Q factor for mid bands. Controls bell curve base width.
     */
    static constexpr T BASE_Q = T(1.4);

    /**
     * @brief High shelf fixed cutoff frequency in Hz.
     */
    static constexpr size_t HIGH_SHELF_CUTOFF = T(5000);

    /**
     * @brief Operating gain for nonlinear EQ. Controls how hard the soft clipper works.
     */
    static constexpr T NONLINEAR_OPERATING_GAIN = T(0.5);

    /** 
     * @brief Makeup gain for nonlinear EQ. Compensates the operating gain reduction.
     */
    static constexpr T NONLINEAR_MAKEUP_GAIN = T(1.4142); // approx sqrt(2)

    /**
     * @brief Oversampling factor for nonlinear EQ.
     */
    static constexpr size_t OVERSAMPLING_FACTOR = 4;

    // Constructor and Destructor
    Equalizer() = default;
    Equalizer(size_t newNumChannels, size_t newMaxBlockSize, T newSampleRate) {
        prepare(newNumChannels, newMaxBlockSize, newSampleRate);
    }
    ~Equalizer() = default;

    // No copy or move semantics
    Equalizer(const Equalizer&) = delete;
    Equalizer& operator=(const Equalizer&) = delete;
    Equalizer(Equalizer&&) = delete;
    Equalizer& operator=(Equalizer&&) = delete;

    /**
     * @brief Prepare the Equalizer effect for processing.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     * @param maxDelayMs Maximum Equalizer in milliseconds
     */
    void prepare(size_t newNumChannels, size_t newMaxBlockSize, T newSampleRate) {

        // Store global parameters
        numChannels = newNumChannels;
        sampleRate = newSampleRate;

        // Prepare Equalizer chains
        prepareEqualizer(equalizerLinear, newNumChannels, newSampleRate);
        prepareEqualizer(equalizerNonlinear, newNumChannels, newSampleRate * OVERSAMPLING_FACTOR); // note: higher sample rate for nonlinear EQ

        // Prepare oversampler and oversampled buffer
        oversampler.prepare(newNumChannels, newMaxBlockSize);
        oversampledBuffer.resize(newNumChannels, newMaxBlockSize * OVERSAMPLING_FACTOR);

        togglePrepared = true;
    }

    void reset() {
        equalizerLinear.reset();
        equalizerNonlinear.reset();
    }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input buffer (numChannels x numSamples)
     * @param output Output buffer (numChannels x numSamples)
     * @param numSamples Number of samples to process
     */
    void processBlock(const T* const* input, T* const* output, size_t numSamples)
    {
        // Run Linear EQ
        if (!toggleSoftClipper) 
            equalizerLinear.processBlock(input, output, numSamples);
        
        // Run Nonlinear EQ with soft clipping
        else { 
            // Lower the operating gain for nonlinear processing
            applyGain<T>(output, numChannels, numSamples, NONLINEAR_OPERATING_GAIN); 

            // Upsample
            oversampler.upsample(input, oversampledBuffer.writePtrs(), numSamples);

            // Process nonlinear EQ at higher sample rate
            equalizerNonlinear.processBlock(oversampledBuffer.readPtrs(), 
                                            oversampledBuffer.writePtrs(), 
                                            numSamples * OVERSAMPLING_FACTOR); 

            // Apply soft clipping at higher sample rate
            outputSaturator.processBlock(oversampledBuffer.readPtrs(),
                                         oversampledBuffer.writePtrs(),
                                         numChannels, 
                                         numSamples * OVERSAMPLING_FACTOR); 
            // Downsample
            oversampler.downsample(oversampledBuffer.readPtrs(), output, numSamples); 

            // Apply makeup gain to compensate operating gain reduction
            applyGain<T>(output, numChannels, numSamples, NONLINEAR_MAKEUP_GAIN); 
        }

    }

    // SETTERS FOR PARAMETERS
    void setLowCutFreq(T newFreqHz, bool skipSmoothing) {
        equalizerLinear.setFreq(0, newFreqHz);
        equalizerNonlinear.setFreq(0, newFreqHz);
        
    }
    void setLowMidGainDb(T newGainDb, bool skipSmoothing) {
        equalizerLinear.setGainDb(1, newGainDb);
        equalizerNonlinear.setGainDb(1, newGainDb);

        T newQ = computeVariableQ(newGainDb);
        equalizerLinear.setQ(1, newQ);
        equalizerNonlinear.setQ(1, newQ);
        
    }
    void setHighMidGainDb(T newGainDb, bool skipSmoothing) {
        equalizerLinear.setGainDb(2, newGainDb);
        equalizerNonlinear.setGainDb(2, newGainDb);

        T newQ = computeVariableQ(newGainDb);
        equalizerLinear.setQ(2, newQ);
        equalizerNonlinear.setQ(2, newQ);
    }
    void setHighShelfGainDb(T newGainDb, bool skipSmoothing) {
        equalizerLinear.setGainDb(3, newGainDb);
        equalizerNonlinear.setGainDb(3, newGainDb);
    }
    void setLowMidFreq(T newFreqHz, bool skipSmoothing) {
        equalizerLinear.setFreq(1, newFreqHz);
        equalizerNonlinear.setFreq(1, newFreqHz);
    }
    void setHighMidFreq(T newFreqHz, bool skipSmoothing) {
        equalizerLinear.setFreq(2, newFreqHz);
        equalizerNonlinear.setFreq(2, newFreqHz);
    }

    void setSoftClipper(bool enabled) {
        toggleSoftClipper = enabled;
    }

    // GETTERS FOR GLOBAL PARAMETERS
    size_t getNumChannels() const {
        return numChannels;
    }
    T getSampleRate() const {
        return sampleRate;
    }
    bool isPrepared() const {
        return togglePrepared;
    }
    size_t getLatencySamples() const {
        if (toggleSoftClipper) {
            return oversampler.getLatencySamples(); // latency due to oversampling
        } else {
            return 0; // no latency in linear EQ
        }
    }

private:
    
    // GLOBAL PARAMETERS
    size_t numChannels = 0;
    T sampleRate = T(44100);
    bool togglePrepared = false;
    bool toggleSoftClipper = false;

    // PROCESSORS
    BiquadChain<T> equalizerLinear;
    BiquadChain<T, WaveShaperType::Tanh> equalizerNonlinear;
    WaveShaper<T, WaveShaperType::Atan> outputSaturator;
    Oversampler<T, OVERSAMPLING_FACTOR> oversampler; // oversampler for nonlinear EQ
    AudioBuffer<T> oversampledBuffer; // buffer for oversampling
    



    /**
     * @brief Compute variable Q factor based on gain in dB.
     * @param gainDb Gain in decibels
     * @return Computed Q factor
     */
    T computeVariableQ(T gainDb)
    {
        if (gainDb < 0)
            return BASE_Q * (T(1) + VARIABLE_Q_WEIGHT * std::abs(gainDb));
        else
            return BASE_Q / (T(1) + VARIABLE_Q_WEIGHT * gainDb);
    }

    /**
     * @brief Helper function to prepare Equalizer BiquadChain with default settings.
     * @param eqChain Reference to BiquadChain to prepare
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     */
    template<WaveShaperType shaper>
    void prepareEqualizer(BiquadChain<T, shaper>& eqChain, size_t newNumChannels, T newSampleRate)
    {
        // Prepare Filter Chain
        eqChain.prepare(newNumChannels, 4, newSampleRate); // 4 sections: LowCut, LowMid, HighMid, HighShelf
        eqChain.setType(0, BiquadType::Highpass); // LowCut
        eqChain.setType(1, BiquadType::Peak);     // LowMid
        eqChain.setType(2, BiquadType::Peak);     // HighMid
        eqChain.setType(3, BiquadType::Highshelf); // HighShelf

        // Fixed parameters
        eqChain.setFreq(3, HIGH_SHELF_CUTOFF); // HighShelf fixed freq
    }

};

} // namespace Jonssonic