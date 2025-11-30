// Jonssonic - WaveShaper unit tests
// SPDX-License-Identifier: MIT

#include <gtest/gtest.h>
#include "../../../include/Jonssonic/core/nonlinear/WaveShaper.h"
#include <cmath>

using namespace Jonssonic;

TEST(WaveShaper, HardClip)
{
    WaveShaper<float, WaveShaperType::HardClip> shaper;
    EXPECT_FLOAT_EQ(shaper.processSample(0.5f), 0.5f);
    EXPECT_FLOAT_EQ(shaper.processSample(-0.5f), -0.5f);
    EXPECT_FLOAT_EQ(shaper.processSample(2.0f), 1.0f);
    EXPECT_FLOAT_EQ(shaper.processSample(-2.0f), -1.0f);
}

TEST(WaveShaper, SoftClip)
{
    WaveShaper<float, WaveShaperType::SoftClip> shaper;
    EXPECT_FLOAT_EQ(shaper.processSample(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(shaper.processSample(1.0f), std::atan(1.0f));
    EXPECT_FLOAT_EQ(shaper.processSample(-1.0f), std::atan(-1.0f));
}

TEST(WaveShaper, Tanh)
{
    WaveShaper<float, WaveShaperType::Tanh> shaper;
    EXPECT_FLOAT_EQ(shaper.processSample(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(shaper.processSample(1.0f), std::tanh(1.0f));
    EXPECT_FLOAT_EQ(shaper.processSample(-1.0f), std::tanh(-1.0f));
}

TEST(WaveShaper, FullWaveRectifier)
{
    WaveShaper<float, WaveShaperType::FullWaveRectifier> shaper;
    EXPECT_FLOAT_EQ(shaper.processSample(1.0f), 1.0f);
    EXPECT_FLOAT_EQ(shaper.processSample(-1.0f), 1.0f);
    EXPECT_FLOAT_EQ(shaper.processSample(0.0f), 0.0f);
}

TEST(WaveShaper, HalfWaveRectifier)
{
    WaveShaper<float, WaveShaperType::HalfWaveRectifier> shaper;
    EXPECT_FLOAT_EQ(shaper.processSample(1.0f), 1.0f);
    EXPECT_FLOAT_EQ(shaper.processSample(-1.0f), 0.0f);
    EXPECT_FLOAT_EQ(shaper.processSample(0.0f), 0.0f);
}

TEST(WaveShaper, Custom)
{
    auto fn = [](float x) { return x * x; };
    WaveShaper<float, WaveShaperType::Custom> shaper(fn);
    EXPECT_FLOAT_EQ(shaper.processSample(2.0f), 4.0f);
    EXPECT_FLOAT_EQ(shaper.processSample(-3.0f), 9.0f);
}
