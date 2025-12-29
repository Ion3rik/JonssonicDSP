// Jonssonic - A C++ audio DSP library
// Compressor effect header file
// SPDX-License-Identifier: MIT

#pragma once
#include "../core/dynamics/EnvelopeFollower.h"
#include "../core/dynamics/GainComputer.h"
#include "../core/dynamics/GainSmoother.h"

#include "../core/nonlinear/WaveShaperProcessor.h"
#include "../core/oversampling/Oversampler.h"
#include "../core//common/AudioBuffer.h"
#include "../utils/MathUtils.h"
#include "../core/mixing/DryWetMixer.h"
#include "../core/common/SmoothedValue.h"
#include "../utils/BufferUtils.h"

namespace Jonssonic::effects {
/**
 * @brief Compressor effect with threshold, attack, release, ratio, mix and output gain.
 *        Additional character mode which adds mild harmonic distortion and modifies the gain smoother.
 * @param T Sample data type (e.g., float, double)
 */
template<typename T>
class Compressor {
public:
    // TUNABLE CONSTANTS
    /**
     * @brief Oversampling factor
     */
    static constexpr size_t OVERSAMPLING_FACTOR = 4;

    /**
     * @brief Smoothing time in milliseconds for parameter changes.
     */
    static constexpr T PARAM_SMOOTH_TIME_MS = T(50);

    /**
     * @brief Base drive gain for character mode saturation.
     */
    static constexpr T DRIVE_BASE_GAIN_DB = T(6.0);

    /**
     * @brief Maximum drive gain for character mode saturation.
     */
    static constexpr T DRIVE_MAX_GAIN_DB = T(24.0);

    /**
     * @brief Drive scaling factor for character mode saturation.
     */
    static constexpr T DRIVE_SCALE_FACTOR = T(0.4);

    /**
     * @brief Mode dependent smoother attack ranges in ms.
     */
    static constexpr T NORMAL_SMOOTHER_ATTACK_MIN_MS = T(0.1);
    static constexpr T NORMAL_SMOOTHER_ATTACK_MAX_MS = T(30.0);
    static constexpr T CHARACTER_SMOOTHER_ATTACK_MIN_MS = T(3.0);
    static constexpr T CHARACTER_SMOOTHER_ATTACK_MAX_MS = T(60.0);

    /**
     * @brief Mode dependent smoother release ranges in ms.
     */
    static constexpr T NORMAL_SMOOTHER_RELEASE_MIN_MS = T(30.0);
    static constexpr T NORMAL_SMOOTHER_RELEASE_MAX_MS = T(400.0);
    static constexpr T CHARACTER_SMOOTHER_RELEASE_MIN_MS = T(100.0);
    static constexpr T CHARACTER_SMOOTHER_RELEASE_MAX_MS = T(1500.0);

    /**
     * @brief Mode dependent envelope attack times.
     */
    static constexpr T NORMAL_ENVELOPE_ATTACK_MS = T(0.1);
    static constexpr T CHARACTER_ENVELOPE_ATTACK_MS = T(10.0);

    /**
     * @brief Mode dependent envelope release times.
     */
    static constexpr T NORMAL_ENVELOPE_RELEASE_MS = T(10.0);
    static constexpr T CHARACTER_ENVELOPE_RELEASE_MS = T(100.0);

    /**
     * @brief Mode dependent Knee widths in dB
     */
    static constexpr T CHARACTER_MODE_KNEE_DB = T(10.0);
    static constexpr T NORMAL_MODE_KNEE_DB = T(1.0);

    // Constructor and Destructor
    Compressor() = default;
    Compressor(size_t newNumChannels, size_t newMaxBlockSize, T newSampleRate) {
        prepare(newNumChannels, newMaxBlockSize, newSampleRate);
    }
    ~Compressor() = default;

    // No copy or move semantics
    Compressor(const Compressor&) = delete;
    Compressor& operator=(const Compressor&) = delete;
    Compressor(Compressor&&) = delete;
    Compressor& operator=(Compressor&&) = delete;

    /**
     * @brief Prepare the Compressor effect for processing.
     * @param newNumChannels Number of channels
     * @param newMaxBlockSize Maximum block size for processing (at base sample rate)
     * @param newSampleRate Sample rate in Hz
     */
    void prepare(size_t newNumChannels, size_t newMaxBlockSize, T newSampleRate) {

        // Store global parameters
        numChannels = newNumChannels;
        sampleRate = newSampleRate;

        // Prepare compressor components
        peakFollower.prepare(newNumChannels, newSampleRate, PARAM_SMOOTH_TIME_MS);
        rmsFollower.prepare(newNumChannels, newSampleRate, PARAM_SMOOTH_TIME_MS);
        gainComputer.prepare(newNumChannels, newSampleRate, PARAM_SMOOTH_TIME_MS);
        gainSmoother.prepare(newNumChannels, newSampleRate, PARAM_SMOOTH_TIME_MS);

        shaper.prepare(newNumChannels, newSampleRate, PARAM_SMOOTH_TIME_MS);
  
        // Prepare dry/wet mixer with latency compensation and output gain
        size_t oversamplerLatencySamples = oversampler.getLatencySamples(); 
        dryWetMixer.prepare(newNumChannels, newSampleRate, PARAM_SMOOTH_TIME_MS, oversamplerLatencySamples);
        outputGain.prepare(newNumChannels, newSampleRate, PARAM_SMOOTH_TIME_MS);

        // Prepare internal buffers and oversampler
        fxBuffer.resize(newNumChannels, newMaxBlockSize);
        oversampler.prepare(newNumChannels, newMaxBlockSize);
        oversampledBuffer.resize(newNumChannels, newMaxBlockSize * OVERSAMPLING_FACTOR);

        // Set Default Parameters
        setThreshold(T(-24.0), true);  // -24 dB
        setAttack(T(0.5), true);      // 50 % relative
        setRelease(T(0.5), true);    // 50 % relative
        setRatio(T(4.0), true);        // 4:1
        setMix(T(1.0), true);          // 100% wet
        setOutputGain(T(1.0), true);   // 0 dB
        setCharacterMode(false);       // Character mode off

        // Set fixed envelope follower times 
        peakFollower.setAttackTime(NORMAL_ENVELOPE_ATTACK_MS, true);
        peakFollower.setReleaseTime(NORMAL_ENVELOPE_RELEASE_MS, true);
        rmsFollower.setAttackTime(CHARACTER_ENVELOPE_ATTACK_MS, true);
        rmsFollower.setReleaseTime(CHARACTER_ENVELOPE_RELEASE_MS, true);

        // Mark as prepared
        togglePrepared = true;
    }

    void reset() {
        peakFollower.reset();
        rmsFollower.reset();
        gainComputer.reset();
        gainSmoother.reset();
        shaper.reset();
        oversampler.reset();
        dryWetMixer.reset();
        outputGain.reset();
        fxBuffer.clear();
        oversampledBuffer.clear();
    }

    /**
     * @brief Process a block of samples for all channels.
     * @param input Input buffer (numChannels x numSamples)
     * @param output Output buffer (numChannels x numSamples)
     * @param numSamples Number of samples to process
     */
    void processBlock(const T* const* input, T* const* output, size_t numSamples)
    {   
        // Copy input to fxBuffer for processing
        Jonssonic::copyToBuffer<T>(input, fxBuffer.writePtrs(), numChannels, numSamples);
        
        // CHARACTER MODE PROCESSING
        if (toggleCharacterMode) {
            // Upsample
            size_t numOversampledSamples = oversampler.upsample(fxBuffer.readPtrs(), oversampledBuffer.writePtrs(), numSamples);

            // Process waveshaper
            shaper.processBlock(oversampledBuffer.readPtrs(), oversampledBuffer.writePtrs(), numOversampledSamples);

            // Downsample
            oversampler.downsample(oversampledBuffer.readPtrs(), fxBuffer.writePtrs(), numSamples);
        } 
        // NORMAL MODE PROCESSING
        T characterModeMask = static_cast<T>(toggleCharacterMode); // check per block if character mode is enabled (1.0 or 0.0)
        T minGain = T(1); // track for metering
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t n = 0; n < numSamples; ++n) {

                // Compute envelope 
                T envValue = characterModeMask * rmsFollower.processSample(ch, fxBuffer[ch][n])
                             + (static_cast<T>(1) - characterModeMask) * peakFollower.processSample(ch, fxBuffer[ch][n]);
                
                // Compute target gain from compressor curve
                T targetGainDb = gainComputer.processSample(ch, envValue);
                
                // Smooth gain
                T smoothedGainLinear = gainSmoother.processSample(ch, targetGainDb);
                minGain = std::min(smoothedGainLinear, minGain); // track min gain for metering

                // Apply gain to sample
                fxBuffer[ch][n] = fxBuffer[ch][n] * smoothedGainLinear;
            }
        }

        // Apply dry/wet mixing (with delay compensation if oversampling)
        size_t dryDelaySamples = static_cast<size_t>(characterModeMask) * oversampler.getLatencySamples();
        dryWetMixer.processBlock(input, fxBuffer.readPtrs(), output, numSamples, dryDelaySamples);

        // Apply output gain
        outputGain.applyToBuffer(output, numSamples);

        // Update gain reduction metering value
        gainReduction.store(minGain);
    }

    // SETTERS FOR PARAMETERS

    /** 
     * @brief Set threshold in dB.
    */
    void setThreshold(T newthresholdDb, bool skipSmoothing = false) {
        gainComputer.setThreshold(newthresholdDb, skipSmoothing);

        // Update drive gain for character mode based on threshold
        T driveGainDb = DRIVE_BASE_GAIN_DB -newthresholdDb * DRIVE_SCALE_FACTOR; // drive increases as threshold decreases
        driveGainDb = std::clamp(driveGainDb, T(0), DRIVE_MAX_GAIN_DB); // clamp (0 dB to max)
        shaper.setInputGain(dB2Mag(driveGainDb), skipSmoothing); // set input gain for waveshaper in linear
        T driveCompensation = T(1) / std::sqrt(dB2Mag(driveGainDb)); // approximate compensation to retain overall level
        shaper.setOutputGain(driveCompensation, skipSmoothing); // set output gain to retain overall level
    }

    /**
     * @brief Set attack time normalized (0.0 to 1.0).
     */
    void setAttack(T attackTimeNorm, bool skipSmoothing = false) {

        // Map to actual time in ms based on mode
        T attackTimeMs = T(0);
        toggleCharacterMode ?
        attackTimeMs = mapTime(attackTimeNorm, CHARACTER_SMOOTHER_ATTACK_MIN_MS, CHARACTER_SMOOTHER_ATTACK_MAX_MS) :
        attackTimeMs = mapTime(attackTimeNorm, NORMAL_SMOOTHER_ATTACK_MIN_MS, NORMAL_SMOOTHER_ATTACK_MAX_MS);
        // Gain smoother attack
        gainSmoother.setAttackTime(attackTimeMs, skipSmoothing);
    }

    /**
     * @brief Set release time in milliseconds.
     */
    void setRelease(T releaseTimeMs, bool skipSmoothing = false) {
        // Map to actual time in ms based on mode
        T mappedReleaseTimeMs = T(0);
        toggleCharacterMode ?
        mappedReleaseTimeMs = mapTime(releaseTimeMs, CHARACTER_SMOOTHER_RELEASE_MIN_MS, CHARACTER_SMOOTHER_RELEASE_MAX_MS) :
        mappedReleaseTimeMs = mapTime(releaseTimeMs, NORMAL_SMOOTHER_RELEASE_MIN_MS, NORMAL_SMOOTHER_RELEASE_MAX_MS);
        // Gain smoother release
        gainSmoother.setReleaseTime(mappedReleaseTimeMs, skipSmoothing);
    }

    /**
     * @brief Set compression ratio.
     */
    void setRatio(T newRatio, bool skipSmoothing = false) {
        gainComputer.setRatio(newRatio, skipSmoothing);
    }

    
    /**
     * @brief Set mix between dry and wet signal.
     * @param mixNormalized Mix amount in normalized range [0, 1] (0 = full dry, 1 = full wet).
     */
    void setMix(T mixNormalized, bool skipSmoothing) {
        dryWetMixer.setMix(mixNormalized, skipSmoothing);
    }

    /**
     * @brief Set output gain in dB.
     */
    void setOutputGain(T gainDb, bool skipSmoothing = false) {
        T gainLinear = dB2Mag(gainDb); // Convert dB to linear
        outputGain.setTarget(gainLinear, skipSmoothing);
    }

    /**
     * @brief Enable or disable character mode.
     */
    void setCharacterMode(bool newMode, bool skipSmoothing = false) {
        toggleCharacterMode = newMode;
        toggleCharacterMode 
            ? gainComputer.setKnee(CHARACTER_MODE_KNEE_DB, skipSmoothing) // softer knee for character mode
            : gainComputer.setKnee(NORMAL_MODE_KNEE_DB, skipSmoothing); // harder knee for normal mode
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
        if (toggleCharacterMode) {
            return oversampler.getLatencySamples(); // latency due to oversampling
        } else {
            return 0; // otherwise no latency
        }
    }

    // GETTER FOR METERING
    T getGainReduction() const {
        return gainReduction.load();
    }

private:
    
    // GLOBAL PARAMETERS
    size_t numChannels = 0;
    T sampleRate = T(44100);
    bool togglePrepared = false;
    bool toggleCharacterMode = false;

    // GLOBAL COMPONENTS
    core::GainComputer<T, core::GainType::Compressor> gainComputer; // Gain computer for compression curve
    core::GainSmoother<T, core::GainSmootherType::AttackRelease> gainSmoother; // Gain smoother for smooth gain changes
    DryWetMixer<T> dryWetMixer; // dry/wet mixer
    SmoothedValue<float> outputGain; // Smoothed output gain parameter

    // NORMAL MODE COMPONENTS
    core::EnvelopeFollower<T, core::EnvelopeType::Peak> peakFollower; // Envelope follower for normal mode

    // CHARACTER MODE COMPONENTS
    core::EnvelopeFollower<T, core::EnvelopeType::RMS> rmsFollower; // Envelope follower for the character mode
    WaveShaperProcessor<T, WaveShaperType::Tanh> shaper; // Waveshaper for character mode
    Oversampler<T, OVERSAMPLING_FACTOR> oversampler; // oversampler for character mode
    AudioBuffer<T> oversampledBuffer; // buffer for oversampling
    AudioBuffer<T> fxBuffer; // buffer for the effect processing

    // Metering variable
    std::atomic<T> gainReduction { T(1) };

    /**
     * @brief Map normalized attack time [0.0, 1.0] exponentially to time range [min, max].
     */
    T mapTime(T attackTimeNorm, T min, T max) const {
        attackTimeNorm = std::clamp(attackTimeNorm, T(0), T(1)); // safety clamp
        T attackTime = T(0);
        attackTime = min * std::pow((max / min), attackTimeNorm); // exponential mapping
        return attackTime;
    }

};

} // namespace Jonssonic::effects