#include <gtest/gtest.h>
#include <jonssonic/effects/flanger.h>
using namespace jnsc::effects;

class FlangerTest : public ::testing::Test {
  protected:
    static constexpr size_t numChannels = 2;
    static constexpr size_t blockSize = 8;
    static constexpr float sampleRate = 48000.0f;
    Flanger<float> flanger;
    void SetUp() override { flanger.prepare(numChannels, sampleRate); }
};

TEST_F(FlangerTest, PrepareInitializes) {
    EXPECT_EQ(flanger.getNumChannels(), numChannels);
    EXPECT_FLOAT_EQ(flanger.getSampleRate(), sampleRate);
}

TEST_F(FlangerTest, ResetWorks) {
    flanger.reset();
    SUCCEED();
}

TEST_F(FlangerTest, ProcessBlockWorks) {
    float input[numChannels][blockSize] = {};
    float output[numChannels][blockSize] = {};
    const float* inPtrs[numChannels] = {input[0], input[1]};
    float* outPtrs[numChannels] = {output[0], output[1]};
    flanger.processBlock(inPtrs, outPtrs, blockSize);
    for (size_t ch = 0; ch < numChannels; ++ch)
        for (size_t i = 0; i < blockSize; ++i)
            EXPECT_FLOAT_EQ(output[ch][i], 0.0f);
}

TEST_F(FlangerTest, SettersWork) {
    flanger.setRate(1.0f, true);
    flanger.setDepth(0.5f, true);
    flanger.setFeedback(0.3f, true);
    flanger.setDelayMs(2.0f, true);
    flanger.setSpread(0.2f, true);
    SUCCEED();
}
