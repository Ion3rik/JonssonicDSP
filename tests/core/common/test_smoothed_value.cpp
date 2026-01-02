// Jonssonic - A C++ audio DSP library
// Unit tests for SmoothedValue
// SPDX-License-Identifier: MIT

#include <gtest/gtest.h>
#include <jonssonic/core/common/smoothed_value.h>

namespace jonssonic::core::common {

class SmoothedValueTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SmoothedValueTest, OnePoleOrder1BasicSmoothing) {
    SmoothedValue<float, SmootherType::OnePole, 1> smoother;
    smoother.prepare(1, 1000);
    smoother.setTimeMs(10);
    smoother.reset();
    smoother.setTarget(1.0f);
    float last = 0.0f;
    for (int i = 0; i < 100; ++i) {
        last = smoother.getNextValue(0);
    }
    EXPECT_TRUE(last > 0.99f);
}

TEST_F(SmoothedValueTest, OnePoleOrder2Cascaded) {
    SmoothedValue<float, SmootherType::OnePole, 2> smoother;
    smoother.prepare(1, 1000);
    smoother.setTimeMs(10);
    smoother.reset();
    smoother.setTarget(1.0f);
    float last = 0.0f;
    for (int i = 0; i < 200; ++i) {
        last = smoother.getNextValue(0);
    }
    EXPECT_TRUE(last > 0.99f);
}

TEST_F(SmoothedValueTest, LinearBasic) {
    LinearSmoother<float> smoother;
    smoother.prepare(1, 1000);
    smoother.setTimeMs(10);
    smoother.reset();
    smoother.setTarget(1.0f);
    float val = 0.0f;
    for (int i = 0; i < 10; ++i) {
        val = smoother.getNextValue(0);
    }
    EXPECT_NEAR(val, 1.0f, 1e-3f);
}

TEST_F(SmoothedValueTest, LinearReachesTargetExactly) {
    LinearSmoother<float> smoother;
    smoother.prepare(1, 1000);
    smoother.setTimeMs(20);
    smoother.reset();
    smoother.setTarget(2.0f);
    float val = 0.0f;
    for (int i = 0; i < 20; ++i) {
        val = smoother.getNextValue(0);
    }
    EXPECT_NEAR(val, 2.0f, 1e-3f);
}

TEST_F(SmoothedValueTest, ResetSetsCurrentAndTarget) {
    SmoothedValue<float, SmootherType::OnePole, 1> smoother;
    smoother.prepare(1, 1000);
    smoother.setTimeMs(10);
    smoother.reset();
    smoother.setTarget(0.5f);
    // After setTarget, current is still at reset value (0), target is 0.5
    EXPECT_NEAR(smoother.getCurrentValue(0), 0.0f, 1e-6f);
    EXPECT_NEAR(smoother.getTargetValue(0), 0.5f, 1e-6f);
}

TEST_F(SmoothedValueTest, SetSampleRateAndTime) {
    SmoothedValue<float, SmootherType::OnePole, 1> smoother;
    smoother.prepare(1, 1000);
    smoother.setTimeMs(10);
    // To change sample rate and time, call prepare and setTimeMs again with new values
    smoother.prepare(1, 2000);
    smoother.setTimeMs(20);
    smoother.reset();
    smoother.setTarget(1.0f);
    float last = 0.0f;
    for (int i = 0; i < 200; ++i) {
        last = smoother.getNextValue(0);
    }
    EXPECT_TRUE(last > 0.99f);
}

// Multichannel tests
TEST_F(SmoothedValueTest, OnePoleMultiChannel) {
    constexpr size_t numChannels = 4;
    SmoothedValue<float, SmootherType::OnePole, 1> smoother;
    smoother.prepare(numChannels, 1000);
    smoother.setTimeMs(10);
    smoother.reset();
    // Set different targets for each channel
    for (size_t ch = 0; ch < numChannels; ++ch) {
        smoother.setTarget(ch, static_cast<float>(ch + 1));
    }
    float last[numChannels] = {0};
    for (int i = 0; i < 100; ++i) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            last[ch] = smoother.getNextValue(ch);
        }
    }
    for (size_t ch = 0; ch < numChannels; ++ch) {
        EXPECT_NEAR(last[ch], static_cast<float>(ch + 1), 1e-2f);
    }
}

TEST_F(SmoothedValueTest, LinearMultiChannel) {
    constexpr size_t numChannels = 3;
    LinearSmoother<float> smoother;
    smoother.prepare(numChannels, 1000);
    smoother.setTimeMs(10);
    smoother.reset();
    // Set different targets for each channel
    for (size_t ch = 0; ch < numChannels; ++ch) {
        smoother.setTarget(ch, static_cast<float>(ch + 2));
    }
    float last[numChannels] = {0};
    for (int i = 0; i < 10; ++i) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            last[ch] = smoother.getNextValue(ch);
        }
    }
    for (size_t ch = 0; ch < numChannels; ++ch) {
        EXPECT_NEAR(last[ch], static_cast<float>(ch + 2), 1e-3f);
    }
}

    
} // namespace Jonssonic