// Jonssonic - A C++ audio DSP library
// Noise generator class header file
// SPDX-License-Identifier: MIT

#pragma once

#include <vector>
#include <cmath>
#include "../common/DspParam.h"

namespace Jonssonic
{

enum class NoiseType
{
    Gaussian,
    Uniform
    // TO DO:
    //TotallyRandom,
    //AdditiveRandom,
    //Velvet
};

// =============================================================================
// TEMPLATE CLASS DEFINITION
// =============================================================================
template<typename T, NoiseType Type = NoiseType::Gaussian>
class Noise;

// =============================================================================
// TEMPLATE SPECIALIZATION FOR GAUSSIAN NOISE
// =============================================================================
template<typename T>
class Noise<T, NoiseType::Gaussian>
{
public:
    // Constructors and Destructor
    Noise() = default;
    Noise(size_t newNumChannels) {
        prepare(newNumChannels);
    }
    ~Noise() = default;

    // No copy semantics nor move semantics
    Noise(const Noise&) = delete;
    const Noise& operator=(const Noise&) = delete;
    Noise(Noise&&) = delete;
    const Noise& operator=(Noise&&) = delete;

    /**
     * @brief Prepare the noise generator for processing.
     * @param newNumChannels Number of channels
     */
    void prepare(size_t newNumChannels)
    {
        numChannels = newNumChannels;
        rngs.resize(numChannels);
        for (size_t ch = 0; ch < numChannels; ++ch) {
            std::random_device rd;
            rngs[ch].seed(rd());
        }
    }

    /**
     * @brief Reseed the random number generators for all channels
     */
    void reset()
    {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            std::random_device rd;
            rngs[ch].seed(rd());
        }
    }

    /**
     * @brief Generate single sample for a specific channel 
     */
    T processSample(size_t ch)
    {
        // Gaussian white noise sample
        return T(distribution(rngs[ch]));
    }


    /**
     * @brief Generate block of samples for all channels 
     */
    void processBlock(T* const* output, size_t numSamples)
    {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t n = 0; n < numSamples; ++n) {
                output[ch][n] = processSample(ch);
            }
        }
    }
    

private:
    size_t numChannels = 0;
    std::vector<std::mt19937> rngs;
    std::normal_distribution<double> distribution{0.0, 1.0};
};


// =============================================================================
// TEMPLATE SPECIALIZATION FOR UNIFORM NOISE
// =============================================================================
template<typename T>
class Noise<T, NoiseType::Uniform>
{
public:
    // Constructors and Destructor
    Noise() = default;
    Noise(size_t newNumChannels) {
        prepare(newNumChannels);
    }
    ~Noise() = default;

    // No copy semantics nor move semantics
    Noise(const Noise&) = delete;
    const Noise& operator=(const Noise&) = delete;
    Noise(Noise&&) = delete;
    const Noise& operator=(Noise&&) = delete;

    /**
     * @brief Prepare the noise generator for processing.
     * @param newNumChannels Number of channels
     */
    void prepare(size_t newNumChannels)
    {
        numChannels = newNumChannels;
        rngs.resize(numChannels);
        for (size_t ch = 0; ch < numChannels; ++ch) {
            std::random_device rd;
            rngs[ch].seed(rd());
        }
    }

    /**
     * @brief Reseed the random number generators for all channels
     */
    void reset()
    {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            std::random_device rd;
            rngs[ch].seed(rd());
        }
    }

    /**
     * @brief Generate single sample for a specific channel 
     */
    T processSample(size_t ch)
    {
        // Uniform white noise sample
        return T(distribution(rngs[ch]));
    }

    /**
     * @brief Generate block of samples for all channels 
     */
    void processBlock(T* const* output, size_t numSamples)
    {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t n = 0; n < numSamples; ++n) {
                output[ch][n] = processSample(ch);
            }
        }
    }

private:
    size_t numChannels = 0;
    std::vector<std::mt19937> rngs;
    std::uniform_real_distribution<double> distribution{-1.0, 1.0};
};

} // namespace Jonssonic