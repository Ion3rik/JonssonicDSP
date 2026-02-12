#include <gtest/gtest.h>
#include <jonssonic/effects/distortion.h>
using namespace jnsc::effects;

class DistortionTest : public ::testing::Test {
  protected:
    static constexpr size_t numChannels = 2;
    static constexpr size_t blockSize = 8;
    static constexpr float sampleRate = 48000.0f;
    Distortion<float> dist;
    void SetUp() override { dist.prepare(numChannels, blockSize, sampleRate); }
};

TEST_F(DistortionTest, PrepareInitializes) {
    EXPECT_EQ(dist.getNumChannels(), numChannels);
    EXPECT_FLOAT_EQ(dist.getSampleRate(), sampleRate);
}

TEST_F(DistortionTest, ResetWorks) {
    dist.reset();
    SUCCEED();
}

TEST_F(DistortionTest, ProcessBlockWorks) {
    float input[numChannels][blockSize] = {};
    float output[numChannels][blockSize] = {};
    const float* inPtrs[numChannels] = {input[0], input[1]};
    float* outPtrs[numChannels] = {output[0], output[1]};
    dist.processBlock(inPtrs, outPtrs, blockSize);
    for (size_t ch = 0; ch < numChannels; ++ch)
        for (size_t i = 0; i < blockSize; ++i)
            EXPECT_FLOAT_EQ(output[ch][i], 0.0f);
}

TEST_F(DistortionTest, SettersWork) {
    dist.setDriveDb(6.0f, true);
    dist.setAsymmetry(0.5f, true);
    dist.setShape(0.8f, true);
    dist.setToneFrequency(8000.0f);
    dist.setMix(0.7f, true);
    dist.setOutputGainDb(3.0f, true);
    dist.setOversamplingEnabled(true);
    SUCCEED();
}
