#include <gtest/gtest.h>
#include <jonssonic/effects/delay.h>
using namespace jnsc::effects;

class DelayTest : public ::testing::Test {
  protected:
    static constexpr size_t numChannels = 2;
    static constexpr size_t blockSize = 8;
    static constexpr float sampleRate = 48000.0f;
    Delay<float> delay;
    void SetUp() override { delay.prepare(numChannels, blockSize, sampleRate); }
};

TEST_F(DelayTest, PrepareInitializes) {
    EXPECT_EQ(delay.getNumChannels(), numChannels);
    EXPECT_FLOAT_EQ(delay.getSampleRate(), sampleRate);
}

TEST_F(DelayTest, ResetWorks) {
    delay.reset();
    SUCCEED();
}

TEST_F(DelayTest, ProcessBlockWorks) {
    float input[numChannels][blockSize] = {};
    float output[numChannels][blockSize] = {};
    const float* inPtrs[numChannels] = {input[0], input[1]};
    float* outPtrs[numChannels] = {output[0], output[1]};
    delay.processBlock(inPtrs, outPtrs, blockSize);
    for (size_t ch = 0; ch < numChannels; ++ch)
        for (size_t i = 0; i < blockSize; ++i)
            EXPECT_FLOAT_EQ(output[ch][i], 0.0f);
}

TEST_F(DelayTest, SettersWork) {
    delay.setDelayMs(250.0f, true);
    delay.setFeedback(0.5f, true);
    delay.setDamping(0.3f, true);
    delay.setPingPong(0.7f, true);
    delay.setModDepth(0.4f, true);
    SUCCEED();
}
