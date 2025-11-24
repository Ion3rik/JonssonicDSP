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
    param.prepare(1, timeMs, sampleRate);
    param.reset();
    param.setTarget(1.0f, 0);
    float last = 0.0f;
    for (int i = 0; i < 100; ++i) {
        last = param.getNextValue(0);
    }
    EXPECT_TRUE(last > 0.99f);
}

TEST_F(DspParamTest, LinearBaseSmoothing) {
    DspParam<float, SmootherType::Linear> param;
    param.prepare(1, timeMs, sampleRate);
    param.reset();
    param.setTarget(1.0f, 0);
    float val = 0.0f;
    for (int i = 0; i < 10; ++i) {
        val = param.getNextValue(0);
    }
    EXPECT_NEAR(val, 1.0f, 1e-3f);
}

TEST_F(DspParamTest, AdditiveModRaw) {
    DspParam<float, SmootherType::None, 1, SmootherType::None, 1> param;
    param.prepare(1, timeMs, sampleRate);
    param.reset();
    param = 0.5f; // set current value forcibly
    float result = param.applyAdditiveMod(0.25f, 0);
    EXPECT_NEAR(result, 0.5f + 0.25f, 1e-6f);
}

TEST_F(DspParamTest, AdditiveModSmoothed) {
    DspParam<float, SmootherType::OnePole, 1, SmootherType::OnePole, 1> param;
    param.prepare(1, timeMs, sampleRate, modTimeMs);
    param.reset();
    param.setTarget(0.5f, 0);
    float result = 0.0f;
    for (int i = 0; i < 100; ++i) {
        result = param.applyAdditiveMod(0.25f, 0);
    }
    EXPECT_TRUE(result > 0.74f);
}

TEST_F(DspParamTest, MultiplicativeModRaw) {
    DspParam<float, SmootherType::None, 1, SmootherType::None, 1> param;
    param.prepare(1, timeMs, sampleRate);
    param.reset();
    param = 0.5f; // set current value forcibly
    float result = param.applyMultiplicativeMod(2.0f, 0);
    EXPECT_NEAR(result, 1.0f, 1e-6f);
}

TEST_F(DspParamTest, MultiplicativeModSmoothed) {
    DspParam<float, SmootherType::OnePole, 1, SmootherType::OnePole, 1> param;
    param.prepare(1, timeMs, sampleRate, modTimeMs);
    param.reset();
    param.setTarget(0.5f, 0);
    float result = 0.0f;
    for (int i = 0; i < 100; ++i) {
        result = param.applyMultiplicativeMod(2.0f, 0);
    }
    EXPECT_TRUE(result > 0.99f);
}

// Multichannel tests
TEST_F(DspParamTest, OnePoleBaseSmoothingMultiChannel) {
    constexpr size_t numChannels = 4;
    DspParam<float> param;
    param.prepare(numChannels, timeMs, sampleRate);
    param.reset();
    for (size_t ch = 0; ch < numChannels; ++ch) {
        param.setTarget(static_cast<float>(ch + 1), ch);
    }
    float last[numChannels] = {0};
    for (int i = 0; i < 100; ++i) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            last[ch] = param.getNextValue(ch);
        }
    }
    for (size_t ch = 0; ch < numChannels; ++ch) {
        EXPECT_NEAR(last[ch], static_cast<float>(ch + 1), 1e-2f);
    }
}

TEST_F(DspParamTest, AdditiveModMultiChannel) {
    constexpr size_t numChannels = 3;
    DspParam<float, SmootherType::OnePole, 1, SmootherType::OnePole, 1> param;
    param.prepare(numChannels, timeMs, sampleRate, modTimeMs);
    param.reset();
    for (size_t ch = 0; ch < numChannels; ++ch) {
        param.setTarget(1.0f, ch);
    }
    float result[numChannels] = {0};
    for (int i = 0; i < 100; ++i) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            result[ch] = param.applyAdditiveMod(static_cast<float>(ch), ch);
        }
    }
    for (size_t ch = 0; ch < numChannels; ++ch) {
        EXPECT_NEAR(result[ch], 1.0f + static_cast<float>(ch), 1e-2f);
    }
}

TEST_F(DspParamTest, MultiplicativeModMultiChannel) {
    constexpr size_t numChannels = 2;
    DspParam<float, SmootherType::OnePole, 1, SmootherType::OnePole, 1> param;
    param.prepare(numChannels, timeMs, sampleRate, modTimeMs);
    param.reset();
    for (size_t ch = 0; ch < numChannels; ++ch) {
        param.setTarget(2.0f, ch);
    }
    float result[numChannels] = {0};
    for (int i = 0; i < 100; ++i) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            result[ch] = param.applyMultiplicativeMod(2.0f, ch);
        }
    }
    for (size_t ch = 0; ch < numChannels; ++ch) {
        EXPECT_NEAR(result[ch], 4.0f, 1e-2f);
    }
}

} // namespace Jonssonic
