#include <gtest/gtest.h>
#include <jonssonic/effects/reverb.h>
using namespace jnsc::effects;

class ReverbTest : public ::testing::Test {
  protected:
    static constexpr size_t numChannels = 2;
    static constexpr float sampleRate = 48000.0f;
    static constexpr size_t blockSize = 8;
    Reverb<float> reverb;
    void SetUp() override { reverb.prepare(numChannels, sampleRate); }
};

TEST_F(ReverbTest, PrepareInitializes) {
    EXPECT_EQ(reverb.getNumChannels(), numChannels);
    EXPECT_FLOAT_EQ(reverb.getSampleRate(), sampleRate);
}

TEST_F(ReverbTest, ResetWorks) {
    reverb.reset();
    SUCCEED();
}

TEST_F(ReverbTest, ProcessBlockWorks) {
    float input[numChannels][blockSize] = {};
    float output[numChannels][blockSize] = {};
    const float* inPtrs[numChannels] = {input[0], input[1]};
    float* outPtrs[numChannels] = {output[0], output[1]};
    reverb.processBlock(inPtrs, outPtrs, blockSize);
    for (size_t ch = 0; ch < numChannels; ++ch)
        for (size_t i = 0; i < blockSize; ++i)
            EXPECT_FLOAT_EQ(output[ch][i], 0.0f);
}

TEST_F(ReverbTest, SettersWork) {
    reverb.setReverbTimeLowS(2.5f, true);
    reverb.setReverbTimeHighS(1.2f, true);
    reverb.setDiffusion(0.7f, true);
    reverb.setPreDelayTimeMs(10.0f, true);
    reverb.setLowCutFreqHz(1200.0f);
    reverb.setModulationRateHz(2.0f);
    reverb.setModulationDepth(0.5f);
    reverb.setDampingCrossoverFreqHz(800.0f);
    SUCCEED();
}
