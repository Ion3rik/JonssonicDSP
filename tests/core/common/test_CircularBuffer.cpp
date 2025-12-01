// Jonssonic - A C++ audio DSP library
// Unit tests for CircularBuffer class
// SPDX-License-Identifier: MIT

#include "../../../include/Jonssonic/core/common/CircularBuffer.h"
#include <gtest/gtest.h>

using namespace Jonssonic;

TEST(CircularAudioBufferTest, ResizeAndWriteRead) {
    CircularAudioBuffer<float> buffer;
    buffer.resize(2, 8);
    EXPECT_EQ(buffer.getNumChannels(), 2u);
    EXPECT_EQ(buffer.getBufferSize(), 8u);
    // Write and read
    buffer.write(0, 1.0f);
    buffer.write(1, 2.0f);
    EXPECT_FLOAT_EQ(buffer.read(0, 0), 1.0f);
    EXPECT_FLOAT_EQ(buffer.read(1, 0), 2.0f);
}

TEST(CircularAudioBufferTest, WrapAround) {
    CircularAudioBuffer<float> buffer;
    buffer.resize(1, 4);
    // Write more than buffer size to test wrap
    for (int i = 0; i < 6; ++i) buffer.write(0, float(i));
    // The most recent is 5, then 4, 3, 2
    EXPECT_FLOAT_EQ(buffer.read(0, 0), 5.0f);
    EXPECT_FLOAT_EQ(buffer.read(0, 1), 4.0f);
    EXPECT_FLOAT_EQ(buffer.read(0, 2), 3.0f);
    EXPECT_FLOAT_EQ(buffer.read(0, 3), 2.0f);
}

TEST(CircularAudioBufferTest, Reset) {
    CircularAudioBuffer<float> buffer;
    buffer.resize(1, 4);
    buffer.write(0, 1.0f);
    buffer.write(0, 2.0f);
    buffer.reset();
    buffer.write(0, 3.0f);
    EXPECT_FLOAT_EQ(buffer.read(0, 0), 3.0f);
}

TEST(CircularAudioBufferTest, MultiChannel) {
    CircularAudioBuffer<float> buffer;
    buffer.resize(2, 4);
    for (int i = 0; i < 4; ++i) {
        buffer.write(0, float(i));
        buffer.write(1, float(i + 10));
    }
    EXPECT_FLOAT_EQ(buffer.read(0, 0), 3.0f);
    EXPECT_FLOAT_EQ(buffer.read(1, 0), 13.0f);
    EXPECT_FLOAT_EQ(buffer.read(0, 3), 0.0f);
    EXPECT_FLOAT_EQ(buffer.read(1, 3), 10.0f);
}
