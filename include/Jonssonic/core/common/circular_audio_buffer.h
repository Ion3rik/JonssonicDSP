// Jonssonic - A Modular Realtime C++ Audio DSP Library
// CircularAudioBuffer class header file
// SPDX-License-Identifier: MIT

#pragma once
#include "jonssonic/utils/detail/config_utils.h"
#include <jonssonic/core/common/audio_buffer.h>
#include <jonssonic/utils/math_utils.h>
#include <vector>
#include <cstddef>
#include <cassert>


namespace jonssonic::core::common
{

//==============================================================================
// CIRCULAR AUDIO BUFFER CLASS
//==============================================================================
/**
 * @brief A multi-channel circular audio buffer with power-of-two size for efficient wrap-around.
 * @tparam T Sample type (typically float or double)
 */
template<typename T>
class CircularAudioBuffer{
public:
    CircularAudioBuffer() = default;
    ~CircularAudioBuffer() = default;


    void resize(size_t newNumChannels, size_t newNumSamples) {
        bufferSize = utils::nextPowerOfTwo(newNumSamples); // ensure power-of-two size for efficient wrap-around
        buffer.resize(newNumChannels, bufferSize);
        writeIndex.assign(newNumChannels, 0);
    }

    // Write a sample to a channel (advances write index)
    void write(size_t channel, T value) {
        assert(channel < buffer.getNumChannels());
        buffer[channel][writeIndex[channel]] = value;
        writeIndex[channel] = (writeIndex[channel] + 1) & (bufferSize - 1);
    }

    // Read sample at delay (0 = most recent, 1 = previous, ...)
    T read(size_t channel, size_t delay) const {
        assert(channel < buffer.getNumChannels());
        assert(delay < bufferSize);
        size_t readIndex = (writeIndex[channel] + bufferSize - delay - 1) & (bufferSize - 1);
        return buffer[channel][readIndex];
    }

    /**
     * @brief Get a pointer to the start of a channel's data (for read-only access).
     */
    const T* readChannelPtr(size_t channel) const {
        return buffer.readChannelPtr(channel);
    }

    /**
     * @brief Get a pointer to the start of a channel's data (for write access).
     */
    T* writeChannelPtr(size_t channel) {
        return buffer.writeChannelPtr(channel);
    }

    size_t getNumChannels() const { return buffer.getNumChannels(); }
    size_t getBufferSize() const { return bufferSize; }
    size_t getChannelWriteIndex(size_t channel) const { return writeIndex[channel]; }

    void clear() {
        buffer.clear(); // zero the buffer, does not deallocate
        writeIndex.assign(buffer.getNumChannels(), 0); // reset indices
    }

private:
    AudioBuffer<T> buffer;
    std::vector<size_t> writeIndex; // per-channel write index
    size_t bufferSize = 0;
};

} // namespace Jonssonic
