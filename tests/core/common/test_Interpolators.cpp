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

// Tests for NearestInterpolator
TEST_F(InterpolatorTest, NearestDown) {
    float result = NearestInterpolator<float>::interpolate(buffer, 1, 0.3f, bufferSize);
    EXPECT_FLOAT_EQ(result, 10.0f);
}
TEST_F(InterpolatorTest, NearestUp) {
    float result = NearestInterpolator<float>::interpolate(buffer, 2, 0.7f, bufferSize);
    EXPECT_FLOAT_EQ(result, 30.0f);
}

// Tests for LinearInterpolator
TEST_F(InterpolatorTest, LinearFractionalIdx) {
    float result = LinearInterpolator<float>::interpolate(buffer, 1, 0.5f, bufferSize);
    EXPECT_NEAR(result, 15.0f, 1e-5);
}

TEST_F(InterpolatorTest, LinearIntegerIdx) {
    float result = LinearInterpolator<float>::interpolate(buffer, 2, 0.0f, bufferSize);
    EXPECT_FLOAT_EQ(result, 20.0f);
}

// Tests for circular buffer wrap-around
TEST_F(InterpolatorTest, LinearWrapAroundAtBufferEnd) {
    // Test interpolation at the last index - should wrap to first element
    float result = LinearInterpolator<float>::interpolate(buffer, 3, 0.5f, bufferSize);
    // buffer[3] = 30.0f, buffer[0] = 0.0f (wrapped)
    // Linear interpolation: 30.0 * 0.5 + 0.0 * 0.5 = 15.0
    EXPECT_NEAR(result, 15.0f, 1e-5);
}

TEST_F(InterpolatorTest, NearestWrapAroundAtBufferEnd) {
    // Test nearest neighbor at the last index with high frac - should wrap to first element
    float result = NearestInterpolator<float>::interpolate(buffer, 3, 0.6f, bufferSize);
    EXPECT_FLOAT_EQ(result, 0.0f); // Should return buffer[0] due to wrap-around
}

TEST_F(InterpolatorTest, LinearWrapAroundFullInterpolation) {
    // Test full interpolation from last to first element
    float result = LinearInterpolator<float>::interpolate(buffer, 3, 1.0f, bufferSize);
    // buffer[3] = 30.0f, buffer[0] = 0.0f
    // 30.0 * 0.0 + 0.0 * 1.0 = 0.0
    EXPECT_NEAR(result, 0.0f, 1e-5);
}

} // namespace Jonssonic