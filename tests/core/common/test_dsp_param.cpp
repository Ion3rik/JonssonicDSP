// Jonssonic - A Modular Realtime C++ Audio DSP Library
// Unit tests for DspParam
// SPDX-License-Identifier: MIT

#include <gtest/gtest.h>
#include <jonssonic/core/common/dsp_param.h>
#include <jonssonic/core/common/quantities.h>

using namespace jnsc;

class DspParamTest : public ::testing::Test {
  protected:
    void SetUp() override {}
    void TearDown() override {}
    float sampleRate = 1000.0f;
    Time<float> timeMs = Time<float>::Milliseconds(10.0f);
};

TEST_F(DspParamTest, OnePoleBaseSmoothing) {
    DspParam<float, SmootherType::OnePole> param;
    param.prepare(1, sampleRate);
    param.setSmoothingTime(timeMs);
    param.reset();
    param.setTarget(0, 1.0f);
    float last = 0.0f;
    for (int i = 0; i < 100; ++i) {
        last = param.getNextValue(0);
    }
    EXPECT_NEAR(last, 1.0f, 1e-2f);
}

TEST_F(DspParamTest, LinearBaseSmoothing) {
    DspParam<float, SmootherType::Linear> param;
    param.prepare(1, sampleRate);
    param.setSmoothingTime(0.0_s);
    param.reset();
    param.setTarget(0, 1.0f);
    float val = 0.0f;
    for (int i = 0; i < 10; ++i) {
        val = param.getNextValue(0);
    }
    EXPECT_NEAR(val, 1.0f, 1e-3f);
}

TEST_F(DspParamTest, AdditiveModRaw) {
    DspParam<float, SmootherType::None> param;
    param.prepare(1, sampleRate);
    param.setSmoothingTime(0.0_s);
    param.reset();
    param.setTarget(0.5f, true); // set current value forcibly
    float result = param.getNextValue(0) + 0.25f;
    EXPECT_NEAR(result, 0.5f + 0.25f, 1e-6f);
}

TEST_F(DspParamTest, AdditiveModSmoothed) {
    DspParam<float, SmootherType::OnePole> param;
    param.prepare(1, sampleRate);
    param.setSmoothingTime(0.0_s);
    param.reset();
    param.setTarget(0, 0.5f);
    float result = 0.0f;
    for (int i = 0; i < 100; ++i) {
        result = param.getNextValue(0) + 0.25f;
    }
    EXPECT_TRUE(result > 0.74f);
}

TEST_F(DspParamTest, MultiplicativeModRaw) {
    DspParam<float, SmootherType::None> param;
    param.prepare(1, sampleRate);
    param.setSmoothingTime(0.0_s);
    param.reset();
    param.setTarget(0.5f, true); // set current value forcibly
    float result = param.getNextValue(0) * 2.0f;
    EXPECT_NEAR(result, 1.0f, 1e-6f);
}

TEST_F(DspParamTest, MultiplicativeModSmoothed) {
    DspParam<float, SmootherType::OnePole> param;
    param.prepare(1, sampleRate);
    param.setSmoothingTime(0.0_s);
    param.reset();
    param.setTarget(0, 0.5f);
    float result = 0.0f;
    for (int i = 0; i < 100; ++i) {
        result = param.getNextValue(0) * 2.0f;
    }
    EXPECT_TRUE(result > 0.99f);
}

// Multichannel tests
TEST_F(DspParamTest, OnePoleBaseSmoothingMultiChannel) {
    constexpr size_t numChannels = 4;
    DspParam<float> param;
    param.prepare(numChannels, sampleRate);
    param.setSmoothingTime(timeMs);
    param.reset();
    for (size_t ch = 0; ch < numChannels; ++ch) {
        param.setTarget(ch, static_cast<float>(ch + 1));
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
    DspParam<float, SmootherType::OnePole> param;
    param.prepare(numChannels, sampleRate);
    param.setSmoothingTime(0.0_s);
    param.reset();
    for (size_t ch = 0; ch < numChannels; ++ch) {
        param.setTarget(ch, 1.0f);
    }
    float result[numChannels] = {0};
    for (int i = 0; i < 100; ++i) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            result[ch] = param.getNextValue(ch) + static_cast<float>(ch);
        }
    }
    for (size_t ch = 0; ch < numChannels; ++ch) {
        EXPECT_NEAR(result[ch], 1.0f + static_cast<float>(ch), 1e-2f);
    }
}

TEST_F(DspParamTest, MultiplicativeModMultiChannel) {
    constexpr size_t numChannels = 2;
    DspParam<float, SmootherType::OnePole> param;
    param.prepare(numChannels, sampleRate);
    param.setSmoothingTime(0.0_s);
    param.reset();
    for (size_t ch = 0; ch < numChannels; ++ch) {
        param.setTarget(ch, 2.0f);
    }
    float result[numChannels] = {0};
    for (int i = 0; i < 100; ++i) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            result[ch] = param.getNextValue(ch) * 2.0f;
        }
    }
    for (size_t ch = 0; ch < numChannels; ++ch) {
        EXPECT_NEAR(result[ch], 4.0f, 1e-2f);
    }
}
