#include <gtest/gtest.h>
#include <jonssonic/core/common/audio_buffer.h>
#include <jonssonic/effects/phaser.h>
#include <array>
#include <complex>
#include <vector>

using namespace jnsc;
using namespace jnsc::effects;

class PhaserTest : public ::testing::Test {
  protected:
    static constexpr size_t numChannels = 2;
    static constexpr size_t blockSize = 1024;
    static constexpr float sampleRate = 48000.0f;
    Phaser<float> phaser;
    void SetUp() override { phaser.prepare(numChannels, sampleRate); }
};

TEST_F(PhaserTest, PrepareInitializesAndSilenceRemainsSilent) {
    EXPECT_EQ(phaser.getNumChannels(), numChannels);
    EXPECT_FLOAT_EQ(phaser.getSampleRate(), sampleRate);
    AudioBuffer<float> buffer(numChannels, blockSize);
    phaser.processBlock(buffer.readPtrs(), buffer.writePtrs(), blockSize);
    for (size_t ch = 0; ch < numChannels; ++ch)
        for (size_t n = 0; n < blockSize; ++n)
            EXPECT_FLOAT_EQ(buffer[ch][n], 0.0f);
}

TEST_F(PhaserTest, BlockBoundariesAndInPlaceProcessingPreserveFeedbackAndModulation) {
    Phaser<float> splitPhaser(numChannels, sampleRate);
    phaser.setSpread(0.8f, true);
    splitPhaser.setSpread(0.8f, true);
    phaser.setFeedback(-0.8f, true);
    splitPhaser.setFeedback(-0.8f, true);
    // Exercise control smoothing as well as persistent feedback history.
    phaser.setCenterFrequency(2000.0f);
    splitPhaser.setCenterFrequency(2000.0f);
    AudioBuffer<float> input(numChannels, blockSize);
    AudioBuffer<float> expected(numChannels, blockSize);
    AudioBuffer<float> actual(numChannels, blockSize);
    for (size_t ch = 0; ch < numChannels; ++ch)
        for (size_t n = 0; n < blockSize; ++n)
            input[ch][n] = actual[ch][n] = 0.2f * std::sin(float(n) * 0.13f) + (n == 0 ? 0.5f : 0.0f);
    phaser.processBlock(input.readPtrs(), expected.writePtrs(), blockSize);
    for (size_t offset = 0; offset < blockSize;) {
        size_t count = std::min(size_t(37), blockSize - offset);
        const float* in[] = {actual.writeChannelPtr(0) + offset, actual.writeChannelPtr(1) + offset};
        float* out[] = {actual.writeChannelPtr(0) + offset, actual.writeChannelPtr(1) + offset};
        splitPhaser.processBlock(in, out, count);
        offset += count;
    }
    for (size_t ch = 0; ch < numChannels; ++ch)
        for (size_t n = 0; n < blockSize; ++n)
            EXPECT_FLOAT_EQ(actual[ch][n], expected[ch][n]);
}

TEST_F(PhaserTest, ResetAndReprepareClearHistory) {
    AudioBuffer<float> input(numChannels, blockSize);
    AudioBuffer<float> first(numChannels, blockSize);
    AudioBuffer<float> second(numChannels, blockSize);
    input[0][0] = input[1][0] = 1.0f;
    phaser.processBlock(input.readPtrs(), first.writePtrs(), blockSize);
    phaser.reset();
    phaser.processBlock(input.readPtrs(), second.writePtrs(), blockSize);
    for (size_t ch = 0; ch < numChannels; ++ch)
        for (size_t n = 0; n < blockSize; ++n)
            EXPECT_FLOAT_EQ(first[ch][n], second[ch][n]);
    phaser.prepare(numChannels, sampleRate);
    phaser.processBlock(input.readPtrs(), second.writePtrs(), blockSize);
    for (size_t ch = 0; ch < numChannels; ++ch)
        for (size_t n = 0; n < blockSize; ++n)
            EXPECT_FLOAT_EQ(first[ch][n], second[ch][n]);
}

TEST_F(PhaserTest, DepthAndSpreadChangeTheSweepWithoutChannelCrosstalk) {
    Phaser<float> staticPhaser(numChannels, sampleRate);
    staticPhaser.setDepth(0.0f, true);
    phaser.setSpread(1.0f, true);
    AudioBuffer<float> input(numChannels, blockSize);
    AudioBuffer<float> moving(numChannels, blockSize);
    AudioBuffer<float> fixed(numChannels, blockSize);
    input[0][0] = input[1][0] = 1.0f;
    phaser.processBlock(input.readPtrs(), moving.writePtrs(), blockSize);
    staticPhaser.processBlock(input.readPtrs(), fixed.writePtrs(), blockSize);
    EXPECT_FLOAT_EQ(moving[1][0], fixed[1][0]);
    float modulationDifference = 0.0f, stereoDifference = 0.0f;
    for (size_t n = 0; n < blockSize; ++n) {
        modulationDifference += std::abs(moving[0][n] - fixed[0][n]);
        stereoDifference += std::abs(moving[0][n] - moving[1][n]);
    }
    EXPECT_GT(modulationDifference, 0.1f);
    EXPECT_GT(stereoDifference, 0.1f);
    phaser.reset();
    input[1][0] = 0.0f;
    phaser.processBlock(input.readPtrs(), moving.writePtrs(), blockSize);
    for (size_t n = 0; n < blockSize; ++n)
        EXPECT_FLOAT_EQ(moving[1][n], 0.0f);
}

TEST(PhaserResponseTest, StaticSweepProducesTwoNotches) {
    constexpr double fs = 48000.0, center = 1000.0;
    constexpr double pi = 3.14159265358979323846;
    Phaser<double> phaser(1, fs);
    phaser.setDepth(0.0, true);
    phaser.setFeedback(0.0, true);
    phaser.setCenterFrequency(center, true);
    constexpr size_t length = 16384;
    std::vector<double> impulse(length, 0.0);
    impulse[0] = 1.0;
    const double* in[] = {impulse.data()};
    double* out[] = {impulse.data()};
    phaser.processBlock(in, out, length);
    // Four first-order phase shifts cancel the dry path at these warped frequencies.
    for (double ratio : {std::tan(pi / 8.0), std::tan(3.0 * pi / 8.0)}) {
        double frequency = fs / pi * std::atan(std::tan(pi * center / fs) * ratio);
        std::complex<double> response(0.0, 0.0);
        for (size_t n = 0; n < length; ++n)
            response += impulse[n] * std::polar(1.0, -2.0 * pi * frequency * n / fs);
        EXPECT_LT(std::abs(response), 1e-8);
    }
}

TEST(PhaserStabilityTest, ExtremeControlsRemainFiniteAtSupportedSampleRates) {
    for (float fs : {1000.0f, 44100.0f, 48000.0f, 96000.0f, 768000.0f}) {
        for (float feedback : {-1.0f, 1.0f}) {
            Phaser<float> phaser(2, fs);
            phaser.setRate(10.0f, true);
            phaser.setDepth(1.0f, true);
            phaser.setSpread(1.0f, true);
            phaser.setFeedback(feedback, true);
            phaser.setCenterFrequency(5000.0f, true);
            std::array<float, 2> input{}, output{};
            const float* in[] = {&input[0], &input[1]};
            float* out[] = {&output[0], &output[1]};
            for (size_t n = 0; n < 48000; ++n) {
                input[0] = input[1] = 0.1f * std::sin(float(n) * 0.17f);
                phaser.processBlock(in, out, 1);
                ASSERT_TRUE(std::isfinite(output[0]));
                ASSERT_TRUE(std::isfinite(output[1]));
                ASSERT_LT(std::abs(output[0]), 10.0f) << "sample rate=" << fs;
                ASSERT_LT(std::abs(output[1]), 10.0f) << "sample rate=" << fs;
            }
        }
    }
}
