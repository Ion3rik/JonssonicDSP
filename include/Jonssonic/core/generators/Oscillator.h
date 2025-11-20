// Jonssonic - A C++ audio DSP library
// Waveform generator class header file
// Author: Jon Fagerström
// Update: 18.11.2025

#pragma once

#include <vector>
#include <cmath>

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
template<typename T>
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
        phaseIncrement.assign(numChannels, T(0));
        frequency.assign(numChannels, T(0));
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

    // Set all channels to same frequency
    void setFrequency(T freq) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            frequency[ch] = freq;
            phaseIncrement[ch] = freq / sampleRate;
        }
    }
    
        // Set specific channel frequency
    void setFrequency(T freq, size_t channel) {
        frequency[channel] = freq;
        phaseIncrement[channel] = freq / sampleRate;
    }

    // Set waveform type
    void setWaveform(Waveform wf) {
        waveform = wf;
    }

    // Enable/disable anti-aliasing
    void setAntiAliasing(bool enable) {
        useAntiAliasing = enable;
    }

    // Process single sample for a specific channel (no phase modulation)
    T processSample(size_t ch)
    {
        // Generate waveform at current phase
        T output = generateWaveform(phase[ch]);
        
        // Advance phase
        phase[ch] += phaseIncrement[ch];
        if (phase[ch] >= T(1.0)) 
            phase[ch] -= T(1.0);
        
        return output;
    }

    // Process single sample for a specific channel (with phase modulation)
    T processSample(size_t ch, T phaseMod)
    {
        // Calculate modulated phase
        T modulatedPhase = phase[ch] + phaseMod;
        
        // Wrap phase to [0, 1)
        while (modulatedPhase >= T(1.0)) modulatedPhase -= T(1.0);
        while (modulatedPhase < T(0.0)) modulatedPhase += T(1.0);
        
        // Generate waveform at modulated phase
        T output = generateWaveform(modulatedPhase);
        
        // Advance phase
        phase[ch] += phaseIncrement[ch];
        if (phase[ch] >= T(1.0)) 
            phase[ch] -= T(1.0);
        
        return output;
    }

    // Process block for all channels (no phase modulation)
    void processBlock(T* const* output, size_t numSamples)
    {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t i = 0; i < numSamples; ++i) {
                // Generate waveform at current phase
                output[ch][i] = generateWaveform(phase[ch]);
                
                // Advance phase
                phase[ch] += phaseIncrement[ch];
                if (phase[ch] >= T(1.0)) 
                    phase[ch] -= T(1.0);
            }
        }
    }

    // Process block for all channels (with phase modulation)
    void processBlock(T* const* output, const T* const* phaseMod, size_t numSamples)
    {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t i = 0; i < numSamples; ++i) {
                // Calculate modulated phase
                T modulatedPhase = phase[ch] + phaseMod[ch][i];
                
                // Wrap phase to [0, 1)
                while (modulatedPhase >= T(1.0)) modulatedPhase -= T(1.0);
                while (modulatedPhase < T(0.0)) modulatedPhase += T(1.0);
                
                // Generate waveform at modulated phase
                output[ch][i] = generateWaveform(modulatedPhase);
                
                // Advance phase
                phase[ch] += phaseIncrement[ch];
                if (phase[ch] >= T(1.0)) 
                    phase[ch] -= T(1.0);
            }
        }
    }

private:
    // Generate waveform sample at given phase (0.0 to 1.0)
    inline T generateWaveform(T phase) const
    {
        switch (waveform)
        {
            case Waveform::Sine:
                return std::sin(T(2.0 * M_PI) * phase);
            
            case Waveform::Saw:
                // Naive sawtooth
                return T(2.0) * phase - T(1.0);
            
            case Waveform::Square:
                // Naive square
                return (phase < T(0.5)) ? T(-1.0) : T(1.0);
            
            case Waveform::Triangle:
                // Naive triangle
                if (phase < T(0.5))
                    return T(4.0) * phase - T(1.0);
                else
                    return T(-4.0) * phase + T(3.0);
            
            default:
                return T(0);
        }
    }

    T sampleRate = 44100.0;
    size_t numChannels;
    Waveform waveform = Waveform::Sine;
    bool useAntiAliasing = false;

    std::vector<T> phase;          // Phase per channel
    std::vector<T> phaseIncrement; // Phase increment per channel
    std::vector<T> frequency;      // Frequency per channel
};
} // namespace Jonssonic