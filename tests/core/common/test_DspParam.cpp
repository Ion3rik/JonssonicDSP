// Jonssonic - A C++ audio DSP library
// Unit tests for DspParam
// SPDX-License-Identifier: MIT


#include <gtest/gtest.h>
#include "Jonssonic/core/common/DspParam.h"


namespace Jonssonic {

class DspParamTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
    float sampleRate = 1000.0f;
    float timeMs = 10.0f;
    float modTimeMs = 10.0f;
};

TEST_F(DspParamTest, OnePoleBaseSmoothing) {
    DspParam<float> param;
    param.prepare(sampleRate, timeMs);
    param.reset();
    param.setTarget(1.0f);
    float last = 0.0f;
    for (int i = 0; i < 100; ++i) {
        last = param.getNextValue();
    }
    EXPECT_TRUE(last > 0.99f);
}

TEST_F(DspParamTest, LinearBaseSmoothing) {
    DspParam<float, SmootherType::Linear> param;
    param.prepare(sampleRate, timeMs);
    param.reset();
    param.setTarget(1.0f);
    float val = 0.0f;
    for (int i = 0; i < 10; ++i) {
        val = param.getNextValue();
    }
    EXPECT_NEAR(val, 1.0f, 1e-3f);
}

TEST_F(DspParamTest, AdditiveModRaw) {
    DspParam<float> param;
    param.prepare(sampleRate, timeMs);
    param.reset(0.5);
    float result = param.applyAdditiveMod(0.25f, false);
    EXPECT_NEAR(result, 0.5f + 0.25f, 1e-6f);
}

TEST_F(DspParamTest, AdditiveModSmoothed) {
    DspParam<float, SmootherType::OnePole, 1, SmootherType::OnePole, 1> param;
    param.prepare(sampleRate, timeMs, modTimeMs);
    param.reset(0.5);
    float result = 0.0f;
    for (int i = 0; i < 100; ++i) {
        result = param.applyAdditiveMod(0.25f, true);
    }
    EXPECT_TRUE(result > 0.74f);
}

TEST_F(DspParamTest, MultiplicativeModRaw) {
    DspParam<float> param;
    param.prepare(sampleRate, timeMs);
    param.reset(0.5);
    float result = param.applyMultiplicativeMod(2.0f, false);
    EXPECT_NEAR(result, 1.0f, 1e-6f);
}

TEST_F(DspParamTest, MultiplicativeModSmoothed) {
    DspParam<float, SmootherType::OnePole, 1, SmootherType::OnePole, 1> param;
    param.prepare(sampleRate, timeMs, modTimeMs);
    param.reset(0.5);
    float result = 0.0f;
    for (int i = 0; i < 100; ++i) {
        result = param.applyMultiplicativeMod(2.0f, true);
    }
    EXPECT_TRUE(result > 0.99f);
}

} // namespace Jonssonic
