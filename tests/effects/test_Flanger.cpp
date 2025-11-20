// Unit tests for Jonssonic::Flanger
// Author: Jon Fagerström
#include <gtest/gtest.h>
#include "Jonssonic/effects/Flanger.h"
#include <vector>

using namespace Jonssonic;

TEST(FlangerTest, BasicMonoProcess)
{
    Flanger<float> flanger;
    flanger.prepare(1, 48000.0f);
    flanger.setRate(0.5f);
    flanger.setDepth(0.7f);
    flanger.setFeedback(0.5f);
    flanger.setCenterDelay(3.0f);
    flanger.setSpread(0.0f);

    constexpr size_t numSamples = 32;
    std::vector<float> input(numSamples, 0.0f);
    std::vector<float> output(numSamples, 0.0f);
    input[0] = 1.0f; // Impulse

    // Prepare pointers for processBlock
    std::vector<const float*> inPtrs = { input.data() };
    std::vector<float*> outPtrs = { output.data() };

    flanger.processBlock(inPtrs.data(), outPtrs.data(), numSamples);

    // Output should not be all zeros due to impulse response
    bool nonZero = false;
    for (float v : output) {
        if (std::abs(v) > 1e-6f) nonZero = true;
    }
    EXPECT_TRUE(nonZero);
}

TEST(FlangerTest, StereoImpulseResponse)
{
    Flanger<float> flanger;
    flanger.prepare(2, 44100.0f);
    flanger.setRate(1.0f);
    flanger.setDepth(1.0f);
    flanger.setFeedback(0.0f);
    flanger.setCenterDelay(5.0f);
    flanger.setSpread(1.0f);

    constexpr size_t numSamples = 64;
    std::vector<float> inputL(numSamples, 0.0f);
    std::vector<float> inputR(numSamples, 0.0f);
    std::vector<float> outputL(numSamples, 0.0f);
    std::vector<float> outputR(numSamples, 0.0f);
    inputL[0] = 1.0f; // Impulse left
    inputR[0] = 0.0f; // Silence right

    std::vector<const float*> inPtrs = { inputL.data(), inputR.data() };
    std::vector<float*> outPtrs = { outputL.data(), outputR.data() };

    flanger.processBlock(inPtrs.data(), outPtrs.data(), numSamples);

    // Check left output is non-zero
    bool nonZeroL = false;
    for (float v : outputL) {
        if (std::abs(v) > 1e-6f) nonZeroL = true;
    }
    EXPECT_TRUE(nonZeroL) << "Left channel output should not be all zeros";
    // Check right output is zero (no input, no feedback)
    bool nonZeroR = false;
    for (float v : outputR) {
        if (std::abs(v) > 1e-6f) nonZeroR = true;
    }
    EXPECT_FALSE(nonZeroR) << "Right channel output should be all zeros";
}
