// Jonssonic - A C++ audio DSP library
// Multi-channel audio buffer class
// Author: Jon Fagerström
// Update: 18.11.2025

#pragma once
#include <vector>
#include <cassert>

namespace Jonssonic::core
{
/**
 * @brief Storage layout for multi-channel audio data.
 */
enum class BufferLayout
{
    Interleaved,    // [L0, R0, L1, R1, ...] - better for I/O
    Deinterleaved   // [L0, L1, ..., R0, R1, ...] - better for processing
};

/**
 * @brief A multi-channel audio buffer with efficient flat storage.
 * 
 * @tparam T Sample type (typically float or double)
 */
template<typename T>
class MultiChannelBuffer
{
public:
    /**
     * @brief Constructor.
     * @param numChannels Number of audio channels
     * @param numSamples Number of samples per channel
     * @param layout Storage layout (interleaved or deinterleaved)
     */
    MultiChannelBuffer(size_t numChannels, size_t numSamples, BufferLayout layout = BufferLayout::Deinterleaved)
        : m_numChannels(numChannels)
        , m_numSamples(numSamples)
        , m_layout(layout)
        , m_data(numChannels * numSamples, T(0))
    {
        assert(numChannels > 0 && "Number of channels must be greater than 0");
        assert(numSamples > 0 && "Number of samples must be greater than 0");
    }

    /**
     * @brief Get a pointer to a specific channel's data.
     * @param channel Channel index
     * @return Pointer to the first sample of the channel
     */
    T* getChannelPointer(size_t channel)
    {
        assert(channel < m_numChannels && "Channel index out of bounds");
        
        if (m_layout == BufferLayout::Deinterleaved)
        {
            return m_data.data() + (channel * m_numSamples);
        }
        else
        {
            return m_data.data() + channel;
        }
    }

    /**
     * @brief Get a const pointer to a specific channel's data.
     * @param channel Channel index
     * @return Const pointer to the first sample of the channel
     */
    const T* getChannelPointer(size_t channel) const
    {
        assert(channel < m_numChannels && "Channel index out of bounds");
        
        if (m_layout == BufferLayout::Deinterleaved)
        {
            return m_data.data() + (channel * m_numSamples);
        }
        else
        {
            return m_data.data() + channel;
        }
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
        
        if (m_layout == BufferLayout::Deinterleaved)
        {
            return m_data[channel * m_numSamples + sampleIndex];
        }
        else
        {
            return m_data[sampleIndex * m_numChannels + channel];
        }
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
        
        if (m_layout == BufferLayout::Deinterleaved)
        {
            return m_data[channel * m_numSamples + sampleIndex];
        }
        else
        {
            return m_data[sampleIndex * m_numChannels + channel];
        }
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
     * @brief Get the buffer layout.
     */
    BufferLayout getLayout() const { return m_layout; }

    /**
     * @brief Get the total size of the flat buffer.
     */
    size_t getTotalSize() const { return m_data.size(); }

    /**
     * @brief Get direct access to the underlying flat buffer.
     * Use with caution - you need to know the layout to interpret the data correctly.
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
    BufferLayout m_layout;
    std::vector<T> m_data;  // Flat storage for all channels
};

} // namespace Jonssonic::core
