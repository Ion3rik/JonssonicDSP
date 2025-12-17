// Jonssonic - A C++ audio DSP library
// Oscillator class header file
// SPDX-License-Identifier: MIT

#pragma once

#include <vector>
#include <cmath>
#include "../common/DspParam.h"

namespace Jonssonic
{

enum class Waveform
{
    Sine,
    Saw,
    Square,
    Triangle
};
/**
 * @brief Waveform generator class.
 * This class generates basic waveforms (sine, square, sawtooth, triangle) at a specified frequency.
 */
template<typename T,
    SmootherType SmootherType = SmootherType::OnePole,
    int SmootherOrder = 1,
    int SmoothingTimeMs = 10
>
class Oscillator
{
    public:
    Oscillator()
        : numChannels(0), sampleRate(T(0))
    {
    }
    ~Oscillator() = default;
    // No copy semantics nor move semantics
    Oscillator(const Oscillator&) = delete;
    const Oscillator& operator=(const Oscillator&) = delete;
    Oscillator(Oscillator&&) = delete;
    const Oscillator& operator=(Oscillator&&) = delete;

    void prepare(size_t newNumChannels, T newSampleRate)
    {
        // Update number of channels and sample rate
        numChannels = newNumChannels;
        sampleRate = newSampleRate;

        // Resize and initialize to zero
        phase.assign(numChannels, T(0));
        phaseIncrement.prepare(numChannels, sampleRate, SmoothingTimeMs);
    }

    // Reset phase for all channels
    void reset()
    {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            phase[ch] = T(0);
        }
    }

    // Reset phase for specific channel
    void reset(size_t channel)
    {
        phase[channel] = T(0);
    }

    // Process single sample for a specific channel (no phase modulation)
    T processSample(size_t ch)
    {
        // Generate waveform at current phase
        T output = generateWaveform(phase[ch]);
        
        // Advance and wrap phase using floor
        phase[ch] += phaseIncrement.getNextValue(ch);
        phase[ch] = phase[ch] - std::floor(phase[ch]);
        
        return output;
    }

    // Process single sample for a specific channel (with phase modulation)
    T processSample(size_t ch, T phaseMod)
    {    
        // Calculate modulated phase and wrap using floor
        T modulatedPhase = phase[ch] + phaseMod;
        modulatedPhase = modulatedPhase - std::floor(modulatedPhase);
        
        // Generate waveform at modulated phase
        T output = generateWaveform(modulatedPhase);

        // Advance and wrap current phase using floor
        phase[ch] += phaseIncrement.getNextValue(ch);
        phase[ch] = phase[ch] - std::floor(phase[ch]);
        
        return output;
    }

    // Process block for all channels (no phase modulation)
    void processBlock(T* const* output, size_t numSamples)
    {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t i = 0; i < numSamples; ++i) {
                // Generate waveform at current phase
                output[ch][i] = generateWaveform(phase[ch]);
                
                // Advance and wrap current phase using floor
                phase[ch] += phaseIncrement.getNextValue(ch);
                phase[ch] = phase[ch] - std::floor(phase[ch]);
            }
        }
    }

    // Process block for all channels (with phase modulation)
    void processBlock(T* const* output, const T* const* phaseMod, size_t numSamples)
    {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t i = 0; i < numSamples; ++i) {

                // Calculate and wrap modulated phase using floor
                T modulatedPhase = phase[ch] + phaseMod[ch][i];
                modulatedPhase = modulatedPhase - std::floor(modulatedPhase);
                
                // Generate waveform at modulated phase
                output[ch][i] = generateWaveform(modulatedPhase);
                
                // Advance and wrap current phase using floor
                phase[ch] += phaseIncrement.getNextValue(ch);
                phase[ch] = phase[ch] - std::floor(phase[ch]);
            }
        }
    }

    // Set all channels to same frequency
    void setFrequency(T freq, bool skipSmoothing = false) {
        phaseIncrement.setTarget(freq / sampleRate, skipSmoothing);
    }
    
    // Set specific channel frequency
    void setFrequency(size_t channel, T freq, bool skipSmoothing = false) {
        phaseIncrement.setTarget(channel, freq / sampleRate, skipSmoothing);
    }

    // Set waveform type
    void setWaveform(Waveform newWaveform) {
        waveform = newWaveform;
    }

    // Enable/disable anti-aliasing
    void setAntiAliasing(bool enable) {
        useAntiAliasing = enable;
    }

private:
    // Generate waveform sample at given phase (0.0 to 1.0)
    inline T generateWaveform(T phase) const
    {
        switch (waveform)
        {   
            case Waveform::Sine:
                // Sine wave
                return std::sin(T(2.0 * M_PI) * phase);
            
            case Waveform::Saw:
                // Sawtooth wave
                return T(2.0) * phase - T(1.0);
            
            case Waveform::Square:
                // Square wave
                return (phase < T(0.5)) ? T(-1.0) : T(1.0);
            
            case Waveform::Triangle:
                // Triangle wave
                return T(1.0) - std::abs(T(4.0) * phase - T(2.0));
            
            default:
                return T(0);
        }
    }

    T sampleRate = 44100.0;
    size_t numChannels;
    Waveform waveform = Waveform::Sine;
    bool useAntiAliasing = false;

    std::vector<T> phase;           // Phase per channel
    DspParam<T,SmootherType, SmootherOrder> phaseIncrement;     // Phase increment per channel
};
} // namespace Jonssonic