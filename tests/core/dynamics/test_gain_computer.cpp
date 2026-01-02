#include <gtest/gtest.h>
#include <jonssonic/core/dynamics/gain_computer.h>
#include <cmath>

using namespace jonssonic::core::dynamics;

TEST(GainComputerCompressorTest, NoCompressionBelowThreshold) {
    GainComputer<float, GainType::Compressor> gc;
    gc.prepare(1, 48000.0f);
    gc.setThreshold(-10.0f, true);
    gc.setRatio(4.0f, true);
    gc.setKnee(0.0f, true);
    float input = std::pow(10.0f, -20.0f / 20.0f); // -20 dB
    float gain = gc.processSample(0, input);
    EXPECT_NEAR(gain, 0.0f, 1e-6f); // 0 dB gain expected
}

TEST(GainComputerCompressorTest, CompressionAboveThreshold) {
    GainComputer<float, GainType::Compressor> gc;
    gc.prepare(1, 48000.0f);
    gc.setThreshold(-10.0f, true);
    gc.setRatio(4.0f, true);
    gc.setKnee(0.0f, true);
    float input = std::pow(10.0f, 0.0f / 20.0f); // 0 dB
    float gain = gc.processSample(0, input);
    // For 0 dB input, threshold -10 dB, ratio 4:1:
    // gain reduction = (threshold - input) * (1 - 1/ratio) = (-10 - 0) * (1 - 0.25) = -10 * 0.75 = -7.5 dB
    float expectedGain = -7.5f;
    EXPECT_NEAR(gain, expectedGain, 1e-5f);
}

TEST(GainComputerCompressorTest, SoftKneeTransition) {
    GainComputer<float, GainType::Compressor> gc;
    gc.prepare(1, 48000.0f);
    gc.setThreshold(-10.0f, true);
    gc.setRatio(4.0f, true);
    gc.setKnee(10.0f, true); // 10 dB knee
    float input = std::pow(10.0f, -10.0f / 20.0f); // exactly at threshold
    float gain = gc.processSample(0, input);
    // Should be between no compression and full compression
    EXPECT_GT(gain, -7.5f);
    EXPECT_LT(gain, 1.0f);
}

TEST(GainComputerCompressorTest, HardKneeEqualsSoftKneeZero) {
    GainComputer<float, GainType::Compressor> gc1;
    gc1.prepare(1, 48000.0f);
    gc1.setThreshold(-10.0f, true);
    gc1.setRatio(4.0f, true);
    gc1.setKnee(0.0f, true);
    GainComputer<float, GainType::Compressor> gc2;
    gc2.prepare(1, 48000.0f);
    gc2.setThreshold(-10.0f, true);
    gc2.setRatio(4.0f, true);
    gc2.setKnee(0.0f, true);
    float input = std::pow(10.0f, 0.0f / 20.0f); // 0 dB
    float gain1 = gc1.processSample(0, input);
    float gain2 = gc2.processSample(0, input);
    EXPECT_FLOAT_EQ(gain1, gain2);
}
