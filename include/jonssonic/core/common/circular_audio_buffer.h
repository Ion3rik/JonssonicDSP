// Jonssonic - A Modular Realtime C++ Audio DSP Library
// CircularAudioBuffer class header file
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/utils/detail/config_utils.h"
#include <cassert>
#include <cstddef>
#include <jonssonic/core/common/audio_buffer.h>
#include <jonssonic/utils/math_utils.h>
#include <vector>

namespace jnsc {

//==============================================================================
// CIRCULAR AUDIO BUFFER CLASS
//==============================================================================
/**
 * @brief A multi-channel circular audio buffer with power-of-two size for efficient wrap-around.
 * @tparam T Sample type (typically float or double)
 */
template <typename T>
class CircularAudioBuffer {
  public:
    CircularAudioBuffer() = default;
    ~CircularAudioBuffer() = default;

    /// No copy semantics nor move semantics
    CircularAudioBuffer(const CircularAudioBuffer&) = delete;
    const CircularAudioBuffer& operator=(const CircularAudioBuffer&) = delete;
    CircularAudioBuffer(CircularAudioBuffer&&) = delete;
    const CircularAudioBuffer& operator=(CircularAudioBuffer&&) = delete;

    /**
     * @brief Resize the circular buffer to accommodate a given number of channels and samples.
     * @param newNumChannels Number of audio channels
     * @param newNumSamples Number of samples per channel
     * @note The actual buffer size will be the next power of two greater than or equal to newNumSamples for efficient
     * wrap-around using bitwise operations.
     */
    void resize(size_t newNumChannels, size_t newNumSamples) {
        bufferSize = utils::nextPowerOfTwo(newNumSamples); // ensure power-of-two size for efficient wrap-around
        buffer.resize(newNumChannels, bufferSize);
        writeIndex.assign(newNumChannels, 0);
    }

    /// Clear the buffer and reset write indices
    void clear() {
        buffer.clear();
        writeIndex.assign(buffer.getNumChannels(), 0);
    }

    /**
     * @brief Write a sample to the buffer at the current write position for a given channel and increment the write
     * index.
     * @param channel Channel index to write to
     * @param value Sample value to write
     */
    void write(size_t channel, T value) {
        assert(channel < buffer.getNumChannels());
        buffer[channel][writeIndex[channel]] = value;
        // Increment write index with wrap-around using bitwise AND
        writeIndex[channel] = (writeIndex[channel] + 1) & (bufferSize - 1);
    }

    /**
     * @brief Read a sample from the buffer at a given delay for a given channel.
     * @param channel Channel index to read from
     * @param delay Delay in samples (0 = most recent, 1 = previous, ...)
     * @return Sample value
     */
    T read(size_t channel, size_t delay) const {
        assert(channel < buffer.getNumChannels());
        size_t readIndex = (writeIndex[channel] + bufferSize - delay - 1) & (bufferSize - 1);
        assert(readIndex < bufferSize);
        return buffer[channel][readIndex];
    }

    /**
     * @brief Get a pointer to the start of a channel's data (for read-only access).
     * @param channel Channel index
     * @return Pointer to the start of the channel's data
     */
    const T* readChannelPtr(size_t channel) const { return buffer.readChannelPtr(channel); }

    /**
     * @brief Get a pointer to the start of a channel's data (for write access).
     * @param channel Channel index
     * @return Pointer to the start of the channel's data
     */
    T* writeChannelPtr(size_t channel) { return buffer.writeChannelPtr(channel); }

    /// Get number of channels
    size_t getNumChannels() const { return buffer.getNumChannels(); }
    /// Get buffer size (number of samples per channel)
    size_t getBufferSize() const { return bufferSize; }
    /// Get current write index for a channel
    size_t getChannelWriteIndex(size_t channel) const { return writeIndex[channel]; }
    /// Get current read index for a channel and delay
    size_t getChannelReadIndex(size_t channel, size_t delay) const {
        assert(channel < buffer.getNumChannels());
        assert(delay < bufferSize);
        return (writeIndex[channel] + bufferSize - delay - 1) & (bufferSize - 1);
    }

  private:
    AudioBuffer<T> buffer;
    std::vector<size_t> writeIndex; // per-channel write index
    size_t bufferSize = 0;
};

} // namespace jnsc
