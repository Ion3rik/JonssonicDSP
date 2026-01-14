#include <cmath>
#include <gtest/gtest.h>
#include <jonssonic/core/common/quantities.h>
#include <jonssonic/core/dynamics/gain_smoother.h>
#include <jonssonic/utils/math_utils.h>
#include <vector>

using namespace jnsc;
using namespace jnsc::utils;

class GainSmootherTest : public ::testing::Test {
  protected:
    void SetUp() override {
        sampleRate = 48000.0f;
        numChannels = 2;
        tolerance = 1e-4f;
    }
    float sampleRate;
    size_t numChannels;
    Time<float> attackMs = 10.0_ms;
    Time<float> releaseMs = 50.0_ms;

    float tolerance;
};

TEST_F(GainSmootherTest, AttackPhase) {
    GainSmoother<float, GainSmootherType::AttackRelease> smoother;
    smoother.prepare(1, sampleRate);
    smoother.setAttackTime(attackMs, true);
    smoother.setReleaseTime(releaseMs, true);
    smoother.reset(0.0f);
    float targetDb = -12.0f;
    float last = 0.0f;
    for (int i = 0; i < 3000; ++i) {
        float out = mag2dB(smoother.processSample(0, targetDb));
        EXPECT_LE(out, last + tolerance);
        last = out;
    }
    EXPECT_NEAR(last, targetDb, 0.1f);
}

TEST_F(GainSmootherTest, ReleasePhase) {
    GainSmoother<float, GainSmootherType::AttackRelease> smoother;
    smoother.prepare(1, sampleRate);
    smoother.setAttackTime(attackMs, true);
    smoother.setReleaseTime(releaseMs, true);
    smoother.reset(-12.0f);
    float targetDb = 0.0f;
    float last = -12.0f;
    for (int i = 0; i < 15000; ++i) {
        float out = mag2dB(smoother.processSample(0, targetDb));
        EXPECT_GE(out, last - tolerance);
        last = out;
    }
    EXPECT_NEAR(last, targetDb, 0.1f);
}

TEST_F(GainSmootherTest, ResetWorks) {
    GainSmoother<float, GainSmootherType::AttackRelease> smoother;
    smoother.prepare(1, sampleRate);
    smoother.reset(-6.0f);
    float out = mag2dB(smoother.processSample(0, -6.0f));
    EXPECT_NEAR(out, -6.0f, tolerance);
}

TEST_F(GainSmootherTest, MultiChannel) {
    GainSmoother<float, GainSmootherType::AttackRelease> smoother;
    smoother.prepare(2, sampleRate);
    smoother.setAttackTime(attackMs, true);
    smoother.setReleaseTime(releaseMs, true);
    smoother.reset(0.0f);
    float targetDb0 = -6.0f;
    float targetDb1 = -12.0f;
    for (int i = 0; i < 3000; ++i) {
        float out0 = mag2dB(smoother.processSample(0, targetDb0));
        float out1 = mag2dB(smoother.processSample(1, targetDb1));
        EXPECT_LE(out0, 0.0f + tolerance);
        EXPECT_LE(out1, 0.0f + tolerance);
    }
    float out0 = mag2dB(smoother.processSample(0, targetDb0));
    float out1 = mag2dB(smoother.processSample(1, targetDb1));
    EXPECT_NEAR(out0, targetDb0, 0.1f);
    EXPECT_NEAR(out1, targetDb1, 0.1f);
}
