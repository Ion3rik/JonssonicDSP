#include <gtest/gtest.h>
#include <Jonssonic/core/dynamics/GainSmoother.h>
#include <Jonssonic/utils/MathUtils.h>
#include <vector>
#include <cmath>

using namespace Jonssonic::core;
using Jonssonic::mag2dB;
using Jonssonic::dB2Mag;

constexpr float kSampleRate = 48000.0f;
constexpr size_t kNumChannels = 2;
constexpr float kAttackMs = 10.0f;
constexpr float kReleaseMs = 50.0f;
constexpr float kTolerance = 1e-4f;

TEST(GainSmoother, AttackPhase) {
    GainSmoother<float, GainSmootherType::AttackRelease> smoother;
    smoother.prepare(1, kSampleRate);
    smoother.setAttackTime(kAttackMs, true);
    smoother.setReleaseTime(kReleaseMs, true);
    smoother.reset(0.0f);
    float targetDb = -12.0f;
    float last = 0.0f;
    for (int i = 0; i < 100; ++i) {
        float out = mag2dB(smoother.processSample(0, targetDb));
        EXPECT_LE(out, last + kTolerance);
        last = out;
    }
    EXPECT_NEAR(last, targetDb, 0.1f);
}

TEST(GainSmoother, ReleasePhase) {
    GainSmoother<float, GainSmootherType::AttackRelease> smoother;
    smoother.prepare(1, kSampleRate);
    smoother.setAttackTime(kAttackMs, true);
    smoother.setReleaseTime(kReleaseMs, true);
    smoother.reset(-12.0f);
    float targetDb = 0.0f;
    float last = -12.0f;
    for (int i = 0; i < 200; ++i) {
        float out = mag2dB(smoother.processSample(0, targetDb));
        EXPECT_GE(out, last - kTolerance);
        last = out;
    }
    EXPECT_NEAR(last, targetDb, 0.1f);
}

TEST(GainSmoother, ResetWorks) {
    GainSmoother<float, GainSmootherType::AttackRelease> smoother;
    smoother.prepare(1, kSampleRate);
    smoother.reset(-6.0f);
    float out = mag2dB(smoother.processSample(0, -6.0f));
    EXPECT_NEAR(out, -6.0f, kTolerance);
}

TEST(GainSmoother, MultiChannel) {
    GainSmoother<float, GainSmootherType::AttackRelease> smoother;
    smoother.prepare(2, kSampleRate);
    smoother.setAttackTime(kAttackMs, true);
    smoother.setReleaseTime(kReleaseMs, true);
    smoother.reset(0.0f);
    float targetDb0 = -6.0f;
    float targetDb1 = -12.0f;
    for (int i = 0; i < 100; ++i) {
        float out0 = mag2dB(smoother.processSample(0, targetDb0));
        float out1 = mag2dB(smoother.processSample(1, targetDb1));
        EXPECT_LE(out0, 0.0f + kTolerance);
        EXPECT_LE(out1, 0.0f + kTolerance);
    }
    float out0 = mag2dB(smoother.processSample(0, targetDb0));
    float out1 = mag2dB(smoother.processSample(1, targetDb1));
    EXPECT_NEAR(out0, targetDb0, 0.1f);
    EXPECT_NEAR(out1, targetDb1, 0.1f);
}
