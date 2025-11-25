// Jonssonic - A C++ audio DSP library
// Unit tests for the DelayLine class
// Author: Jon Fagerström
// Update: 18.11.2025

#include <gtest/gtest.h>
#include <cmath>
#include "Jonssonic/core/common/Interpolators.h"
#include "Jonssonic/core/delays/DelayLine.h"
#include "Jonssonic/utils/MathUtils.h"

namespace Jonssonic {

class DelayLineTest : public ::testing::Test {
protected:
    float sampleRate = 1000.0f; // Changed to 1000 Hz for easier ms-sample conversions
    float maxDelayMs = 100.0f;

    void SetUp() override {
        // Use new prepare API: (channels, sampleRate, maxDelayMs)
        delayLine.prepare(2, sampleRate, maxDelayMs);
        blockDelayLine.prepare(2, sampleRate, maxDelayMs);
        zeroDelayLine.prepare(2, sampleRate, maxDelayMs);
        maxDelayLine.prepare(1, sampleRate, maxDelayMs);
        smallDelayLine.prepare(1, sampleRate, maxDelayMs);
        clampDelayLine.prepare(2, sampleRate, maxDelayMs);
        oversizeDelayLine.prepare(1, sampleRate, maxDelayMs);
        quadDelayLine.prepare(4, sampleRate, maxDelayMs);
        surroundDelayLine.prepare(6, sampleRate, maxDelayMs);

    // Set delays in samples (directly)
    delayLine.setDelaySamples(2.0f);
    blockDelayLine.setDelaySamples(2.0f);
    zeroDelayLine.setDelaySamples(0.0f);
    maxDelayLine.setDelaySamples(7.0f);
    smallDelayLine.setDelaySamples(4.0f);
    clampDelayLine.setDelaySamples(2.0f);
    oversizeDelayLine.setDelaySamples(100.0f);
    quadDelayLine.setDelaySamples(2.0f);
    surroundDelayLine.setDelaySamples(3.0f);
    }

    void TearDown() override {

    }
    
    // Helper: Process samples one at a time and verify output
    void processSamplesAndVerify(const float* left, const float* right, 
                                 const float* expectedLeft, const float* expectedRight,
                                 size_t numSamples) {
        float inputSamples[2];
        float outputSamples[2];
        for (size_t i = 0; i < numSamples; ++i) {
            inputSamples[0] = left[i];
            inputSamples[1] = right[i];
            outputSamples[0] =delayLine.processSample(inputSamples[0], 0u);
            outputSamples[1] =delayLine.processSample(inputSamples[1], 1u);
            EXPECT_FLOAT_EQ(outputSamples[0], expectedLeft[i]) << "Sample " << i << " left channel";
            EXPECT_FLOAT_EQ(outputSamples[1], expectedRight[i]) << "Sample " << i << " right channel";
        }
    }
    
    // Helper: Process with modulation and verify
    void processModulatedAndVerify(const float* left, const float* right,
                                   float modValue, const float* expectedLeft, 
                                   const float* expectedRight, size_t numSamples) {
        float inputSamples[2];
        float outputSamples[2];
        float modulation[2] = {modValue, modValue};
        
        for (size_t i = 0; i < numSamples; ++i) {
            inputSamples[0] = left[i];
            inputSamples[1] = right[i];
            outputSamples[0] = delayLine.processSample(inputSamples[0], modulation[0], 0u);
            outputSamples[1] = delayLine.processSample(inputSamples[1], modulation[1], 1u);
            EXPECT_FLOAT_EQ(outputSamples[0], expectedLeft[i]) << "Sample " << i << " left channel";
            EXPECT_FLOAT_EQ(outputSamples[1], expectedRight[i]) << "Sample " << i << " right channel";
        }
    }
    
    // Test fixture members - DelayLine instances (with smoothing disabled for precise testing)
    DelayLine<float, NearestInterpolator<float>, SmootherType::OnePole, 1, 0> delayLine;          // Standard stereo
    DelayLine<float, NearestInterpolator<float>, SmootherType::OnePole, 1, 0> blockDelayLine;     // For block processing tests
    DelayLine<float, NearestInterpolator<float>, SmootherType::OnePole, 1, 0> zeroDelayLine;      // Zero delay test
    DelayLine<float, NearestInterpolator<float>, SmootherType::OnePole, 1, 0> maxDelayLine;       // Maximum delay test
    DelayLine<float, NearestInterpolator<float>, SmootherType::OnePole, 1, 0> smallDelayLine;     // Wraparound test
    DelayLine<float, NearestInterpolator<float>, SmootherType::OnePole, 1, 0> clampDelayLine;     // Modulation clamping test
    DelayLine<float, NearestInterpolator<float>, SmootherType::OnePole, 1, 0> oversizeDelayLine;  // Delay exceeds buffer test
    DelayLine<float, NearestInterpolator<float>, SmootherType::OnePole, 1, 0> quadDelayLine;      // Quad (4-channel)
    DelayLine<float, NearestInterpolator<float>, SmootherType::OnePole, 1, 0> surroundDelayLine;  // 5.1 surround (6-channel)
    
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

    // Test: Different integer delay times per channel
    TEST_F(DelayLineTest, PerChannelIntegerDelay)
    {
        DelayLine<float, NearestInterpolator<float>, SmootherType::OnePole, 1, 0> dl;
    dl.prepare(2, sampleRate, maxDelayMs);
    // Set different delays for each channel (in samples)
    dl.setDelaySamples(2.0f, 0u); // left
    dl.setDelaySamples(4.0f, 1u); // right
        float leftInput[6] = {1,2,3,4,5,6};
        float rightInput[6] = {10,20,30,40,50,60};
        float leftExpected[6] = {0,0,1,2,3,4};
        float rightExpected[6] = {0,0,0,0,10,20};
        // Process left with delay 2, right with delay 4
        for (size_t i = 0; i < 6; ++i) {
            float outL = dl.processSample(leftInput[i], 0u);
            float outR = dl.processSample(rightInput[i], 1u);
            EXPECT_FLOAT_EQ(outL, leftExpected[i]) << "Left sample " << i;
            EXPECT_FLOAT_EQ(outR, rightExpected[i]) << "Right sample " << i;
        }
    }

    // Test: Different modulated delay times per channel
    TEST_F(DelayLineTest, PerChannelModulatedDelay)
    {
        DelayLine<float, NearestInterpolator<float>, SmootherType::OnePole, 1, 0> dl;
    dl.prepare(2, sampleRate, maxDelayMs);
    dl.setDelaySamples(2.0f, 0u); // left
    dl.setDelaySamples(4.0f, 1u); // right
        float leftInput[6] = {1,2,3,4,5,6};
        float rightInput[6] = {10,20,30,40,50,60};
        float leftExpected[6] = {0,0,1,2,3,4};
        float rightExpected[6] = {0,0,0,0,10,20};
        float leftMod[6] = {0,0,0,0,0,0};
        float rightMod[6] = {0,0,0,0,0,0};

        for (size_t i = 0; i < 6; ++i) {
            float outL = dl.processSample(leftInput[i], leftMod[i], 0u);
            float outR = dl.processSample(rightInput[i], rightMod[i], 1u);
    
            // For simplicity, just check output is close to expected (interpolated)
            EXPECT_NEAR(outL, leftExpected[i], 1e-5) << "Left sample " << i;
            EXPECT_NEAR(outR, rightExpected[i], 1e-5) << "Right sample " << i;
        }
    }

// Tests for simple sample processing
TEST_F(DelayLineTest, ProcessSingleSampleFixedDelaySteroToStereo) {
    processSamplesAndVerify(leftChannel, rightChannel, expectedLeft, expectedRight, 4);
}

// Test modulated delay with no modulation (should match fixed delay)
TEST_F(DelayLineTest, ProcessSingleSampleModulatedDelayNoModulation) {
    processModulatedAndVerify(leftChannel, rightChannel, 0.0f, expectedLeft, expectedRight, 4);
}

// Test modulated delay with positive modulation (increased delay)
TEST_F(DelayLineTest, ProcessSingleSampleModulatedDelayPositiveModulation) {
    float expected3Left[5] = {0.0f, 0.0f, 0.0f, 0.0f, 10.0f};
    float expected3Right[5] = {0.0f, 0.0f, 0.0f, 0.0f, 20.0f};
    float inputSamples[2];
    float outputSamples[2];
    float modulation[2] = {1.0f, 1.0f};

    for (int i = 0; i < 5; ++i) {
        inputSamples[0] = leftChannel[i % 4];
        inputSamples[1] = rightChannel[i % 4];
        outputSamples[0] = delayLine.processSample(inputSamples[0], modulation[0], 0u);
        outputSamples[1] = delayLine.processSample(inputSamples[1], modulation[1], 1u);
        EXPECT_FLOAT_EQ(outputSamples[0], expected3Left[i]) << "Sample " << i << " left";
        EXPECT_FLOAT_EQ(outputSamples[1], expected3Right[i]) << "Sample " << i << " right";
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
    float inputSamples[2];
    float outputSamples[2];
    float modulation[2] = {0.5f, 0.5f};

    for (int i = 0; i < 4; ++i) {
        inputSamples[0] = leftChannel[i];
        inputSamples[1] = rightChannel[i];
        outputSamples[0] = delayLine.processSample(inputSamples[0], modulation[0], 0u);
        outputSamples[1] = delayLine.processSample(inputSamples[1], modulation[1], 1u);
        EXPECT_GE(outputSamples[0], 0.0f) << "Sample " << i << " left channel should be non-negative";
        EXPECT_GE(outputSamples[1], 0.0f) << "Sample " << i << " right channel should be non-negative";
    }
}

// Test modulated delay with varying modulation signal
TEST_F(DelayLineTest, ProcessSingleSampleModulatedDelayVaryingModulation) {
    float modulationValues[4] = {0.0f, 0.0f, 1.0f, -1.0f};
    float inputSamples[2];
    float outputSamples[2];
    float modulation[2];
    
    for (int i = 0; i < 4; ++i) {
        inputSamples[0] = leftChannel[i];
        inputSamples[1] = rightChannel[i];
        modulation[0] = modulationValues[i];
        modulation[1] = modulationValues[i];
        
        outputSamples[0] = delayLine.processSample(inputSamples[0], modulation[0], 0u);
        outputSamples[1] = delayLine.processSample(inputSamples[1], modulation[1], 1u);
        EXPECT_FALSE(std::isnan(outputSamples[0])) << "Sample " << i << " left should not be NaN";
        EXPECT_FALSE(std::isnan(outputSamples[1])) << "Sample " << i << " right should not be NaN";
        EXPECT_TRUE(std::isfinite(outputSamples[0])) << "Sample " << i << " left should be finite";
        EXPECT_TRUE(std::isfinite(outputSamples[1])) << "Sample " << i << " right should be finite";
    }
}

// Test block processing with fixed delay
TEST_F(DelayLineTest, ProcessBlockFixedDelay) {
    // Prepare output buffers
    float outputLeft[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float outputRight[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float* outputPtrs[2] = { outputLeft, outputRight };
    
    // Process entire block at once
    blockDelayLine.processBlock(stereoInput, outputPtrs, 4);
    
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
    DelayLine<float, NearestInterpolator<float>, SmootherType::OnePole, 1, 0> sampleDelayLine;
    DelayLine<float, NearestInterpolator<float>, SmootherType::OnePole, 1, 0> blockCompareDelayLine;
    sampleDelayLine.prepare(2, sampleRate, maxDelayMs);
    blockCompareDelayLine.prepare(2, sampleRate, maxDelayMs);
    // Set delay in samples (directly)
    sampleDelayLine.setDelaySamples(3.0f);
    blockCompareDelayLine.setDelaySamples(3.0f);
    
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
        sampleOutputLeft[i] = sampleDelayLine.processSample(testInputLeft[i], 0u);
        sampleOutputRight[i] = sampleDelayLine.processSample(testInputRight[i], 1u);
    }
    
    // Process as block
    float* blockOutputPtrs[2] = { blockOutputLeft, blockOutputRight };
    blockCompareDelayLine.processBlock(testInput, blockOutputPtrs, 8);
    
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
    blockDelayLine.processBlock(stereoInput, outputPtrs, modulation, 4);
    
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
    float inputSample[1];
    float outputSample[1];
    
    for (int i = 0; i < 20; ++i)
    {
        inputSample[0] = static_cast<float>(i);
        outputSample[0] = smallDelayLine.processSample(inputSample[0], 0u);
        
        if (i >= 4)
        {
            EXPECT_FLOAT_EQ(outputSample[0], static_cast<float>(i - 4)) 
                << "Sample " << i << " should be delayed by 4";
        }
    }
}

// Edge case: Zero delay (immediate pass-through)
TEST_F(DelayLineTest, ZeroDelay) {
    float inputSamples[2];
    float outputSamples[2];
    for (int i = 0; i < 4; ++i) {
        inputSamples[0] = leftChannel[i];
        inputSamples[1] = rightChannel[i];
        outputSamples[0] = zeroDelayLine.processSample(inputSamples[0], 0u);
        outputSamples[1] = zeroDelayLine.processSample(inputSamples[1], 1u);
        EXPECT_FLOAT_EQ(outputSamples[0], leftChannel[i]) << "Sample " << i << " left should pass through";
        EXPECT_FLOAT_EQ(outputSamples[1], rightChannel[i]) << "Sample " << i << " right should pass through";
    }
}

// Edge case: Maximum delay (buffer size - 1)
TEST_F(DelayLineTest, MaximumDelay) {
    float inputSample[1];
    float outputSample[1];
    
    float testSamples[10] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f};
    
    for (int i = 0; i < 10; ++i)
    {
        inputSample[0] = testSamples[i];
        outputSample[0] = maxDelayLine.processSample(inputSample[0], 0u);
        
        
        if (i >= 7)
        {
            EXPECT_FLOAT_EQ(outputSample[0], testSamples[i - 7]) 
                << "Sample " << i << " should be delayed by 7";
        }
        else
        {
            EXPECT_FLOAT_EQ(outputSample[0], 0.0f) 
                << "Sample " << i << " should be zero (buffer not filled yet)";
        }
    }
}

// Edge case: Delay set to maximum allowed (should not be clamped)
TEST_F(DelayLineTest, DelayAtMaximum) {
    float inputSample[1];
    float outputSample[1];
    
    // maxDelayMs=100ms at 1000Hz = 100 samples (this is the max allowed)
    // bufferSize = nextPowerOfTwo(100) = 128 (internal buffer is larger for efficiency)
    // Setting 100 samples should give exactly 100 samples of delay
    
    float testSamples[105];
    for (int i = 0; i < 105; ++i) {
        testSamples[i] = static_cast<float>(i + 1);
    }
    
    for (int i = 0; i < 105; ++i)
    {
        inputSample[0] = testSamples[i];
        outputSample[0] = oversizeDelayLine.processSample(inputSample[0], 0u);
        
        if (i < 100) {
            // First 100 samples are zeros (delay buffer filling)
            EXPECT_FLOAT_EQ(outputSample[0], 0.0f) 
                << "Sample " << i << " should be zero (buffer not filled yet)";
        } else {
            // After 100 samples, we get delayed output
            EXPECT_FLOAT_EQ(outputSample[0], testSamples[i - 100]) 
                << "Sample " << i << " should be delayed by 100 samples (at max delay)";
        }
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
    
    // Expected outputs with 2-sample delay
    float expectedCh1[4] = {0.0f, 0.0f, 1.0f, 2.0f};
    float expectedCh2[4] = {0.0f, 0.0f, 10.0f, 20.0f};
    float expectedCh3[4] = {0.0f, 0.0f, 100.0f, 200.0f};
    float expectedCh4[4] = {0.0f, 0.0f, 1000.0f, 2000.0f};
    
    for (int i = 0; i < 4; ++i) {
        output[0] = quadDelayLine.processSample(channel1[i], 0u);
        output[1] = quadDelayLine.processSample(channel2[i], 1u);
        output[2] = quadDelayLine.processSample(channel3[i], 2u);
        output[3] = quadDelayLine.processSample(channel4[i], 3u);
        
        EXPECT_FLOAT_EQ(output[0], expectedCh1[i]) << "Sample " << i << " channel 1";
        EXPECT_FLOAT_EQ(output[1], expectedCh2[i]) << "Sample " << i << " channel 2";
        EXPECT_FLOAT_EQ(output[2], expectedCh3[i]) << "Sample " << i << " channel 3";
        EXPECT_FLOAT_EQ(output[3], expectedCh4[i]) << "Sample " << i << " channel 4";
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
    surroundDelayLine.processBlock(sixChInput, sixChOutput, 8);
    
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
    float modExtreme[2] = {-10.0f, -10.0f};
    
    for (int i = 0; i < 4; ++i) {
        float outputSamples[2];
        outputSamples[0] = clampDelayLine.processSample(leftChannel[i], modExtreme[0], 0u);
        outputSamples[1] = clampDelayLine.processSample(rightChannel[i], modExtreme[1], 1u);
        EXPECT_FLOAT_EQ(outputSamples[0], leftChannel[i]) << "Sample " << i << " left (clamped)";
        EXPECT_FLOAT_EQ(outputSamples[1], rightChannel[i]) << "Sample " << i << " right (clamped)";
    }
}

// Edge case: Modulated delay with extreme positive modulation (clamped to max delay)
TEST_F(DelayLineTest, ModulatedDelayClampedToMaxDelay) {
    DelayLine<float, NearestInterpolator<float>, SmootherType::OnePole, 1, 0> clampLargeDelayLine;
    clampLargeDelayLine.prepare(1, sampleRate, maxDelayMs);
    clampLargeDelayLine.setDelaySamples(2.0f);

    // Base delay = 2 samples, modulation = 1000 samples
    // Total = 1002 samples, but max allowed is 100 samples
    // Should clamp to 100 samples
    float modExtreme[1] = {1000.0f};
    float testSamples[105];
    for (int i = 0; i < 105; ++i) {
        testSamples[i] = static_cast<float>(i + 1);
    }
    
    for (int i = 0; i < 105; ++i) {
        float outputSample = clampLargeDelayLine.processSample(testSamples[i], modExtreme[0], 0u);
        
        if (i < 100) {
            // First 100 samples are zeros (buffer filling to max delay)
            EXPECT_FLOAT_EQ(outputSample, 0.0f) 
                << "Sample " << i << " zero (buffer not filled)";
        } else {
            // After 100 samples, we get delayed output (clamped to max 100 samples)
            EXPECT_FLOAT_EQ(outputSample, testSamples[i - 100]) 
                << "Sample " << i << " delayed by 100 samples (clamped to max delay)";
        }
    }
}

// Test setDelayMs and setDelaySamples consistency
TEST_F(DelayLineTest, SetDelayMsAndSamplesConsistency) {
    DelayLine<float, NearestInterpolator<float>, SmootherType::OnePole, 1, 0> testDelayLine1;
    DelayLine<float, NearestInterpolator<float>, SmootherType::OnePole, 1, 0> testDelayLine2;
    
    testDelayLine1.prepare(1, sampleRate, maxDelayMs);
    testDelayLine2.prepare(1, sampleRate, maxDelayMs);
    
    // Set same delay using different methods: 10ms = 10 samples at 1000 Hz
    testDelayLine1.setDelayMs(10.0f);
    testDelayLine2.setDelaySamples(10.0f);
    
    // Process identical input through both delay lines
    float testInput[15] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f};
    
    for (int i = 0; i < 15; ++i) {
        float output1 = testDelayLine1.processSample(testInput[i], 0u);
        float output2 = testDelayLine2.processSample(testInput[i], 0u);
        
        EXPECT_FLOAT_EQ(output1, output2) 
            << "Sample " << i << ": setDelayMs(10.0) and setDelaySamples(10.0) should produce identical output";
    }
} 

}// namespace Jonssonic