// Jonssonic - A Modular Realtime C++ Audio DSP Library
// General-purpose audio buffer class
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/utils/detail/config_utils.h"
#include <cassert>
#include <vector>

namespace jnsc {
//==============================================================================
// LAYOUT TYPES
//==============================================================================
/**
 * @brief Buffer layout type for audio data.
 */
enum class BufferLayout {
    Planar,     /**< Planar layout: [ch0_s0, ch0_s1, ..., ch1_s0, ch1_s1, ...] */
    Interleaved /**< Interleaved layout: [s0_ch0, s0_ch1, ..., s1_ch0, s1_ch1, ...] */
};

// ===============================================================================
// TEMPLATE DECLARATION
// ===============================================================================
/**
 * @brief A multi-channel audio buffer supporting both planar and interleaved layouts.
 *
 * Layout is selected via the BufferLayout template parameter:
 *   - Planar: [Ch0_Sample0, Ch0_Sample1, ..., Ch1_Sample0, Ch1_Sample1, ...]
 *     (Each channel's samples are stored contiguously for cache-friendly per-channel access.)
 *   - Interleaved: [Sample0_Ch0, Sample0_Ch1, ..., Sample1_Ch0, Sample1_Ch1, ...]
 *     (All channels for each sample are stored contiguously for cache-friendly per-sample access.)
 *
 * @tparam T Sample type (typically float or double)
 * @tparam BufferLayout Layout of the buffer (default: Planar)
 * @note All the Jonssonic DSP processors use planar layout internally for simplicity and
 * performance.
 */

template <typename T, BufferLayout Layout = BufferLayout::Planar>
class AudioBuffer;

//==============================================================================
// Planar specialization
//==============================================================================
/// Planar layout specialization.
template <typename T>
class AudioBuffer<T, BufferLayout::Planar> {

  public:
    /**
     * @brief Default constructor - creates an uninitialized buffer.
     */
    AudioBuffer() : m_numChannels(0), m_numSamples(0), m_data() {}

    /**
     * @brief Parameterized constructor.
     * @param numChannels Number of audio channels.
     * @param numSamples Number of samples per channel.
     */
    AudioBuffer(size_t numChannels, size_t numSamples)
        : m_numChannels(numChannels), m_numSamples(numSamples),
          m_data(numChannels * numSamples, T(0)) {}

    /**
     * @brief Get array of const pointers to each channel's data (for read-only access).
     */
    const T* const* readPtrs() const {
        m_readPtrs.resize(m_numChannels);
        for (size_t ch = 0; ch < m_numChannels; ++ch)
            m_readPtrs[ch] = m_data.data() + (ch * m_numSamples);
        return m_readPtrs.data();
    }

    /**
     * @brief Get array of pointers to each channel's data (for write access).
     */
    T* const* writePtrs() {
        m_writePtrs.resize(m_numChannels);
        for (size_t ch = 0; ch < m_numChannels; ++ch)
            m_writePtrs[ch] = m_data.data() + (ch * m_numSamples);
        return m_writePtrs.data();
    }

    /**
     * @brief Get a pointer to the start of a channel's data (for write access).
     */
    T* writeChannelPtr(size_t channel) {
        assert(channel < m_numChannels && "Channel index out of bounds");
        return m_data.data() + (channel * m_numSamples);
    }

    /**
     * @brief Get a pointer to the start of a channel's data (for read-only access).
     */
    const T* readChannelPtr(size_t channel) const {
        assert(channel < m_numChannels && "Channel index out of bounds");
        return m_data.data() + (channel * m_numSamples);
    }

    /**
     * @brief Get pointers to a single sample index across all channels (for write access).
     * @note For planar, this is less efficient due to non-contiguous memory access, but can be
     * useful for some algorithms.
     */
    std::vector<T*> writeSamplePtr(size_t sampleIdx) {
        assert(sampleIdx < m_numSamples && "Sample index out of bounds");
        std::vector<T*> ptrs(m_numChannels);
        for (size_t ch = 0; ch < m_numChannels; ++ch)
            ptrs[ch] = m_data.data() + (ch * m_numSamples) + sampleIdx;
        return ptrs;
    }

    /**
     * @brief Get pointers to a single sample index across all channels (for read-only access).
     * @note For planar, this is less efficient due to non-contiguous memory access, but can be
     * useful for some algorithms.
     */
    std::vector<const T*> readSamplePtr(size_t sampleIdx) const {
        assert(sampleIdx < m_numSamples && "Sample index out of bounds");
        std::vector<const T*> ptrs(m_numChannels);
        for (size_t ch = 0; ch < m_numChannels; ++ch)
            ptrs[ch] = m_data.data() + (ch * m_numSamples) + sampleIdx;
        return ptrs;
    }

    /**
     * @brief Clear all samples in the buffer (set to zero).
     */
    void clear() { std::fill(m_data.begin(), m_data.end(), T(0)); }

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
     * @note Data is stored planar: [Ch0_Sample0, Ch0_Sample1, ..., Ch1_Sample0, Ch1_Sample1, ...]
     */
    T* data() { return m_data.data(); }
    const T* data() const { return m_data.data(); }

    /**
     * @brief Resize the buffer.
     * @param numChannels New number of channels
     * @param numSamples New number of samples per channel
     */
    void resize(size_t numChannels, size_t numSamples) {
        m_numChannels = numChannels;
        m_numSamples = numSamples;
        m_data.assign(numChannels * numSamples,
                      T(0)); // This is what you expect for audio buffers not std::vector::resize()
    }

    // ========================================================================
    // Proxy classes for channel access
    // ========================================================================

    // Proxy class for channel access
    class ChannelProxy {
      public:
        ChannelProxy(T* channelData, size_t numSamples)
            : m_channelData(channelData), m_numSamples(numSamples) {}
        T& operator[](size_t sample) {
            assert(sample < m_numSamples && "Sample index out of bounds");
            return m_channelData[sample];
        }
        const T& operator[](size_t sample) const {
            assert(sample < m_numSamples && "Sample index out of bounds");
            return m_channelData[sample];
        }

      private:
        T* m_channelData;
        size_t m_numSamples;
    };

    // Const proxy for channel access
    class ConstChannelProxy {
      public:
        ConstChannelProxy(const T* channelData, size_t numSamples)
            : m_channelData(channelData), m_numSamples(numSamples) {}
        const T& operator[](size_t sample) const {
            assert(sample < m_numSamples && "Sample index out of bounds");
            return m_channelData[sample];
        }

      private:
        const T* m_channelData;
        size_t m_numSamples;
    };

    // ========================================================================
    // Operator Overloads
    // ========================================================================
    /**
     * @brief Get a proxy to access a specific channel.
     * @param channel Channel index
     */
    ChannelProxy operator[](size_t channel) {
        assert(channel < m_numChannels && "Channel index out of bounds");
        return ChannelProxy(m_data.data() + (channel * m_numSamples), m_numSamples);
    }
    ConstChannelProxy operator[](size_t channel) const {
        assert(channel < m_numChannels && "Channel index out of bounds");
        return ConstChannelProxy(m_data.data() + (channel * m_numSamples), m_numSamples);
    }
    /**
     * @brief Add two buffers element-wise
     */
    AudioBuffer operator+(const AudioBuffer& other) const {
        assert(m_numChannels == other.m_numChannels && m_numSamples == other.m_numSamples);
        AudioBuffer result(m_numChannels, m_numSamples);
        for (size_t i = 0; i < m_data.size(); ++i)
            result.m_data[i] = m_data[i] + other.m_data[i];
        return result;
    }

    /**
     * @brief Add scalar to all samples
     */
    AudioBuffer operator+(T scalar) const {
        AudioBuffer result(m_numChannels, m_numSamples);
        for (size_t i = 0; i < m_data.size(); ++i)
            result.m_data[i] = m_data[i] + scalar;
        return result;
    }

    /**
     * @brief Multiply buffer by scalar (apply gain)
     */
    AudioBuffer operator*(T scalar) const {
        AudioBuffer result(m_numChannels, m_numSamples);
        for (size_t i = 0; i < m_data.size(); ++i)
            result.m_data[i] = m_data[i] * scalar;
        return result;
    }

    /**
     * @brief Multiply two buffers element-wise (ring modulation)
     */
    AudioBuffer operator*(const AudioBuffer& other) const {
        assert(m_numChannels == other.m_numChannels && m_numSamples == other.m_numSamples);
        AudioBuffer result(m_numChannels, m_numSamples);
        for (size_t i = 0; i < m_data.size(); ++i)
            result.m_data[i] = m_data[i] * other.m_data[i];
        return result;
    }

    /**
     * @brief In-place addition
     */
    AudioBuffer& operator+=(const AudioBuffer& other) {
        assert(m_numChannels == other.m_numChannels && m_numSamples == other.m_numSamples);
        for (size_t i = 0; i < m_data.size(); ++i)
            m_data[i] += other.m_data[i];
        return *this;
    }

    /**
     * @brief In-place addition with scalar
     */
    AudioBuffer& operator+=(T scalar) {
        for (size_t i = 0; i < m_data.size(); ++i)
            m_data[i] += scalar;
        return *this;
    }

    /**
     * @brief In-place multiplication (apply gain)
     */
    AudioBuffer& operator*=(T scalar) {
        for (size_t i = 0; i < m_data.size(); ++i)
            m_data[i] *= scalar;
        return *this;
    }

    /**
     * @brief In-place element-wise multiplication
     */
    AudioBuffer& operator*=(const AudioBuffer& other) {
        assert(m_numChannels == other.m_numChannels && m_numSamples == other.m_numSamples);
        for (size_t i = 0; i < m_data.size(); ++i)
            m_data[i] *= other.m_data[i];
        return *this;
    }

    /**
     * @brief Assignment operator
     */
    AudioBuffer& operator=(const AudioBuffer& other) {
        if (this != &other) {
            m_numChannels = other.m_numChannels;
            m_numSamples = other.m_numSamples;
            m_data = other.m_data;
        }
        return *this;
    }

  private:
    size_t m_numSamples;
    size_t m_numChannels;
    std::vector<T> m_data; // Flat planar storage for all channels

    // Cached pointer arrays for readPtrs() and writePtrs()
    mutable std::vector<const T*> m_readPtrs;
    mutable std::vector<T*> m_writePtrs;
};

//==============================================================================
// Interleaved specialization
//==============================================================================
template <typename T>
class AudioBuffer<T, BufferLayout::Interleaved> {
  public:
    /**
     * @brief Default constructor - creates an uninitialized buffer.
     */
    AudioBuffer() : m_numChannels(0), m_numSamples(0), m_data() {}

    /**
     * @brief Constructor.
     * @param numChannels Number of audio channels
     * @param numSamples Number of samples per channel
     */
    AudioBuffer(size_t numChannels, size_t numSamples)
        : m_numChannels(numChannels), m_numSamples(numSamples),
          m_data(numChannels * numSamples, T(0)) {}

    /**
     * @brief Get array of const pointers to each sample's data (for read-only access).
     * @note For interleaved, each pointer is to the first channel of a sample (i.e., &data[sample *
     * numChannels]). Useful for per-sample block processing.
     */
    const T* const* readPtrs() const {
        m_readPtrs.resize(m_numSamples);
        for (size_t s = 0; s < m_numSamples; ++s)
            m_readPtrs[s] = m_data.data() + (s * m_numChannels);
        return m_readPtrs.data();
    }

    /**
     * @brief Get array of pointers to each sample's data (for write access).
     * @note For interleaved, each pointer is to the first channel of a sample (i.e., &data[sample *
     * numChannels]). Useful for per-sample block processing.
     */
    T* const* writePtrs() {
        m_writePtrs.resize(m_numSamples);
        for (size_t s = 0; s < m_numSamples; ++s)
            m_writePtrs[s] = m_data.data() + (s * m_numChannels);
        return m_writePtrs.data();
    }

    /**
     * @brief Get a pointer to the start of a sample's data (all channels at that sample, for write
     * access).
     * @param sampleIdx Sample index
     */
    T* writeSamplePtr(size_t sampleIdx) {
        assert(sampleIdx < m_numSamples && "Sample index out of bounds");
        return m_data.data() + (sampleIdx * m_numChannels);
    }

    /**
     * @brief Get a pointer to the start of a sample's data (all channels at that sample, for
     * read-only access).
     * @param sampleIdx Sample index
     */
    const T* readSamplePtr(size_t sampleIdx) const {
        assert(sampleIdx < m_numSamples && "Sample index out of bounds");
        return m_data.data() + (sampleIdx * m_numChannels);
    }

    /**
     * @brief Get pointers to a single channel across all samples (for write access).
     * @param channel Channel index
     * @note For interleaved, this is less efficient due to non-contiguous memory access, but can be
     * useful for some algorithms.
     */
    std::vector<T*> writeChannelPtr(size_t channel) {
        assert(channel < m_numChannels && "Channel index out of bounds");
        std::vector<T*> ptrs(m_numSamples);
        for (size_t s = 0; s < m_numSamples; ++s)
            ptrs[s] = m_data.data() + (s * m_numChannels) + channel;
        return ptrs;
    }

    /**
     * @brief Get pointers to a single channel across all samples (for read-only access).
     * @param channel Channel index
     * @note For interleaved, this is less efficient due to non-contiguous memory access, but can be
     * useful for some algorithms.
     */
    std::vector<const T*> readChannelPtr(size_t channel) const {
        assert(channel < m_numChannels && "Channel index out of bounds");
        std::vector<const T*> ptrs(m_numSamples);
        for (size_t s = 0; s < m_numSamples; ++s)
            ptrs[s] = m_data.data() + (s * m_numChannels) + channel;
        return ptrs;
    }

    /**
     * @brief Clear all samples in the buffer (set to zero).
     */
    void clear() { std::fill(m_data.begin(), m_data.end(), T(0)); }

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
     * @note Data is stored interleaved: [Sample0_Ch0, Sample0_Ch1, ..., Sample1_Ch0, ...]
     */
    T* data() { return m_data.data(); }
    const T* data() const { return m_data.data(); }

    /**
     * @brief Resize the buffer.
     * @param numChannels New number of channels
     * @param numSamples New number of samples per channel
     */
    void resize(size_t numChannels, size_t numSamples) {
        m_numChannels = numChannels;
        m_numSamples = numSamples;
        m_data.assign(numChannels * numSamples, T(0));
    }

    // Proxy for sample-major access: buffer[sample][channel]
    class SampleProxy {
      public:
        SampleProxy(T* data, size_t numChannels, size_t channelCount)
            : m_data(data), m_numChannels(numChannels), m_channelCount(channelCount) {}
        T& operator[](size_t channel) {
            assert(channel < m_channelCount && "Channel index out of bounds");
            return m_data[channel];
        }
        const T& operator[](size_t channel) const {
            assert(channel < m_channelCount && "Channel index out of bounds");
            return m_data[channel];
        }

      private:
        T* m_data;
        size_t m_numChannels;
        size_t m_channelCount;
    };
    class ConstSampleProxy {
      public:
        ConstSampleProxy(const T* data, size_t numChannels, size_t channelCount)
            : m_data(data), m_numChannels(numChannels), m_channelCount(channelCount) {}
        const T& operator[](size_t channel) const {
            assert(channel < m_channelCount && "Channel index out of bounds");
            return m_data[channel];
        }

      private:
        const T* m_data;
        size_t m_numChannels;
        size_t m_channelCount;
    };

    // buffer[sample][channel]
    SampleProxy operator[](size_t sample) {
        assert(sample < m_numSamples && "Sample index out of bounds");
        return SampleProxy(m_data.data() + (sample * m_numChannels), m_numChannels, m_numChannels);
    }
    ConstSampleProxy operator[](size_t sample) const {
        assert(sample < m_numSamples && "Sample index out of bounds");
        return ConstSampleProxy(m_data.data() + (sample * m_numChannels),
                                m_numChannels,
                                m_numChannels);
    }

    // Arithmetic and assignment operators remain unchanged
    AudioBuffer operator+(const AudioBuffer& other) const {
        assert(m_numChannels == other.m_numChannels && m_numSamples == other.m_numSamples);
        AudioBuffer result(m_numChannels, m_numSamples);
        for (size_t i = 0; i < m_data.size(); ++i)
            result.m_data[i] = m_data[i] + other.m_data[i];
        return result;
    }
    AudioBuffer operator+(T scalar) const {
        AudioBuffer result(m_numChannels, m_numSamples);
        for (size_t i = 0; i < m_data.size(); ++i)
            result.m_data[i] = m_data[i] + scalar;
        return result;
    }
    AudioBuffer operator*(T scalar) const {
        AudioBuffer result(m_numChannels, m_numSamples);
        for (size_t i = 0; i < m_data.size(); ++i)
            result.m_data[i] = m_data[i] * scalar;
        return result;
    }
    AudioBuffer operator*(const AudioBuffer& other) const {
        assert(m_numChannels == other.m_numChannels && m_numSamples == other.m_numSamples);
        AudioBuffer result(m_numChannels, m_numSamples);
        for (size_t i = 0; i < m_data.size(); ++i)
            result.m_data[i] = m_data[i] * other.m_data[i];
        return result;
    }
    AudioBuffer& operator+=(const AudioBuffer& other) {
        assert(m_numChannels == other.m_numChannels && m_numSamples == other.m_numSamples);
        for (size_t i = 0; i < m_data.size(); ++i)
            m_data[i] += other.m_data[i];
        return *this;
    }
    AudioBuffer& operator+=(T scalar) {
        for (size_t i = 0; i < m_data.size(); ++i)
            m_data[i] += scalar;
        return *this;
    }
    AudioBuffer& operator*=(T scalar) {
        for (size_t i = 0; i < m_data.size(); ++i)
            m_data[i] *= scalar;
        return *this;
    }
    AudioBuffer& operator*=(const AudioBuffer& other) {
        assert(m_numChannels == other.m_numChannels && m_numSamples == other.m_numSamples);
        for (size_t i = 0; i < m_data.size(); ++i)
            m_data[i] *= other.m_data[i];
        return *this;
    }
    AudioBuffer& operator=(const AudioBuffer& other) {
        if (this != &other) {
            m_numChannels = other.m_numChannels;
            m_numSamples = other.m_numSamples;
            m_data = other.m_data;
        }
        return *this;
    }

  private:
    size_t m_numSamples;
    size_t m_numChannels;
    std::vector<T> m_data;
    mutable std::vector<const T*> m_readPtrs;
    mutable std::vector<T*> m_writePtrs;
};

// ============================================================================
// Friend operators for scalar on left-hand side
// ============================================================================

template <typename T, BufferLayout L>
inline AudioBuffer<T, L> operator*(T scalar, const AudioBuffer<T, L>& buffer) {
    return buffer * scalar;
}

template <typename T, BufferLayout L>
inline AudioBuffer<T, L> operator+(T scalar, const AudioBuffer<T, L>& buffer) {
    return buffer + scalar;
}

} // namespace jnsc
