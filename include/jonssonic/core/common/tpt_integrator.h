// Jonssonic - A Modular Realtime C++ Audio DSP Library
// TPTIntegrator class (Topology-Preserving Transform Integrator)
// SPDX-License-Identifier: MIT

#pragma once
#include <algorithm>
#include <cstddef>
#include <jonssonic/core/common/audio_buffer.h>

namespace jnsc {
/**
 * @brief Topology-Preserving Transform (TPT) Integrator.
 * @tparam T Sample type (float, double, etc.)
 */
template <typename T>
class TPTIntegrator {
  public:
    /// Default constructor
    TPTIntegrator() = default;

    /// Default destructor
    ~TPTIntegrator() = default;

    /// No copy nor move semantics
    TPTIntegrator(const TPTIntegrator&) = delete;
    TPTIntegrator& operator=(const TPTIntegrator&) = delete;
    TPTIntegrator(TPTIntegrator&&) = delete;
    TPTIntegrator& operator=(TPTIntegrator&&) = delete;

    /**
     * @brief Prepare the integrator for processing.
     */
    void prepare(size_t newNumChannels, size_t newNumSections) {
        numChannels = newNumChannels;
        numSections = newNumSections;
        z.resize(numChannels, numSections);
    }

    /// Reset the integrator state
    void reset() { z.clear(); }

    /**
     * @brief Process a single sample, single channel, single section through the TPT integrator.
     * @param ch Channel index.
     * @param section Section index.
     * @param x Input sample.
     * @param g Integration coefficient.
     * @return Integrated output sample.
     */
    T processSample(size_t ch, size_t section, T x, T g) {
        T y = (g * x + z(ch, section)) / (1 + g);
        z(ch, section) = y + g * (x - y);
        return y;
    }

    /**
     * @brief Get the state variable for a specific channel and section.
     * @param ch Channel index.
     * @param section Section index.
     * @return State variable value.
     */
    T getState(size_t ch, size_t section) const { return z(ch, section); }

    /// Get number of prepared channels
    size_t getNumChannels() const { return numChannels; }
    /// Get number of prepared sections
    size_t getNumSections() const { return numSections; }
    /// Get the current state buffer.
    const AudioBuffer<T>& getState() const { return z; }

  private:
    size_t numChannels = 0;
    size_t numSections = 0;
    AudioBuffer<T> z; // state variable per channel and section.
};

} // namespace jnsc
