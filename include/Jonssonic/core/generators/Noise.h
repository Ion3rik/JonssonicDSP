// Jonssonic - A Modular Realtime C++ Audio DSP Library
// Noise generator class header file
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/utils/detail/config_utils.h"

#include <cmath>
#include <cstdint>
#include <jonssonic/utils/math_utils.h>
#include <vector>

namespace jnsc {

// TODO: Move all types to generator_types.h and same for each module
enum class NoiseType {
    Uniform,
    Gaussian
    // TO DO:
    // TotallyRandom,
    // AdditiveRandom,
    // Velvet
};

// =============================================================================
// TEMPLATE CLASS DEFINITION
// =============================================================================
template <typename T, NoiseType Type = NoiseType::Uniform>
class Noise;

// =============================================================================
// TEMPLATE SPECIALIZATION FOR GAUSSIAN NOISE
// =============================================================================
template <typename T>
class Noise<T, NoiseType::Gaussian> {
  public:
    // Constructors and Destructor
    Noise() = default;
    Noise(size_t newNumChannels) { prepare(newNumChannels); }
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
    void prepare(size_t newNumChannels) {
        numChannels = newNumChannels;
        rngs.resize(numChannels);
        hasSpare.resize(numChannels, false);
        spare.resize(numChannels, 0.0f);
        for (size_t ch = 0; ch < numChannels; ++ch) {
            rngs[ch].seed(static_cast<uint32_t>(2463534242UL + ch * 7919));
            hasSpare[ch] = false;
        }
    }

    /**
     * @brief Reseed the random number generators for all channels
     */
    void reset() {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            rngs[ch].seed(static_cast<uint32_t>(2463534242UL + ch * 7919));
            hasSpare[ch] = false;
        }
    }

    /**
     * @brief Generate single sample for a specific channel
     */
    T processSample(size_t ch) {
        // Gaussian white noise sample using Box-Muller transform
        if (hasSpare[ch]) {
            hasSpare[ch] = false;
            return T(spare[ch]);
        }
        float u, v, s;
        do {
            u = rngs[ch].nextFloat();
            v = rngs[ch].nextFloat();
            s = u * u + v * v;
        } while (s >= 1.0f || s == 0.0f);
        float mul = std::sqrt(-2.0f * std::log(s) / s);
        spare[ch] = v * mul;
        hasSpare[ch] = true;
        return T(u * mul);
    }

    /**
     * @brief Generate block of samples for all channels
     */
    void processBlock(T* const* output, size_t numSamples) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t n = 0; n < numSamples; ++n) {
                output[ch][n] = processSample(ch);
            }
        }
    }

  private:
    size_t numChannels = 0;
    std::vector<Xorshift32> rngs;
    std::vector<bool> hasSpare;
    std::vector<float> spare;
};

// =============================================================================
// TEMPLATE SPECIALIZATION FOR UNIFORM NOISE
// =============================================================================
template <typename T>
class Noise<T, NoiseType::Uniform> {
  public:
    // Constructors and Destructor
    Noise() = default;
    Noise(size_t newNumChannels) { prepare(newNumChannels); }
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
    void prepare(size_t newNumChannels) {
        numChannels = newNumChannels;
        rngs.resize(numChannels);
        for (size_t ch = 0; ch < numChannels; ++ch) {
            rngs[ch].seed(static_cast<uint32_t>(2463534242UL + ch * 7919));
        }
    }

    /**
     * @brief Reseed the random number generators for all channels
     */
    void reset() {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            rngs[ch].seed(static_cast<uint32_t>(2463534242UL + ch * 7919));
        }
    }

    /**
     * @brief Generate single sample for a specific channel
     */
    T processSample(size_t ch) {
        // Uniform white noise sample in [-1, 1)
        return T(rngs[ch].nextFloat());
    }

    /**
     * @brief Generate block of samples for all channels
     */
    void processBlock(T* const* output, size_t numSamples) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t n = 0; n < numSamples; ++n) {
                output[ch][n] = processSample(ch);
            }
        }
    }

  private:
    size_t numChannels = 0;
    std::vector<Xorshift32> rngs;
};

} // namespace jnsc