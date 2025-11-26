// Jonssonic - A C++ audio DSP library
// Unit tests for Interpolators
// Author: Jon Fagerström
// Update: 18.11.2025

#include <gtest/gtest.h>
#include "Jonssonic/core/common/Interpolators.h"

namespace Jonssonic {

class InterpolatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // common setup code
    }

    void TearDown() override {
        // cleanup code
    }

    float buffer[4] = {0.0f, 10.0f, 20.0f, 30.0f};
    static constexpr size_t bufferSize = 4;
};

// Tests for NearestInterpolator (backward interpolation)
TEST_F(InterpolatorTest, NearestDown) {
    // frac < 0.5 returns buffer[idx]
    float result = NearestInterpolator<float>::interpolateBackward(buffer, 1, 0.3f, bufferSize);
    EXPECT_FLOAT_EQ(result, 10.0f); // buffer[1]
}
TEST_F(InterpolatorTest, NearestUp) {
    // frac >= 0.5 returns buffer[prevIdx]
    float result = NearestInterpolator<float>::interpolateBackward(buffer, 2, 0.7f, bufferSize);
    EXPECT_FLOAT_EQ(result, 10.0f); // buffer[1] (prevIdx)
}

// Tests for LinearInterpolator (backward interpolation)
TEST_F(InterpolatorTest, LinearFractionalIdx) {
    // Interpolate backward: buffer[1] * 0.5 + buffer[0] * 0.5
    float result = LinearInterpolator<float>::interpolateBackward(buffer, 1, 0.5f, bufferSize);
    EXPECT_NEAR(result, 5.0f, 1e-5); // 10.0 * 0.5 + 0.0 * 0.5
}

TEST_F(InterpolatorTest, LinearIntegerIdx) {
    // frac = 0.0 returns buffer[idx]
    float result = LinearInterpolator<float>::interpolateBackward(buffer, 2, 0.0f, bufferSize);
    EXPECT_FLOAT_EQ(result, 20.0f); // buffer[2]
}

// Tests for circular buffer wrap-around
TEST_F(InterpolatorTest, LinearWrapAroundAtBufferEnd) {
    // Interpolate backward at buffer end: buffer[3] * 0.5 + buffer[2] * 0.5
    float result = LinearInterpolator<float>::interpolateBackward(buffer, 3, 0.5f, bufferSize);
    EXPECT_NEAR(result, 25.0f, 1e-5); // 30.0 * 0.5 + 20.0 * 0.5
}

TEST_F(InterpolatorTest, NearestWrapAroundAtBufferEnd) {
    // frac >= 0.5 at buffer end returns prevIdx
    float result = NearestInterpolator<float>::interpolateBackward(buffer, 3, 0.6f, bufferSize);
    EXPECT_FLOAT_EQ(result, 20.0f); // buffer[2] (prevIdx)
}

TEST_F(InterpolatorTest, LinearWrapAroundFullInterpolation) {
    // frac = 1.0 returns buffer[prevIdx] fully
    float result = LinearInterpolator<float>::interpolateBackward(buffer, 3, 1.0f, bufferSize);
    EXPECT_NEAR(result, 20.0f, 1e-5); // buffer[2] (prevIdx)
}

// Tests for forward interpolation
TEST_F(InterpolatorTest, LinearForwardFractionalIdx) {
    // Interpolate forward: buffer[1] * 0.5 + buffer[2] * 0.5
    float result = LinearInterpolator<float>::interpolateForward(buffer, 1, 0.5f, bufferSize);
    EXPECT_NEAR(result, 15.0f, 1e-5); // 10.0 * 0.5 + 20.0 * 0.5
}

TEST_F(InterpolatorTest, NearestForwardUp) {
    // frac >= 0.5 returns buffer[nextIdx]
    float result = NearestInterpolator<float>::interpolateForward(buffer, 1, 0.7f, bufferSize);
    EXPECT_FLOAT_EQ(result, 20.0f); // buffer[2] (nextIdx)
}

} // namespace Jonssonic