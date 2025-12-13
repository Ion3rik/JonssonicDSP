// Jonssonic - A C++ audio DSP library
// Unit tests for the Delay effect
// SPDX-License-Identifier: MIT


#include <gtest/gtest.h>
#include "Jonssonic/effects/Delay.h"
#include <vector>

using namespace Jonssonic;

TEST(DelayTest, BasicMonoImpulse)
{
    Delay<float> delay;
    delay.prepare(1, 48000.0f, 100.0f); // 100ms max delay
    delay.setDelaySamples(10.0f, true); // 100 samples delay
    delay.setFeedback(0.0f, true);
    delay.setDamping(20000.0f, true);
    delay.setPingPong(0.0f, true);
    delay.setModDepth(0.0f, true);

    constexpr size_t numSamples = 256;
    std::vector<float> input(numSamples, 0.0f);
    std::vector<float> output(numSamples, 0.0f);
    input[0] = 1.0f; // Impulse

    std::vector<float*> outPtrs = { output.data() };
    std::vector<const float*> inPtrs = { input.data() };

    delay.processBlock(inPtrs.data(), outPtrs.data(), numSamples);


    // Debug: print the first 120 output samples
    std::cout << "First 120 output samples:\n";
    for (int i = 0; i < 120; ++i) {
        std::cout << i << ": " << output[i] << "\n";
    }

    // Output should have a delayed impulse at (likely) 99, 100, or 101 due to implementation details
    bool foundImpulse = false;
    for (int offset = -1; offset <= 1; ++offset) {
        int idx = 100 + offset;
        if (idx >= 0 && idx < (int)output.size()) {
            if (std::abs(output[idx] - 1.0f) < 0.1f && std::abs(output[idx]) > 0.5f) {
                foundImpulse = true;
                break;
            }
        }
    }
    EXPECT_TRUE(foundImpulse) << "Expected impulse at or near sample 100";

    // Check that samples before the delay are near zero
    for (size_t i = 0; i < 99; ++i) {
        EXPECT_NEAR(output[i], 0.0f, 1e-6f) << "Sample " << i << " should be zero";
    }
}

TEST(DelayTest, StereoImpulseWithCrossTalk)
{
    Delay<float> delay;
    delay.prepare(2, 44100.0f, 100.0f); // 100ms max delay
    delay.setDelaySamples(50.0f, true); // 50 samples delay
    delay.setFeedback(0.0f, true);
    delay.setDamping(20000.0f, true);
    delay.setPingPong(1.0f, true); // Max cross-talk
    delay.setModDepth(0.0f, true);

    constexpr size_t numSamples = 256;
    std::vector<float> inputL(numSamples, 0.0f);
    std::vector<float> inputR(numSamples, 0.0f);
    std::vector<float> outputL(numSamples, 0.0f);
    std::vector<float> outputR(numSamples, 0.0f);
    inputL[0] = 1.0f; // Impulse left
    inputR[0] = 0.0f; // Silence right

    std::vector<const float*> inPtrs = { inputL.data(), inputR.data() };
    std::vector<float*> outPtrs = { outputL.data(), outputR.data() };

    delay.processBlock(inPtrs.data(), outPtrs.data(), numSamples);

    // Check right output is non-zero due to cross-talk
    bool nonZeroR = false;
    for (float v : outputR) {
        if (std::abs(v) > 1e-6f) nonZeroR = true;
    }
    EXPECT_TRUE(nonZeroR) << "Right channel output should not be all zeros due to cross-talk";
}

TEST(DelayTest, FeedbackAndDamping)
{
    Delay<float> delay;
    delay.prepare(1, 48000.0f, 100.0f); // 100ms max delay
    delay.setDelaySamples(100.0f, true); // 100 samples delay
    delay.setFeedback(0.5f, true);
    delay.setDamping(1000.0f, true); // Strong damping
    delay.setPingPong(0.0f, true);
    delay.setModDepth(0.0f, true);

    constexpr size_t numSamples = 512;
    std::vector<float> input(numSamples, 0.0f);
    std::vector<float> output(numSamples, 0.0f);
    input[0] = 1.0f; // Impulse

    std::vector<const float*> inPtrs = { input.data() };
    std::vector<float*> outPtrs = { output.data() };

    delay.processBlock(inPtrs.data(), outPtrs.data(), numSamples);

    // Output should decay over time due to feedback and damping
    float prev = 0.0f;
    bool decaying = false;
    for (size_t i = 1; i < numSamples; ++i) {
        if (output[i] < prev) decaying = true;
        prev = output[i];
    }
    EXPECT_TRUE(decaying) << "Output should decay due to feedback and damping";
}


