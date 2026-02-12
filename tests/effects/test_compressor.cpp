#include <gtest/gtest.h>
#include <jonssonic/effects/compressor.h>
using namespace jnsc::effects;

class CompressorTest : public ::testing::Test {
  protected:
    static constexpr size_t numChannels = 2;
    static constexpr size_t blockSize = 8;
    static constexpr float sampleRate = 48000.0f;
    Compressor<float> comp;
    void SetUp() override { comp.prepare(numChannels, sampleRate); }
};

TEST_F(CompressorTest, PrepareInitializes) {
    EXPECT_EQ(comp.getNumChannels(), numChannels);
    EXPECT_FLOAT_EQ(comp.getSampleRate(), sampleRate);
    EXPECT_TRUE(comp.isPrepared());
}

TEST_F(CompressorTest, ResetWorks) {
    comp.reset();
    SUCCEED();
}

TEST_F(CompressorTest, ProcessBlockWorks) {
    float input[numChannels][blockSize] = {};
    float detector[numChannels][blockSize] = {};
    float output[numChannels][blockSize] = {};
    const float* inPtrs[numChannels] = {input[0], input[1]};
    const float* detPtrs[numChannels] = {detector[0], detector[1]};
    float* outPtrs[numChannels] = {output[0], output[1]};
    comp.processBlock(inPtrs, detPtrs, outPtrs, blockSize);
    for (size_t ch = 0; ch < numChannels; ++ch)
        for (size_t i = 0; i < blockSize; ++i)
            EXPECT_FLOAT_EQ(output[ch][i], 0.0f);
}

TEST_F(CompressorTest, SettersWork) {
    comp.setThreshold(-18.0f, true);
    comp.setAttackTime(5.0f, true);
    comp.setReleaseTime(50.0f, true);
    comp.setRatio(2.5f, true);
    comp.setKnee(3.0f, true);
    comp.setOutputGain(2.0f, true);
    SUCCEED();
}

TEST_F(CompressorTest, GetGainReductionWorks) {
    float gr = comp.getGainReduction();
    EXPECT_GE(gr, 0.0f);
}
