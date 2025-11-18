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
};

// Tests for NearestInterpolator
TEST_F(InterpolatorTest, NearestDown) {
    float result = NearestInterpolator<float>::interpolate(buffer, 1, 0.3f);
    EXPECT_FLOAT_EQ(result, 10.0f);
}
TEST_F(InterpolatorTest, NearestUp) {
    float result = NearestInterpolator<float>::interpolate(buffer, 2, 0.7f);
    EXPECT_FLOAT_EQ(result, 30.0f);
}

// Tests for LinearInterpolator
TEST_F(InterpolatorTest, LinearFractionalIdx) {
    float result = LinearInterpolator<float>::interpolate(buffer, 1, 0.5f);
    EXPECT_NEAR(result, 15.0f, 1e-5);
}

TEST_F(InterpolatorTest, LinearIntegerIdx) {
    float result = LinearInterpolator<float>::interpolate(buffer, 2, 0.0f);
    EXPECT_FLOAT_EQ(result, 20.0f);
}

} // namespace Jonssonic