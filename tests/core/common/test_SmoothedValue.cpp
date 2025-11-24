// Jonssonic - A C++ audio DSP library
// Unit tests for SmoothedValue
// Author: Jon Fagerström
// Update: 22.11.2025

#include <gtest/gtest.h>
#include "Jonssonic/core/common/SmoothedValue.h"

namespace Jonssonic {

class SmoothedValueTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SmoothedValueTest, OnePoleOrder1BasicSmoothing) {
    SmoothedValue<float, SmootherType::OnePole, 1> smoother;
    smoother.prepare(1000, 10); // 1kHz, 10ms
    smoother.reset(0.0f);
    smoother.setTarget(1.0f);
    float last = 0.0f;
    for (int i = 0; i < 100; ++i) {
        last = smoother.getNextValue();
    }
    EXPECT_TRUE(last > 0.99f);
}

TEST_F(SmoothedValueTest, OnePoleOrder2Cascaded) {
    SmoothedValue<float, SmootherType::OnePole, 2> smoother;
    smoother.prepare(1000, 10);
    smoother.reset(0.0f);
    smoother.setTarget(1.0f);
    float last = 0.0f;
    for (int i = 0; i < 200; ++i) {
        last = smoother.getNextValue();
    }
    EXPECT_TRUE(last > 0.99f);
}

TEST_F(SmoothedValueTest, LinearBasic) {
    SmoothedValue<float, SmootherType::Linear> smoother;
    smoother.prepare(1000, 10);
    smoother.reset(0.0f);
    smoother.setTarget(1.0f);
    float val = 0.0f;
    for (int i = 0; i < 10; ++i) {
        val = smoother.getNextValue();
    }
    EXPECT_NEAR(val, 1.0f, 1e-3f);
}

TEST_F(SmoothedValueTest, LinearReachesTargetExactly) {
    SmoothedValue<float, SmootherType::Linear> smoother;
    smoother.prepare(1000, 20);
    smoother.reset(0.0f);
    smoother.setTarget(2.0f);
    float val = 0.0f;
    for (int i = 0; i < 20; ++i) {
        val = smoother.getNextValue();
    }
    EXPECT_NEAR(val, 2.0f, 1e-3f);
}

TEST_F(SmoothedValueTest, ResetSetsCurrentAndTarget) {
    SmoothedValue<float, SmootherType::OnePole, 1> smoother;
    smoother.prepare(1000, 10);
    smoother.reset(0.5f);
    EXPECT_NEAR(smoother.getCurrentValue(), 0.5f, 1e-6f);
    EXPECT_NEAR(smoother.getTargetValue(), 0.5f, 1e-6f);
}

TEST_F(SmoothedValueTest, SetSampleRateAndTime) {
    SmoothedValue<float, SmootherType::OnePole, 1> smoother;
    smoother.prepare(1000, 10);
    smoother.setSampleRate(2000);
    smoother.setTimeMs(20);
    smoother.reset(0.0f);
    smoother.setTarget(1.0f);
    float last = 0.0f;
    for (int i = 0; i < 200; ++i) {
        last = smoother.getNextValue();
    }
    EXPECT_TRUE(last > 0.99f);
}

    
} // namespace Jonssonic