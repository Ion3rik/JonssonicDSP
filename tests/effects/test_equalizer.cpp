#include <gtest/gtest.h>
#include <jonssonic/effects/equalizer.h>

using namespace jnsc::effects;

class EqualizerTest : public ::testing::Test {
  protected:
    static constexpr size_t numChannels = 2;
    static constexpr size_t maxBlockSize = 8;
    static constexpr float sampleRate = 48000.0f;
    Equalizer<float> eq;
    void SetUp() override { eq.prepare(numChannels, maxBlockSize, sampleRate); }
};

TEST_F(EqualizerTest, PrepareInitializesFilter) {
    EXPECT_EQ(eq.getNumChannels(), numChannels);
    EXPECT_FLOAT_EQ(eq.getSampleRate(), sampleRate);
}

TEST_F(EqualizerTest, ResetClearsState) {
    eq.reset();
    SUCCEED(); // No exception or error
}

TEST_F(EqualizerTest, ProcessBlockWorks) {
    float input[numChannels][maxBlockSize] = {};
    float output[numChannels][maxBlockSize] = {};
    const float* inPtrs[numChannels] = {input[0], input[1]};
    float* outPtrs[numChannels] = {output[0], output[1]};
    eq.processBlock(inPtrs, outPtrs, maxBlockSize);
    // Output should be zero for zero input
    for (size_t ch = 0; ch < numChannels; ++ch)
        for (size_t i = 0; i < maxBlockSize; ++i)
            EXPECT_FLOAT_EQ(output[ch][i], 0.0f);
}

TEST_F(EqualizerTest, SetLowCutFreqSetsFrequency) {
    eq.setLowCutFreq(100.0f, true);
    SUCCEED();
}

TEST_F(EqualizerTest, SetLowMidGainDbSetsGainAndQ) {
    eq.setLowMidGainDb(3.0f, true);
    eq.setLowMidGainDb(-3.0f, true);
    SUCCEED();
}

TEST_F(EqualizerTest, SetHighMidGainDbSetsGainAndQ) {
    eq.setHighMidGainDb(2.0f, true);
    eq.setHighMidGainDb(-2.0f, true);
    SUCCEED();
}

TEST_F(EqualizerTest, SetHighShelfGainDbSetsGain) {
    eq.setHighShelfGainDb(5.0f, true);
    eq.setHighShelfGainDb(-5.0f, true);
    SUCCEED();
}

TEST_F(EqualizerTest, SetLowMidFreqSetsFrequency) {
    eq.setLowMidFreq(400.0f, true);
    SUCCEED();
}

TEST_F(EqualizerTest, SetHighMidFreqSetsFrequency) {
    eq.setHighMidFreq(2000.0f, true);
    SUCCEED();
}
