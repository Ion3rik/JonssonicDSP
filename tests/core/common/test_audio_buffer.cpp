// Jonssonic - A C++ audio DSP library
// Unit tests for AudioBuffer class
// SPDX-License-Identifier: MIT
#include "../../../include/Jonssonic/core/common/AudioBuffer.h"
#include <gtest/gtest.h>

using namespace Jonssonic;

TEST(AudioBufferTest, ResizeAndAccess) {
    AudioBuffer<float> buffer;
    buffer.resize(2, 8);
    EXPECT_EQ(buffer.getNumChannels(), 2u);
    EXPECT_EQ(buffer.getNumSamples(), 8u);
    // Write and read
    buffer[0][0] = 1.0f;
    buffer[1][7] = 2.0f;
    EXPECT_FLOAT_EQ(buffer[0][0], 1.0f);
    EXPECT_FLOAT_EQ(buffer[1][7], 2.0f);
}

TEST(AudioBufferTest, Clear) {
    AudioBuffer<float> buffer;
    buffer.resize(2, 4);
    buffer[0][0] = 1.0f;
    buffer[1][1] = 2.0f;
    buffer.clear();
    for (size_t ch = 0; ch < 2; ++ch)
        for (size_t i = 0; i < 4; ++i)
            EXPECT_FLOAT_EQ(buffer[ch][i], 0.0f);
}

TEST(AudioBufferTest, GetWritePointer) {
    AudioBuffer<float> buffer;
    buffer.resize(1, 4);
    float* ptr = buffer.writeChannelPtr(0);
    ptr[2] = 3.0f;
    EXPECT_FLOAT_EQ(buffer[0][2], 3.0f);
}

TEST(AudioBufferTest, OutOfBounds) {
    AudioBuffer<float> buffer;
    buffer.resize(1, 2);
    // No assert/exception, but check access
    buffer[0][0] = 1.0f;
    buffer[0][1] = 2.0f;
    EXPECT_FLOAT_EQ(buffer[0][0], 1.0f);
    EXPECT_FLOAT_EQ(buffer[0][1], 2.0f);
}
