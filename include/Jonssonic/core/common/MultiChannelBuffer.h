// Jonssonic - A C++ audio DSP library
// Multi-channel audio buffer class
// Author: Jon Fagerström
// Update: 18.11.2025

#pragma once
#include <vector>
#include <cassert>

namespace Jonssonic
{
/**
 * @brief A multi-channel audio buffer with efficient deinterleaved flat storage.
 * 
 * Uses deinterleaved layout [L0, L1, L2..., R0, R1, R2...] for optimal DSP processing performance.
 * Each channel's samples are stored contiguously for cache-friendly sequential access.
 * 
 * @tparam T Sample type (typically float or double)
 */
template<typename T>
class MultiChannelBuffer
{
public:
    /**
     * @brief Default constructor - creates an uninitialized buffer.
     */
    MultiChannelBuffer()
        : m_numChannels(0)
        , m_numSamples(0)
        , m_data()
    {
    }

    /**
     * @brief Constructor.
     * @param numChannels Number of audio channels
     * @param numSamples Number of samples per channel
     */
    MultiChannelBuffer(size_t numChannels, size_t numSamples)
        : m_numChannels(numChannels)
        , m_numSamples(numSamples)
        , m_data(numChannels * numSamples, T(0))
    {
    }

    /**
     * @brief Get a pointer to a specific channel's data.
     * @param channel Channel index
     * @return Pointer to the first sample of the channel
     */
    T* getChannelPointer(size_t channel)
    {
        assert(channel < m_numChannels && "Channel index out of bounds");
        return m_data.data() + (channel * m_numSamples);
    }

    /**
     * @brief Get a const pointer to a specific channel's data.
     * @param channel Channel index
     * @return Const pointer to the first sample of the channel
     */
    const T* getChannelPointer(size_t channel) const
    {
        assert(channel < m_numChannels && "Channel index out of bounds");
        return m_data.data() + (channel * m_numSamples);
    }

    /**
     * @brief Access a sample at a specific channel and index.
     * @param channel Channel index
     * @param sampleIndex Sample index within the channel
     * @return Reference to the sample
     */
    T& getSample(size_t channel, size_t sampleIndex)
    {
        assert(channel < m_numChannels && "Channel index out of bounds");
        assert(sampleIndex < m_numSamples && "Sample index out of bounds");
        return m_data[channel * m_numSamples + sampleIndex];
    }

    /**
     * @brief Access a sample at a specific channel and index (const version).
     * @param channel Channel index
     * @param sampleIndex Sample index within the channel
     * @return Const reference to the sample
     */
    const T& getSample(size_t channel, size_t sampleIndex) const
    {
        assert(channel < m_numChannels && "Channel index out of bounds");
        assert(sampleIndex < m_numSamples && "Sample index out of bounds");
        return m_data[channel * m_numSamples + sampleIndex];
    }

    /**
     * @brief Set a sample at a specific channel and index.
     * @param channel Channel index
     * @param sampleIndex Sample index within the channel
     * @param value The value to set
     */
    void setSample(size_t channel, size_t sampleIndex, T value)
    {
        getSample(channel, sampleIndex) = value;
    }

    /**
     * @brief Clear all samples in the buffer (set to zero).
     */
    void clear()
    {
        std::fill(m_data.begin(), m_data.end(), T(0));
    }

    /**
     * @brief Get the number of channels.
     */
    size_t getNumChannels() const { return m_numChannels; }

    /**
     * @brief Get the number of samples per channel.
     */
    size_t getNumSamples() const { return m_numSamples; }

    /**
     * @brief Get the total size of the flat buffer.
     */
    size_t getTotalSize() const { return m_data.size(); }

    /**
     * @brief Get direct access to the underlying flat buffer.
     * @note Data is stored deinterleaved: [Ch0_Sample0, Ch0_Sample1, ..., Ch1_Sample0, Ch1_Sample1, ...]
     */
    T* data() { return m_data.data(); }
    const T* data() const { return m_data.data(); }

    /**
     * @brief Resize the buffer.
     * @param numChannels New number of channels
     * @param numSamples New number of samples per channel
     */
    void resize(size_t numChannels, size_t numSamples)
    {
        m_numChannels = numChannels;
        m_numSamples = numSamples;
        m_data.resize(numChannels * numSamples, T(0));
    }

private:
    size_t m_numChannels;
    size_t m_numSamples;
    std::vector<T> m_data;  // Flat deinterleaved storage for all channels
};

} // namespace Jonssonic::core
