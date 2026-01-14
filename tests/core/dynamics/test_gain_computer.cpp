
#include <cmath>
#include <gtest/gtest.h>
#include <jonssonic/core/dynamics/gain_computer.h>
#include <jonssonic/utils/math_utils.h>

using namespace jnsc;
using namespace jnsc::utils;

// =============================================================================
// CompressorPolicy Tests
// =============================================================================
class CompressorPolicyTest : public ::testing::Test {
  protected:
    float sampleRate = 48000.0f;
    GainComputer<float, CompressorPolicy<float>> gc1;
    GainComputer<float, CompressorPolicy<float>> gc2;
    void SetUp() override {
        gc1.prepare(1, 48000.0f);
        gc2.prepare(1, 48000.0f);
        gc1.setControlSmoothingTime(0.0_samples);
        gc2.setControlSmoothingTime(0.0_samples);
    }
    void TearDown() override {}
};

TEST_F(CompressorPolicyTest, NoCompressionBelowThreshold) {
    gc1.setThreshold(-10.0f, true);
    gc1.setRatio(4.0f, true);
    gc1.setKnee(0.0f, true);
    float inputLin = dB2Mag(-11.0f); // -11 dB
    float gainDb = gc1.processSample(0, inputLin);
    EXPECT_NEAR(gainDb, 0.0f, 1e-6f); // 0 dB gain expected
}

TEST_F(CompressorPolicyTest, CompressionAboveThreshold) {
    gc1.setThreshold(-10.0f, true);
    gc1.setRatio(4.0f, true);
    gc1.setKnee(0.0f, true);
    float inputLin = dB2Mag(0.0f); // 0 dB
    float gainDb = gc1.processSample(0, inputLin);
    // For 0 dB input, threshold -10 dB, ratio 4:1:
    // gain reduction = (threshold - input) * (1 - 1/ratio) = (-10 - 0) * (1 - 0.25) = -10 * 0.75 =
    // -7.5 dB
    float expectedGain = -7.5f;
    EXPECT_NEAR(gainDb, expectedGain, 1e-5f);
}

TEST_F(CompressorPolicyTest, SoftKneeTransition) {
    gc1.setThreshold(-10.0f, true);
    gc1.setRatio(4.0f, true);
    gc1.setKnee(10.0f, true); // 10 dB knee
    float ratio = 4.0f;
    float threshold = -10.0f;
    float knee = 10.0f;
    float oneMinusInvRatio = 1.0f - 1.0f / ratio;

    // At lower knee edge: input = threshold - knee/2, should be no compression (0 dB gain)
    float inputLower = dB2Mag(threshold - knee * 0.5f);
    float gainLower = gc1.processSample(0, inputLower);
    EXPECT_NEAR(gainLower, 0.0f, 1e-5f);

    // At upper knee edge: input = threshold + knee/2, should be full hard knee compression
    float inputUpper = dB2Mag(threshold + knee * 0.5f);
    float gainUpper = gc1.processSample(0, inputUpper);
    float expectedUpper = (threshold - (threshold + knee * 0.5f)) * oneMinusInvRatio;
    EXPECT_NEAR(gainUpper, expectedUpper, 1e-5f);

    // At threshold (center of knee): should be halfway between no compression and full compression
    float inputCenter = dB2Mag(threshold);
    float gainCenter = gc1.processSample(0, inputCenter);
    // Soft knee formula at center:
    float kneePos = 0.0f + knee * 0.5f; // over + halfKnee
    float safeKnee = knee + 1e-7f;      // epsilon fudge for float
    float expectedCenter = -oneMinusInvRatio * (kneePos * kneePos) / (2.0f * safeKnee);
    EXPECT_NEAR(gainCenter, expectedCenter, 1e-5f);
}
// =============================================================================
// LimiterPolicy Tests
// =============================================================================
class LimiterPolicyTest : public ::testing::Test {
  protected:
    float sampleRate = 48000.0f;
    GainComputer<float, LimiterPolicy<float>> gc;
    void SetUp() override {
        gc.prepare(1, 48000.0f);
        gc.setControlSmoothingTime(0.0_samples);
    }
    void TearDown() override {}
};

TEST_F(LimiterPolicyTest, LimiterEdges) {
    gc.setThreshold(-10.0f, true);
    float threshold = -10.0f;

    // Below threshold: no limiting
    float inputBelow = dB2Mag(-11.0f);
    float gainBelow = gc.processSample(0, inputBelow);
    EXPECT_NEAR(gainBelow, 0.0f, 1e-5f);

    // At threshold: no limiting
    float inputAt = dB2Mag(threshold);
    float gainAt = gc.processSample(0, inputAt);
    EXPECT_NEAR(gainAt, 0.0f, 1e-5f);

    // Above threshold: limiting (gain = threshold - input)
    float inputAbove = dB2Mag(-5.0f);
    float gainAbove = gc.processSample(0, inputAbove);
    float expectedAbove = threshold - (-5.0f);
    EXPECT_NEAR(gainAbove, expectedAbove, 1e-5f);
}
// =============================================================================
// GatePolicy Tests
// =============================================================================
class GatePolicyTest : public ::testing::Test {
  protected:
    float sampleRate = 48000.0f;
    GainComputer<float, GatePolicy<float>> gc;
    void SetUp() override {
        gc.prepare(1, 48000.0f);
        gc.setControlSmoothingTime(0.0_samples);
    }
    void TearDown() override {}
};

TEST_F(GatePolicyTest, GateEdges) {
    gc.setThreshold(-10.0f, true);
    float threshold = -10.0f;

    // Above threshold: no gating
    float inputAbove = dB2Mag(-9.0f);
    float gainAbove = gc.processSample(0, inputAbove);
    EXPECT_NEAR(gainAbove, 0.0f, 1e-5f);

    // At threshold: no gating
    float inputAt = dB2Mag(threshold);
    float gainAt = gc.processSample(0, inputAt);
    EXPECT_NEAR(gainAt, 0.0f, 1e-5f);

    // Below threshold: full mute (gain = -100 dB)
    float inputBelow = dB2Mag(-20.0f);
    float gainBelow = gc.processSample(0, inputBelow);
    EXPECT_NEAR(gainBelow, -100.0f, 1e-3f);
}

// =============================================================================
// ExpanderDownPolicy Tests
// =============================================================================
class ExpanderDownPolicyTest : public ::testing::Test {
  protected:
    float sampleRate = 48000.0f;
    GainComputer<float, ExpanderDownPolicy<float>> gc;
    void SetUp() override {
        gc.prepare(1, 48000.0f);
        gc.setControlSmoothingTime(0.0_samples);
    }
    void TearDown() override {}
};

TEST_F(ExpanderDownPolicyTest, SoftKneeEdgesAndCenter) {
    gc.setThreshold(-10.0f, true);
    gc.setRatio(4.0f, true);
    gc.setKnee(10.0f, true);
    float ratio = 4.0f;
    float threshold = -10.0f;
    float knee = 10.0f;
    float oneMinusInvRatio = 1.0f - 1.0f / ratio;

    // Lower knee edge: input = threshold + knee/2 (no expansion)
    float inputLower = dB2Mag(threshold + knee * 0.5f);
    float gainLower = gc.processSample(0, inputLower);
    EXPECT_NEAR(gainLower, 0.0f, 1e-5f);

    // Upper knee edge: input = threshold - knee/2 (full expansion)
    float inputUpper = dB2Mag(threshold - knee * 0.5f);
    float gainUpper = gc.processSample(0, inputUpper);
    float expectedUpper = (threshold - (threshold - knee * 0.5f)) * oneMinusInvRatio;
    expectedUpper = -expectedUpper; // Downward expansion is attenuation
    EXPECT_NEAR(gainUpper, expectedUpper, 1e-5f);

    // Center (threshold): soft knee formula
    float inputCenter = dB2Mag(threshold);
    float gainCenter = gc.processSample(0, inputCenter);
    float kneePos = 0.0f + knee * 0.5f;
    float safeKnee = knee + 1e-7f;
    float expectedCenter = -oneMinusInvRatio * (kneePos * kneePos) / (2.0f * safeKnee);
    EXPECT_NEAR(gainCenter, expectedCenter, 1e-5f);
}

class ExpanderUpPolicyTest : public ::testing::Test {
  protected:
    float sampleRate = 48000.0f;
    GainComputer<float, ExpanderUpPolicy<float>> gc;
    void SetUp() override {
        gc.prepare(1, 48000.0f);
        gc.setControlSmoothingTime(0.0_samples);
    }
    void TearDown() override {}
};

TEST_F(ExpanderUpPolicyTest, SoftKneeEdgesAndCenter) {
    gc.setThreshold(-10.0f, true);
    gc.setRatio(4.0f, true);
    gc.setKnee(10.0f, true);
    float ratio = 4.0f;
    float threshold = -10.0f;
    float knee = 10.0f;
    float oneMinusInvRatio = 1.0f - 1.0f / ratio;

    // Lower knee edge: input = threshold - knee/2 (no expansion)
    float inputLower = dB2Mag(threshold - knee * 0.5f);
    float gainLower = gc.processSample(0, inputLower);
    EXPECT_NEAR(gainLower, 0.0f, 1e-5f);

    // Upper knee edge: input = threshold + knee/2 (full expansion)
    float inputUpper = dB2Mag(threshold + knee * 0.5f);
    float gainUpper = gc.processSample(0, inputUpper);
    float expectedUpper = ((threshold + knee * 0.5f) - threshold) * oneMinusInvRatio;
    EXPECT_NEAR(gainUpper, expectedUpper, 1e-5f);

    // Center (threshold): soft knee formula
    float inputCenter = dB2Mag(threshold);
    float gainCenter = gc.processSample(0, inputCenter);
    float kneePos = 0.0f + knee * 0.5f;
    float safeKnee = knee + 1e-7f;
    float expectedCenter = (oneMinusInvRatio) * (kneePos * kneePos) / (2.0f * safeKnee);
    EXPECT_NEAR(gainCenter, expectedCenter, 1e-5f);
}