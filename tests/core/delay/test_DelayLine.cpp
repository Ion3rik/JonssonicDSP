// Jonssonic - A C++ audio DSP library
// Unit tests for the DelayLine class
// Author: Jon Fagerström
// Update: 18.11.2025

#include <gtest/gtest.h>
#include <cmath>
#include "Jonssonic/core/common/Interpolators.h"
#include "Jonssonic/core/delays/DelayLine.h"

namespace Jonssonic {

class DelayLineTest : public ::testing::Test {
protected:
    void SetUp() override {
        delayLine.setDelay(2);
        blockDelayLine.setDelay(2);
        zeroDelayLine.setDelay(0);
        maxDelayLine.setDelay(7);
        smallDelayLine.setDelay(4);
        clampDelayLine.setDelay(2);
        oversizeDelayLine.setDelay(100);
        quadDelayLine.setDelay(2);
        surroundDelayLine.setDelay(3);
    }

    void TearDown() override {

    }
    
    // Helper: Process samples one at a time and verify output
    void processSamplesAndVerify(const float* left, const float* right, 
                                 const float* expectedLeft, const float* expectedRight,
                                 size_t numSamples, size_t numInputChannels = 2) {
        for (size_t i = 0; i < numSamples; ++i) {
            const float* inputPtrs[2] = { &left[i], &right[i] };
            delayLine.processSample(inputPtrs, outputPtrs, numInputChannels);
            EXPECT_FLOAT_EQ(output[0], expectedLeft[i]) << "Sample " << i << " left channel";
            EXPECT_FLOAT_EQ(output[1], expectedRight[i]) << "Sample " << i << " right channel";
        }
    }
    
    // Helper: Process with modulation and verify
    void processModulatedAndVerify(const float* left, const float* right,
                                   float modValue, const float* expectedLeft, 
                                   const float* expectedRight, size_t numSamples) {
        float modLeft[1] = {modValue};
        float modRight[1] = {modValue};
        const float* modulation[2] = { modLeft, modRight };
        
        for (size_t i = 0; i < numSamples; ++i) {
            const float* inputPtrs[2] = { &left[i], &right[i] };
            delayLine.processSample(inputPtrs, outputPtrs, modulation, 2);
            EXPECT_FLOAT_EQ(output[0], expectedLeft[i]) << "Sample " << i << " left channel";
            EXPECT_FLOAT_EQ(output[1], expectedRight[i]) << "Sample " << i << " right channel";
        }
    }
    
    // Test fixture members - DelayLine instances
    DelayLine<float, NearestInterpolator<float>> delayLine{2, 1024};          // Standard stereo
    DelayLine<float, NearestInterpolator<float>> blockDelayLine{2, 1024};     // For block processing tests
    DelayLine<float, NearestInterpolator<float>> zeroDelayLine{2, 1024};      // Zero delay test
    DelayLine<float, NearestInterpolator<float>> maxDelayLine{1, 8};          // Maximum delay test
    DelayLine<float, NearestInterpolator<float>> smallDelayLine{1, 8};        // Wraparound test
    DelayLine<float, NearestInterpolator<float>> clampDelayLine{2, 1024};     // Modulation clamping test
    DelayLine<float, NearestInterpolator<float>> oversizeDelayLine{1, 8};     // Delay exceeds buffer test
    DelayLine<float, NearestInterpolator<float>> quadDelayLine{4, 1024};      // Quad (4-channel)
    DelayLine<float, NearestInterpolator<float>> surroundDelayLine{6, 1024};  // 5.1 surround (6-channel)
    
    // Common input buffers
    float leftChannel[4] = {0.0f, 10.0f, 20.0f, 30.0f};
    float rightChannel[4] = {0.0f, 20.0f, 40.0f, 60.0f};
    const float* stereoInput[2] = { leftChannel, rightChannel };
    
    // Common expected outputs for 2-sample delay
    float expectedLeft[4] = {0.0f, 0.0f, 0.0f, 10.0f};
    float expectedRight[4] = {0.0f, 0.0f, 0.0f, 20.0f};
    
    // Common output buffers (reusable across tests)
    float output[2] = {0.0f, 0.0f};
    float* outputPtrs[2] = { &output[0], &output[1] };
    
    // Modulation buffers
    float modZero[1] = {0.0f};
    float modPositive[1] = {1.0f};
    float modNegative[1] = {-1.0f};
};

// Tests for simple sample processing
TEST_F(DelayLineTest, ProcessSingleSampleFixedDelaySteroToStereo) {
    processSamplesAndVerify(leftChannel, rightChannel, expectedLeft, expectedRight, 4);
}

// Test mono input to stereo output (channel duplication)
TEST_F(DelayLineTest, ProcessSingleSampleFixedMonoToStereo) {
    for (int i = 0; i < 4; ++i) {
        const float* monoInput[1] = { &leftChannel[i] };
        delayLine.processSample(monoInput, outputPtrs, 1);
        EXPECT_FLOAT_EQ(output[0], expectedLeft[i]) << "Sample " << i << " left";
        EXPECT_FLOAT_EQ(output[1], expectedLeft[i]) << "Sample " << i << " right (dup)";
    }
}

// Test modulated delay with no modulation (should match fixed delay)
TEST_F(DelayLineTest, ProcessSingleSampleModulatedDelayNoModulation) {
    processModulatedAndVerify(leftChannel, rightChannel, 0.0f, expectedLeft, expectedRight, 4);
}

// Test modulated delay with positive modulation (increased delay)
TEST_F(DelayLineTest, ProcessSingleSampleModulatedDelayPositiveModulation) {
    float expected3Left[5] = {0.0f, 0.0f, 0.0f, 0.0f, 10.0f};
    float expected3Right[5] = {0.0f, 0.0f, 0.0f, 0.0f, 20.0f};
    const float* modulation[2] = { modPositive, modPositive };

    for (int i = 0; i < 5; ++i) {
        const float* inputPtrs[2] = { &leftChannel[i % 4], &rightChannel[i % 4] };
        delayLine.processSample(inputPtrs, outputPtrs, modulation, 2);
        EXPECT_FLOAT_EQ(output[0], expected3Left[i]) << "Sample " << i << " left";
        EXPECT_FLOAT_EQ(output[1], expected3Right[i]) << "Sample " << i << " right";
    }
}

// Test modulated delay with negative modulation (decreased delay)
TEST_F(DelayLineTest, ProcessSingleSampleModulatedDelayNegativeModulation) {
    float expected1Left[4] = {0.0f, 0.0f, 10.0f, 20.0f};
    float expected1Right[4] = {0.0f, 0.0f, 20.0f, 40.0f};
    processModulatedAndVerify(leftChannel, rightChannel, -1.0f, expected1Left, expected1Right, 4);
}

// Test modulated delay with fractional modulation (tests interpolation)
TEST_F(DelayLineTest, ProcessSingleSampleModulatedDelayFractionalModulation) {
    float modHalf[1] = {0.5f};
    const float* modulation[2] = { modHalf, modHalf };

    for (int i = 0; i < 4; ++i) {
        const float* inputPtrs[2] = { &leftChannel[i], &rightChannel[i] };
        delayLine.processSample(inputPtrs, outputPtrs, modulation, 2);
        EXPECT_GE(output[0], 0.0f) << "Sample " << i << " left channel should be non-negative";
        EXPECT_GE(output[1], 0.0f) << "Sample " << i << " right channel should be non-negative";
    }
}

// Test modulated delay mono to stereo with modulation
TEST_F(DelayLineTest, ProcessSingleSampleModulatedDelayMonoToStereo) {
    const float* monoModulation[1] = { modZero };

    for (int i = 0; i < 4; ++i) {
        const float* monoInput[1] = { &leftChannel[i] };
        delayLine.processSample(monoInput, outputPtrs, monoModulation, 1);
        EXPECT_FLOAT_EQ(output[0], expectedLeft[i]) << "Sample " << i << " left";
        EXPECT_FLOAT_EQ(output[1], expectedLeft[i]) << "Sample " << i << " right (dup)";
    }
}

// Test modulated delay with varying modulation signal
TEST_F(DelayLineTest, ProcessSingleSampleModulatedDelayVaryingModulation) {
    float modulationValues[4] = {0.0f, 0.0f, 1.0f, -1.0f};
    
    for (int i = 0; i < 4; ++i) {
        float modLeft[1] = {modulationValues[i]};
        float modRight[1] = {modulationValues[i]};
        const float* modulation[2] = { modLeft, modRight };
        const float* inputPtrs[2] = { &leftChannel[i], &rightChannel[i] };
        
        delayLine.processSample(inputPtrs, outputPtrs, modulation, 2);
        EXPECT_FALSE(std::isnan(output[0])) << "Sample " << i << " left should not be NaN";
        EXPECT_FALSE(std::isnan(output[1])) << "Sample " << i << " right should not be NaN";
        EXPECT_TRUE(std::isfinite(output[0])) << "Sample " << i << " left should be finite";
        EXPECT_TRUE(std::isfinite(output[1])) << "Sample " << i << " right should be finite";
    }
}

// Test block processing with fixed delay
TEST_F(DelayLineTest, ProcessBlockFixedDelay) {
    // Prepare output buffers
    float outputLeft[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float outputRight[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float* outputPtrs[2] = { outputLeft, outputRight };
    
    // Process entire block at once
    blockDelayLine.processBlock(stereoInput, outputPtrs, 2, 4);
    
    // Expected outputs with 2-sample delay
    float expectedLeft[4] = {0.0f, 0.0f, 0.0f, 10.0f};
    float expectedRight[4] = {0.0f, 0.0f, 0.0f, 20.0f};
    
    for (int i = 0; i < 4; ++i)
    {
        EXPECT_FLOAT_EQ(outputLeft[i], expectedLeft[i]) << "Sample " << i << " left channel";
        EXPECT_FLOAT_EQ(outputRight[i], expectedRight[i]) << "Sample " << i << " right channel";
    }
}

// Test block processing matches sample-by-sample processing
TEST_F(DelayLineTest, ProcessBlockMatchesSampleBySample) {
    // Create two identical delay lines for this comparison test
    DelayLine<float, NearestInterpolator<float>> sampleDelayLine{2, 1024};
    DelayLine<float, NearestInterpolator<float>> blockCompareDelayLine{2, 1024};
    sampleDelayLine.setDelay(3);
    blockCompareDelayLine.setDelay(3);
    
    // Prepare output buffers
    float sampleOutputLeft[8] = {0.0f};
    float sampleOutputRight[8] = {0.0f};
    float blockOutputLeft[8] = {0.0f};
    float blockOutputRight[8] = {0.0f};
    
    // Test input (8 samples)
    float testInputLeft[8] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f};
    float testInputRight[8] = {10.0f, 20.0f, 30.0f, 40.0f, 50.0f, 60.0f, 70.0f, 80.0f};
    const float* testInput[2] = { testInputLeft, testInputRight };
    
    // Process sample-by-sample
    for (int i = 0; i < 8; ++i)
    {
        const float* inputPtrs[2] = { &testInputLeft[i], &testInputRight[i] };
        float* outputPtrs[2] = { &sampleOutputLeft[i], &sampleOutputRight[i] };
        sampleDelayLine.processSample(inputPtrs, outputPtrs, 2);
    }
    
    // Process as block
    float* blockOutputPtrs[2] = { blockOutputLeft, blockOutputRight };
    blockCompareDelayLine.processBlock(testInput, blockOutputPtrs, 2, 8);
    
    // Verify they match
    for (int i = 0; i < 8; ++i)
    {
        EXPECT_FLOAT_EQ(blockOutputLeft[i], sampleOutputLeft[i]) 
            << "Sample " << i << " left channel should match";
        EXPECT_FLOAT_EQ(blockOutputRight[i], sampleOutputRight[i]) 
            << "Sample " << i << " right channel should match";
    }
}

// Test modulated block processing
TEST_F(DelayLineTest, ProcessBlockModulatedDelay) {
    // Prepare output buffers
    float outputLeft[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float outputRight[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float* outputPtrs[2] = { outputLeft, outputRight };
    
    // Modulation: constant +1 sample (total delay = 3)
    float modLeft[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float modRight[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    const float* modulation[2] = { modLeft, modRight };
    
    // Process entire block at once
    blockDelayLine.processBlock(stereoInput, outputPtrs, modulation, 2, 4);
    
    // Expected outputs with 3-sample delay (base 2 + modulation 1)
    float expectedLeft[4] = {0.0f, 0.0f, 0.0f, 0.0f};  // Need more samples to see output
    float expectedRight[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    
    for (int i = 0; i < 4; ++i)
    {
        EXPECT_FLOAT_EQ(outputLeft[i], expectedLeft[i]) << "Sample " << i << " left channel";
        EXPECT_FLOAT_EQ(outputRight[i], expectedRight[i]) << "Sample " << i << " right channel";
    }
}

// Test buffer wraparound
TEST_F(DelayLineTest, BufferWraparound) {
    float localOutput[1] = {0.0f};
    float* outputPtr[1] = { localOutput };
    
    for (int i = 0; i < 20; ++i)
    {
        float input[1] = {static_cast<float>(i)};
        const float* inputPtr[1] = { input };
        smallDelayLine.processSample(inputPtr, outputPtr, 1);
        
        if (i >= 4)
        {
            EXPECT_FLOAT_EQ(localOutput[0], static_cast<float>(i - 4)) 
                << "Sample " << i << " should be delayed by 4";
        }
    }
}

// Edge case: Zero delay (immediate pass-through)
TEST_F(DelayLineTest, ZeroDelay) {
    for (int i = 0; i < 4; ++i) {
        const float* inputPtrs[2] = { &leftChannel[i], &rightChannel[i] };
        zeroDelayLine.processSample(inputPtrs, outputPtrs, 2);
        EXPECT_FLOAT_EQ(output[0], leftChannel[i]) << "Sample " << i << " left should pass through";
        EXPECT_FLOAT_EQ(output[1], rightChannel[i]) << "Sample " << i << " right should pass through";
    }
}

// Edge case: Maximum delay (buffer size - 1)
TEST_F(DelayLineTest, MaximumDelay) {
    float localOutput[1] = {0.0f};
    float* outputPtr[1] = { localOutput };
    
    float testSamples[10] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f};
    
    for (int i = 0; i < 10; ++i)
    {
        const float* inputPtr[1] = { &testSamples[i] };
        maxDelayLine.processSample(inputPtr, outputPtr, 1);
        
        if (i >= 7)
        {
            EXPECT_FLOAT_EQ(localOutput[0], testSamples[i - 7]) 
                << "Sample " << i << " should be delayed by 7";
        }
        else
        {
            EXPECT_FLOAT_EQ(localOutput[0], 0.0f) 
                << "Sample " << i << " should be zero (buffer not filled yet)";
        }
    }
}

// Edge case: Delay exceeds buffer size (should be clamped)
TEST_F(DelayLineTest, DelayExceedsBufferSize) {
    float localOutput[1] = {0.0f};
    float* outputPtr[1] = { localOutput };
    
    float testSamples[12] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f};
    
    for (int i = 0; i < 12; ++i)
    {
        const float* inputPtr[1] = { &testSamples[i] };
        oversizeDelayLine.processSample(inputPtr, outputPtr, 1);
        
        EXPECT_FLOAT_EQ(localOutput[0], testSamples[i]) 
            << "Sample " << i << " should have 0 delay (clamped to buffer size wraps around)";
    }
}

// Multi-channel: 4-channel (quad) processing
TEST_F(DelayLineTest, QuadChannelProcessing) {
    // 4-channel input
    float channel1[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    float channel2[4] = {10.0f, 20.0f, 30.0f, 40.0f};
    float channel3[4] = {100.0f, 200.0f, 300.0f, 400.0f};
    float channel4[4] = {1000.0f, 2000.0f, 3000.0f, 4000.0f};
    
    float output[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float* outputPtrs[4] = { &output[0], &output[1], &output[2], &output[3] };
    
    // Expected outputs with 2-sample delay
    float expectedCh1[4] = {0.0f, 0.0f, 1.0f, 2.0f};
    float expectedCh2[4] = {0.0f, 0.0f, 10.0f, 20.0f};
    float expectedCh3[4] = {0.0f, 0.0f, 100.0f, 200.0f};
    float expectedCh4[4] = {0.0f, 0.0f, 1000.0f, 2000.0f};
    
    for (int i = 0; i < 4; ++i) {
        const float* inputPtrs[4] = { &channel1[i], &channel2[i], &channel3[i], &channel4[i] };
        quadDelayLine.processSample(inputPtrs, outputPtrs, 4);
        
        EXPECT_FLOAT_EQ(output[0], expectedCh1[i]) << "Sample " << i << " channel 1";
        EXPECT_FLOAT_EQ(output[1], expectedCh2[i]) << "Sample " << i << " channel 2";
        EXPECT_FLOAT_EQ(output[2], expectedCh3[i]) << "Sample " << i << " channel 3";
        EXPECT_FLOAT_EQ(output[3], expectedCh4[i]) << "Sample " << i << " channel 4";
    }
}

// Multi-channel: Mono to 4-channel duplication
TEST_F(DelayLineTest, MonoToQuadChannelDuplication) {
    float output[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float* outputPtrs[4] = { &output[0], &output[1], &output[2], &output[3] };
    
    // Expected: mono input duplicated to all 4 channels with 2-sample delay
    float expectedOutputs[4] = {0.0f, 0.0f, 0.0f, 10.0f};
    
    for (int i = 0; i < 4; ++i) {
        const float* monoInput[1] = { &leftChannel[i] };
        quadDelayLine.processSample(monoInput, outputPtrs, 1);
        
        // All 4 output channels should receive the same duplicated mono signal
        EXPECT_FLOAT_EQ(output[0], expectedOutputs[i]) << "Sample " << i << " channel 1";
        EXPECT_FLOAT_EQ(output[1], expectedOutputs[i]) << "Sample " << i << " channel 2 (dup)";
        EXPECT_FLOAT_EQ(output[2], expectedOutputs[i]) << "Sample " << i << " channel 3 (dup)";
        EXPECT_FLOAT_EQ(output[3], expectedOutputs[i]) << "Sample " << i << " channel 4 (dup)";
    }
}

// Multi-channel: 5.1 surround (6 channels) block processing
TEST_F(DelayLineTest, SurroundSixChannelBlockProcessing) {
    // 6-channel input (L, R, C, LFE, LS, RS)
    float chL[8] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f};
    float chR[8] = {10.0f, 20.0f, 30.0f, 40.0f, 50.0f, 60.0f, 70.0f, 80.0f};
    float chC[8] = {100.0f, 200.0f, 300.0f, 400.0f, 500.0f, 600.0f, 700.0f, 800.0f};
    float chLFE[8] = {0.5f, 1.0f, 1.5f, 2.0f, 2.5f, 3.0f, 3.5f, 4.0f};
    float chLS[8] = {11.0f, 22.0f, 33.0f, 44.0f, 55.0f, 66.0f, 77.0f, 88.0f};
    float chRS[8] = {111.0f, 222.0f, 333.0f, 444.0f, 555.0f, 666.0f, 777.0f, 888.0f};
    const float* sixChInput[6] = { chL, chR, chC, chLFE, chLS, chRS };
    
    // Output buffers
    float outL[8] = {0.0f};
    float outR[8] = {0.0f};
    float outC[8] = {0.0f};
    float outLFE[8] = {0.0f};
    float outLS[8] = {0.0f};
    float outRS[8] = {0.0f};
    float* sixChOutput[6] = { outL, outR, outC, outLFE, outLS, outRS };
    
    // Process as block
    surroundDelayLine.processBlock(sixChInput, sixChOutput, 6, 8);
    
    // Verify 3-sample delay on all channels
    for (int i = 0; i < 8; ++i) {
        if (i < 3) {
            // First 3 samples should be zero (delay buffer)
            EXPECT_FLOAT_EQ(outL[i], 0.0f) << "Sample " << i << " L should be zero";
            EXPECT_FLOAT_EQ(outR[i], 0.0f) << "Sample " << i << " R should be zero";
            EXPECT_FLOAT_EQ(outC[i], 0.0f) << "Sample " << i << " C should be zero";
        } else {
            // After delay, should match input delayed by 3 samples
            EXPECT_FLOAT_EQ(outL[i], chL[i - 3]) << "Sample " << i << " L";
            EXPECT_FLOAT_EQ(outR[i], chR[i - 3]) << "Sample " << i << " R";
            EXPECT_FLOAT_EQ(outC[i], chC[i - 3]) << "Sample " << i << " C";
            EXPECT_FLOAT_EQ(outLFE[i], chLFE[i - 3]) << "Sample " << i << " LFE";
            EXPECT_FLOAT_EQ(outLS[i], chLS[i - 3]) << "Sample " << i << " LS";
            EXPECT_FLOAT_EQ(outRS[i], chRS[i - 3]) << "Sample " << i << " RS";
        }
    }
}

// Edge case: Modulated delay with extreme negative modulation (clamped to 0)
TEST_F(DelayLineTest, ModulatedDelayClampedToZero) {
    float modExtreme[1] = {-10.0f};
    const float* modulation[2] = { modExtreme, modExtreme };
    
    for (int i = 0; i < 4; ++i) {
        const float* inputPtrs[2] = { &leftChannel[i], &rightChannel[i] };
        clampDelayLine.processSample(inputPtrs, outputPtrs, modulation, 2);
        EXPECT_FLOAT_EQ(output[0], leftChannel[i]) << "Sample " << i << " left (clamped)";
        EXPECT_FLOAT_EQ(output[1], rightChannel[i]) << "Sample " << i << " right (clamped)";
    }
}

// Edge case: Modulated delay with extreme positive modulation (clamped to buffer size)
TEST_F(DelayLineTest, ModulatedDelayClampedToBufferSize) {
    DelayLine<float, NearestInterpolator<float>> clampLargeDelayLine{1, 8};
    clampLargeDelayLine.setDelay(2);
    
    float localOutput[1] = {0.0f};
    float* outputPtr[1] = { localOutput };
    float modExtreme[1] = {1000.0f};
    const float* modulation[1] = { modExtreme };
    float testSamples[12] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f};
    
    for (int i = 0; i < 12; ++i) {
        const float* inputPtr[1] = { &testSamples[i] };
        clampLargeDelayLine.processSample(inputPtr, outputPtr, modulation, 1);
        if (i >= 7) {
            EXPECT_FLOAT_EQ(localOutput[0], testSamples[i - 7]) 
                << "Sample " << i << " delayed by 7 (clamped)";
        }
    }
}

} // namespace Jonssonic