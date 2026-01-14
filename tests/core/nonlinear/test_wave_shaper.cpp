// Jonssonic - WaveShaper unit tests
// SPDX-License-Identifier: MIT

#include <cmath>
#include <gtest/gtest.h>
#include <jonssonic/core/nonlinear/wave_shaper.h>
#include <jonssonic/utils/math_utils.h>

using namespace jnsc;
using namespace jnsc::utils;
TEST(WaveShaper, HardClip) {
    WaveShaper<float, WaveShaperType::HardClip> shaper;
    EXPECT_FLOAT_EQ(shaper.processSample(0.5f), 0.5f);
    EXPECT_FLOAT_EQ(shaper.processSample(-0.5f), -0.5f);
    EXPECT_FLOAT_EQ(shaper.processSample(2.0f), 1.0f);
    EXPECT_FLOAT_EQ(shaper.processSample(-2.0f), -1.0f);
}

TEST(WaveShaper, Atan) {
    WaveShaper<float, WaveShaperType::Atan> shaper;
    EXPECT_FLOAT_EQ(shaper.processSample(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(shaper.processSample(1.0f), std::atan(1.0f) * inv_atan_1<float>);
    EXPECT_FLOAT_EQ(shaper.processSample(-1.0f), std::atan(-1.0f) * inv_atan_1<float>);
}
TEST(WaveShaper, Cubic) {
    WaveShaper<float, WaveShaperType::Cubic> shaper;
    EXPECT_FLOAT_EQ(shaper.processSample(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(shaper.processSample(1.0f), 1.0f - (1.0f / 3.0f));
    EXPECT_FLOAT_EQ(shaper.processSample(-1.0f), -1.0f + (1.0f / 3.0f));
}
TEST(WaveShaper, Dynamic) {
    WaveShaper<float, WaveShaperType::Dynamic> shaper;
    float shape = 5.0f;
    EXPECT_FLOAT_EQ(shaper.processSample(0.0f, shape), 0.0f);
    EXPECT_FLOAT_EQ(shaper.processSample(1.0f, shape),
                    1.0f / std::pow(1.0f + std::pow(1.0f, shape), 1.0f / shape));
    EXPECT_FLOAT_EQ(shaper.processSample(-1.0f, shape),
                    -1.0f / std::pow(1.0f + std::pow(1.0f, shape), 1.0f / shape));
}
TEST(WaveShaper, Tanh) {
    WaveShaper<float, WaveShaperType::Tanh> shaper;
    EXPECT_FLOAT_EQ(shaper.processSample(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(shaper.processSample(1.0f), std::tanh(1.0f));
    EXPECT_FLOAT_EQ(shaper.processSample(-1.0f), std::tanh(-1.0f));
}

TEST(WaveShaper, FullWaveRectifier) {
    WaveShaper<float, WaveShaperType::FullWaveRectifier> shaper;
    EXPECT_FLOAT_EQ(shaper.processSample(1.0f), 1.0f);
    EXPECT_FLOAT_EQ(shaper.processSample(-1.0f), 1.0f);
    EXPECT_FLOAT_EQ(shaper.processSample(0.0f), 0.0f);
}

TEST(WaveShaper, HalfWaveRectifier) {
    WaveShaper<float, WaveShaperType::HalfWaveRectifier> shaper;
    EXPECT_FLOAT_EQ(shaper.processSample(1.0f), 1.0f);
    EXPECT_FLOAT_EQ(shaper.processSample(-1.0f), 0.0f);
    EXPECT_FLOAT_EQ(shaper.processSample(0.0f), 0.0f);
}

TEST(WaveShaper, Custom) {
    auto fn = [](float x) { return x * x; };
    WaveShaper<float, WaveShaperType::Custom> shaper(fn);
    EXPECT_FLOAT_EQ(shaper.processSample(2.0f), 4.0f);
    EXPECT_FLOAT_EQ(shaper.processSample(-3.0f), 9.0f);
}
