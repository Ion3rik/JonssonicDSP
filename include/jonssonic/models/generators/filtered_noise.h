// Jonssonic - A C++ audio DSP library
// FilteredNoise class header file
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/utils/detail/config_utils.h"

#include <jonssonic/core/filters/one_pole_filter.h>
#include <jonssonic/core/generators/noise.h>
#include <vector>

namespace jnsc::models {
/// Filter types for FilteredNoise
enum class FilterType { Lowpass1stOrder, Lowpass2ndOrder };

// =============================================================================
// TEMPLATE CLASS DECLARATION
// =============================================================================
/**
 * @brief FilteredNoise wraps a Noise generator and applies a filter for colored noise (e.g. pink,
 * brown).
 * @tparam T Sample type
 * @tparam NoiseType Underlying noise type (e.g. Gaussian, Uniform)
 * @tparam FilterType Type of filter to apply for coloring the noise
 */
template <typename T, NoiseType noiseType = NoiseType::Uniform, FilterType filterType = FilterType::Lowpass2ndOrder>
class FilteredNoise;

// =============================================================================
// Lowpass1stOrder Filtered Noise Specialization
// =============================================================================
template <typename T, NoiseType noiseType>
class FilteredNoise<T, noiseType, FilterType::Lowpass1stOrder> {
  public:
    FilteredNoise() = default;
    FilteredNoise(size_t newNumChannels, T newSampleRate) { prepare(newNumChannels, newSampleRate); }
    ~FilteredNoise() = default;

    /**
     * @brief Prepare the filtered noise generator for processing.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     * @note Must be called before processing
     */
    void prepare(size_t newNumChannels, T newSampleRate) {
        // Store parameters
        numChannels = newNumChannels;
        sampleRate = newSampleRate;

        // Prepare components
        noise.prepare(newNumChannels);
        filter.prepare(newNumChannels, sampleRate);
        filter.setResponse(OnePoleFilter<T>::Response::Lowpass);
        filter.setFrequency(Frequency<T>::Hertz(1000)); // Default cutoff frequency

        // Mark as prepared
        togglePrepared = true;
    }

    /// Reset internal states
    void reset() {
        noise.reset();
        filter.reset();
    }

    /**
     * @brief Generate single sample for a specific channel
     * @param ch Channel index
     * @return Filtered noise sample
     * @note Must call prepare() before processing
     */
    T processSample(size_t ch) {
        T sample = noise.processSample(ch);      // Get raw noise sample
        return filter.processSample(ch, sample); // Apply filter
    }

    void processBlock(T* const* output, size_t numSamples) {
        noise.processBlock(output, numSamples);          // Generate raw noise block
        filter.processBlock(output, output, numSamples); // Apply filter to block
    }

    /**
     * @brief Set the lowpass filter cutoff frequency in various units.
     * @param newFreq Cutoff frequency struct.
     */
    void setCutoff(Frequency<T> newFreq) { filter.setFrequency(newFreq); }

    size_t getNumChannels() const { return numChannels; }

    bool isPrepared() const { return togglePrepared; }

  private:
    bool togglePrepared = false;
    size_t numChannels = 0;
    T sampleRate = T(44100);
    Noise<T, noiseType> noise;
    OnePoleFilter<T> filter;
};

// =============================================================================
// Lowpass2ndOrder Filtered Noise Specialization
// =============================================================================
template <typename T, NoiseType noiseType>
class FilteredNoise<T, noiseType, FilterType::Lowpass2ndOrder> {
  public:
    FilteredNoise() = default;
    FilteredNoise(size_t newNumChannels, T newSampleRate) { prepare(newNumChannels, newSampleRate); }
    ~FilteredNoise() = default;

    /**
     * @brief Prepare the filtered noise generator for processing.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     * @note Must be called before processing
     */
    void prepare(size_t newNumChannels, T newSampleRate) {
        // Store parameters
        numChannels = newNumChannels;
        sampleRate = newSampleRate;

        // Prepare components
        noise.prepare(newNumChannels);
        filter.prepare(newNumChannels, sampleRate);
        filter.setResponse(BiquadFilter<T>::Response::Lowpass);
        filter.setFrequency(Frequency<T>::Hertz(1000)); // Default cutoff frequency

        // Mark as prepared
        togglePrepared = true;
    }

    /// Reset internal states
    void reset() {
        noise.reset();
        filter.reset();
    }

    /**
     * @brief Generate single sample for a specific channel
     * @param ch Channel index
     * @return Filtered noise sample
     * @note Must call prepare() before processing
     */
    T processSample(size_t ch) {
        T sample = noise.processSample(ch);      // Get raw noise sample
        return filter.processSample(ch, sample); // Apply filter
    }

    void processBlock(T* const* output, size_t numSamples) {
        noise.processBlock(output, numSamples);          // Generate raw noise block
        filter.processBlock(output, output, numSamples); // Apply filter to block
    }
    /**
     * @brief Set the lowpass filter cutoff frequency in various units.
     * @param newFreq Cutoff frequency struct.
     */
    void setCutoff(Frequency<T> newFreq) { filter.setFrequency(newFreq); }

    size_t getNumChannels() const { return numChannels; }
    bool isPrepared() const { return togglePrepared; }

  private:
    bool togglePrepared = false;
    size_t numChannels = 0;
    T sampleRate = T(44100);
    Noise<T, noiseType> noise;
    BiquadFilter<T> filter;
};

} // namespace jnsc::models
