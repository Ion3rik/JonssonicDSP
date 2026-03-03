#include <gtest/gtest.h>
#include <jonssonic/effects/chorus.h>
using namespace jnsc::effects;

class ChorusTest : public ::testing::Test {
  protected:
    static constexpr size_t numChannels = 2;
    static constexpr size_t blockSize = 8;
    static constexpr float sampleRate = 48000.0f;
    Chorus<float> chorus;
    void SetUp() override { chorus.prepare(numChannels, sampleRate); }
};

TEST_F(ChorusTest, PrepareInitializes) {
    EXPECT_EQ(chorus.getNumChannels(), numChannels);
    EXPECT_FLOAT_EQ(chorus.getSampleRate(), sampleRate);
}

TEST_F(ChorusTest, ResetWorks) {
    chorus.reset();
    SUCCEED();
}

TEST_F(ChorusTest, ProcessBlockWorks) {
    float input[numChannels][blockSize] = {};
    float output[numChannels][blockSize] = {};
    const float* inPtrs[numChannels] = {input[0], input[1]};
    float* outPtrs[numChannels] = {output[0], output[1]};
    chorus.processBlock(inPtrs, outPtrs, blockSize);
    for (size_t ch = 0; ch < numChannels; ++ch)
        for (size_t i = 0; i < blockSize; ++i)
            EXPECT_FLOAT_EQ(output[ch][i], 0.0f);
}

TEST_F(ChorusTest, SettersWork) {
    chorus.setRate(1.0f, true);
    chorus.setDepth(0.5f, true);
    chorus.setNumVoices(4, true);
    chorus.setDelayMs(20.0f, true);
    chorus.setSpread(0.7f, true);
    SUCCEED();
}
