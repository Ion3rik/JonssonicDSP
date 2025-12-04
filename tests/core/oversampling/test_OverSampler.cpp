// Jonssonic - A C++ audio DSP library
// Unit tests for the OverSampler class
// SPDX-License-Identifier: MIT

#include "../../../include/Jonssonic/core/oversampling/OverSampler.h"
#include "../../../include/Jonssonic/utils/MathUtils.h"
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

using namespace Jonssonic;

//==============================================================================
// OVERSAMPLER TESTS - FACTOR 2
//==============================================================================

TEST(OverSamplerTest, Factor2_DCSignalRoundTrip) {
    OverSampler<float, 2> oversampler;
    oversampler.prepare(1, 128);
    
    constexpr size_t baseLen = 128;
    constexpr size_t oversampledLen = baseLen * 2;
    
    std::vector<float> input(baseLen, 1.0f);  // DC signal (all ones)
    std::vector<float> upsampled(oversampledLen, 0.0f);
    std::vector<float> output(baseLen, 0.0f);
    
    const float* inputPtrs[1] = {input.data()};
    float* upsampledPtrs[1] = {upsampled.data()};
    const float* upsampledPtrsConst[1] = {upsampled.data()};
    float* outputPtrs[1] = {output.data()};
    
    oversampler.upsample(inputPtrs, upsampledPtrs, baseLen);
    oversampler.downsample(upsampledPtrsConst, outputPtrs, baseLen);
    
    // Check DC level is preserved (after filter settles)
    constexpr size_t skipSamples = 50;  // Skip transients
    float sumInput = 0.0f;
    float sumOutput = 0.0f;
    for (size_t i = skipSamples; i < baseLen - skipSamples; ++i) {
        sumInput += input[i];
        sumOutput += output[i];
    }
    float avgInput = sumInput / (baseLen - 2*skipSamples);
    float avgOutput = sumOutput / (baseLen - 2*skipSamples);
    
    std::cout << "DC Test Factor 2: Input avg = " << avgInput 
              << ", Output avg = " << avgOutput 
              << ", Ratio = " << (avgOutput/avgInput) << std::endl;
    
    // Should be very close to 1.0 (within 1%)
    EXPECT_NEAR(avgOutput, 1.0f, 0.02f);
}

TEST(OverSamplerTest, Factor2_UpsampleDoublesSampleCount) {
    OverSampler<float, 2> oversampler;
    oversampler.prepare(1, 128);
    
    constexpr size_t inputLen = 64;
    constexpr size_t outputLen = inputLen * 2;
    
    std::vector<float> input(inputLen, 0.0f);
    std::vector<float> output(outputLen, 0.0f);
    
    input[10] = 1.0f;
    
    const float* inputPtrs[1] = {input.data()};
    float* outputPtrs[1] = {output.data()};
    
    oversampler.upsample(inputPtrs, outputPtrs, inputLen);
    
    // Check output length is correct
    EXPECT_EQ(oversampler.getUpsampledLength(inputLen), outputLen);
    
    // Check some energy was produced
    float energy = 0.0f;
    for (size_t i = 0; i < outputLen; ++i) {
        energy += output[i] * output[i];
    }
    EXPECT_GT(energy, 0.1f);
}

TEST(OverSamplerTest, Factor2_DownsampleHalvesSampleCount) {
    OverSampler<float, 2> oversampler;
    oversampler.prepare(1, 128);
    
    constexpr size_t outputLen = 64;
    constexpr size_t inputLen = outputLen * 2;
    
    std::vector<float> input(inputLen, 0.0f);
    std::vector<float> output(outputLen, 0.0f);
    
    // Create a simple sine wave
    for (size_t i = 0; i < inputLen; ++i) {
        input[i] = std::sin(2.0f * 3.14159f * 0.05f * i);
    }
    
    const float* inputPtrs[1] = {input.data()};
    float* outputPtrs[1] = {output.data()};
    
    oversampler.downsample(inputPtrs, outputPtrs, outputLen);
    
    // Check output length calculation
    EXPECT_EQ(oversampler.getDownsampledLength(inputLen), outputLen);
    
    // Check reasonable output values
    for (size_t i = 0; i < outputLen; ++i) {
        EXPECT_LE(std::abs(output[i]), 2.0f);
    }
}

TEST(OverSamplerTest, Factor2_RoundTripPreservesSignal) {
    OverSampler<float, 2> oversampler;
    oversampler.prepare(1, 128);
    
    constexpr size_t baseLen = 128;
    constexpr size_t oversampledLen = baseLen * 2;
    
    std::vector<float> input(baseLen, 1.0f);  // DC signal
    std::vector<float> upsampled(oversampledLen, 0.0f);
    std::vector<float> output(baseLen, 0.0f);
    
    const float* inputPtrs[1] = {input.data()};
    float* upsampledPtrs[1] = {upsampled.data()};
    const float* upsampledPtrsConst[1] = {upsampled.data()};
    float* outputPtrs[1] = {output.data()};
    
    oversampler.upsample(inputPtrs, upsampledPtrs, baseLen);
    oversampler.downsample(upsampledPtrsConst, outputPtrs, baseLen);
    
    // Check DC level preservation (after filter settles)
    constexpr size_t skipSamples = 50;
    float avgOutput = 0.0f;
    for (size_t i = skipSamples; i < baseLen - skipSamples; ++i) {
        avgOutput += output[i];
    }
    avgOutput /= (baseLen - 2*skipSamples);
    
    std::cout << "Factor 2: DC output = " << avgOutput << std::endl;
    EXPECT_NEAR(avgOutput, 1.0f, 0.02f);
}

//==============================================================================
// OVERSAMPLER TESTS - FACTOR 4
//==============================================================================

TEST(OverSamplerTest, Factor4_UpsampleQuadruplesSampleCount) {
    OverSampler<float, 4> oversampler;
    oversampler.prepare(1, 128);
    
    constexpr size_t inputLen = 64;
    constexpr size_t outputLen = inputLen * 4;
    
    std::vector<float> input(inputLen, 0.0f);
    std::vector<float> output(outputLen, 0.0f);
    
    input[10] = 1.0f;
    
    const float* inputPtr = input.data();
    float* outputPtr = output.data();
    
    oversampler.upsample(&inputPtr, &outputPtr, inputLen);
    
    EXPECT_EQ(oversampler.getUpsampledLength(inputLen), outputLen);
    
    float energy = 0.0f;
    for (size_t i = 0; i < outputLen; ++i) {
        energy += output[i] * output[i];
    }
    EXPECT_GT(energy, 0.1f);
}

TEST(OverSamplerTest, Factor4_DownsampleQuartersSampleCount) {
    OverSampler<float, 4> oversampler;
    oversampler.prepare(1, 128);
    
    constexpr size_t outputLen = 64;
    constexpr size_t inputLen = outputLen * 4;
    
    std::vector<float> input(inputLen, 0.0f);
    std::vector<float> output(outputLen, 0.0f);
    
    for (size_t i = 0; i < inputLen; ++i) {
        input[i] = std::sin(2.0f * 3.14159f * 0.05f * i);
    }
    
    const float* inputPtrs[1] = {input.data()};
    float* outputPtrs[1] = {output.data()};
    
    oversampler.downsample(inputPtrs, outputPtrs, outputLen);
    
    EXPECT_EQ(oversampler.getDownsampledLength(inputLen), outputLen);
}

TEST(OverSamplerTest, Factor4_RoundTripPreservesSignal) {
    OverSampler<float, 4> oversampler;
    oversampler.prepare(1, 128);
    
    constexpr size_t baseLen = 128;
    constexpr size_t oversampledLen = baseLen * 4;
    
    std::vector<float> input(baseLen, 1.0f);  // DC signal
    std::vector<float> upsampled(oversampledLen, 0.0f);
    std::vector<float> output(baseLen, 0.0f);
    
    const float* inputPtrs[1] = {input.data()};
    float* upsampledPtrs[1] = {upsampled.data()};
    const float* upsampledPtrsConst[1] = {upsampled.data()};
    float* outputPtrs[1] = {output.data()};
    
    oversampler.upsample(inputPtrs, upsampledPtrs, baseLen);
    oversampler.downsample(upsampledPtrsConst, outputPtrs, baseLen);
    
    // Check DC level preservation (after filter settles)
    constexpr size_t skipSamples = 50;
    float avgOutput = 0.0f;
    for (size_t i = skipSamples; i < baseLen - skipSamples; ++i) {
        avgOutput += output[i];
    }
    avgOutput /= (baseLen - 2*skipSamples);
    
    std::cout << "Factor 4: DC output = " << avgOutput << std::endl;
    EXPECT_NEAR(avgOutput, 1.0f, 0.02f);
}

//==============================================================================
// OVERSAMPLER TESTS - FACTOR 8
//==============================================================================

TEST(OverSamplerTest, Factor8_UpsampleOctuplesSampleCount) {
    OverSampler<float, 8> oversampler;
    oversampler.prepare(1, 128);
    
    constexpr size_t inputLen = 64;
    constexpr size_t outputLen = inputLen * 8;
    
    std::vector<float> input(inputLen, 0.0f);
    std::vector<float> output(outputLen, 0.0f);
    
    input[10] = 1.0f;
    
    const float* inputPtrs[1] = {input.data()};
    float* outputPtrs[1] = {output.data()};
    
    oversampler.upsample(inputPtrs, outputPtrs, inputLen);
    
    EXPECT_EQ(oversampler.getUpsampledLength(inputLen), outputLen);
    
    float energy = 0.0f;
    for (size_t i = 0; i < outputLen; ++i) {
        energy += output[i] * output[i];
    }
    EXPECT_GT(energy, 0.1f);
}

TEST(OverSamplerTest, Factor8_RoundTripPreservesSignal) {
    OverSampler<float, 8> oversampler;
    oversampler.prepare(1, 128);
    
    constexpr size_t baseLen = 128;
    constexpr size_t oversampledLen = baseLen * 8;
    
    std::vector<float> input(baseLen, 1.0f);  // DC signal
    std::vector<float> upsampled(oversampledLen, 0.0f);
    std::vector<float> output(baseLen, 0.0f);
    
    const float* inputPtrs[1] = {input.data()};
    float* upsampledPtrs[1] = {upsampled.data()};
    const float* upsampledPtrsConst[1] = {upsampled.data()};
    float* outputPtrs[1] = {output.data()};
    
    oversampler.upsample(inputPtrs, upsampledPtrs, baseLen);
    oversampler.downsample(upsampledPtrsConst, outputPtrs, baseLen);
    
    // Check DC level preservation (after filter settles)
    constexpr size_t skipSamples = 50;
    float avgOutput = 0.0f;
    for (size_t i = skipSamples; i < baseLen - skipSamples; ++i) {
        avgOutput += output[i];
    }
    avgOutput /= (baseLen - 2*skipSamples);
    
    std::cout << "Factor 8: DC output = " << avgOutput << std::endl;
    EXPECT_NEAR(avgOutput, 1.0f, 0.02f);
}

//==============================================================================
// OVERSAMPLER TESTS - FACTOR 16
//==============================================================================

TEST(OverSamplerTest, Factor16_UpsampleSixteenXSampleCount) {
    OverSampler<float, 16> oversampler;
    oversampler.prepare(1, 128);
    
    constexpr size_t inputLen = 64;
    constexpr size_t outputLen = inputLen * 16;
    
    std::vector<float> input(inputLen, 0.0f);
    std::vector<float> output(outputLen, 0.0f);
    
    input[10] = 1.0f;
    
    const float* inputPtrs[1] = {input.data()};
    float* outputPtrs[1] = {output.data()};
    
    oversampler.upsample(inputPtrs, outputPtrs, inputLen);
    
    EXPECT_EQ(oversampler.getUpsampledLength(inputLen), outputLen);
    
    float energy = 0.0f;
    for (size_t i = 0; i < outputLen; ++i) {
        energy += output[i] * output[i];
    }
    EXPECT_GT(energy, 0.1f);
}

TEST(OverSamplerTest, Factor16_RoundTripPreservesSignal) {
    OverSampler<float, 16> oversampler;
    oversampler.prepare(1, 128);
    
    constexpr size_t baseLen = 128;
    constexpr size_t oversampledLen = baseLen * 16;
    
    std::vector<float> input(baseLen, 1.0f);  // DC signal
    std::vector<float> upsampled(oversampledLen, 0.0f);
    std::vector<float> output(baseLen, 0.0f);
    
    const float* inputPtrs[1] = {input.data()};
    float* upsampledPtrs[1] = {upsampled.data()};
    const float* upsampledPtrsConst[1] = {upsampled.data()};
    float* outputPtrs[1] = {output.data()};
    
    oversampler.upsample(inputPtrs, upsampledPtrs, baseLen);
    oversampler.downsample(upsampledPtrsConst, outputPtrs, baseLen);
    
    // Check DC level preservation (after filter settles)
    constexpr size_t skipSamples = 50;
    float avgOutput = 0.0f;
    for (size_t i = skipSamples; i < baseLen - skipSamples; ++i) {
        avgOutput += output[i];
    }
    avgOutput /= (baseLen - 2*skipSamples);
    
    std::cout << "Factor 16: DC output = " << avgOutput << std::endl;
    EXPECT_NEAR(avgOutput, 1.0f, 0.02f);
}

//==============================================================================
// MULTI-CHANNEL TESTS
//==============================================================================

TEST(OverSamplerTest, MultiChannel_Factor4_StereoProcessing) {
    OverSampler<float, 4> oversampler;
    oversampler.prepare(2, 128);
    
    constexpr size_t baseLen = 128;
    constexpr size_t oversampledLen = baseLen * 4;
    
    std::vector<float> inputL(baseLen, 0.5f);   // DC at 0.5
    std::vector<float> inputR(baseLen, 0.75f);  // DC at 0.75
    std::vector<float> upsampledL(oversampledLen, 0.0f);
    std::vector<float> upsampledR(oversampledLen, 0.0f);
    std::vector<float> outputL(baseLen, 0.0f);
    std::vector<float> outputR(baseLen, 0.0f);
    
    const float* inputPtrs[2] = {inputL.data(), inputR.data()};
    float* upsampledPtrs[2] = {upsampledL.data(), upsampledR.data()};
    const float* upsampledPtrsConst[2] = {upsampledL.data(), upsampledR.data()};
    float* outputPtrs[2] = {outputL.data(), outputR.data()};
    
    oversampler.upsample(inputPtrs, upsampledPtrs, baseLen);
    oversampler.downsample(upsampledPtrsConst, outputPtrs, baseLen);
    
    // Check both channels preserve their DC levels independently
    constexpr size_t skipSamples = 50;
    float avgL = 0.0f, avgR = 0.0f;
    for (size_t i = skipSamples; i < baseLen - skipSamples; ++i) {
        avgL += outputL[i];
        avgR += outputR[i];
    }
    avgL /= (baseLen - 2*skipSamples);
    avgR /= (baseLen - 2*skipSamples);
    
    std::cout << "Stereo: L DC = " << avgL << " (expected 0.5), R DC = " << avgR << " (expected 0.75)" << std::endl;
    
    EXPECT_NEAR(avgL, 0.5f, 0.02f);
    EXPECT_NEAR(avgR, 0.75f, 0.02f);
}

//==============================================================================
// LATENCY TESTS
//==============================================================================

TEST(OverSamplerTest, Factor2_LatencyAccurate) {
    OverSampler<float, 2> oversampler;
    oversampler.prepare(1, 256);
    
    constexpr size_t baseLen = 256;
    constexpr size_t oversampledLen = baseLen * 2;
    
    // Create impulse at start
    std::vector<float> input(baseLen, 0.0f);
    input[0] = 1.0f;
    std::vector<float> upsampled(oversampledLen, 0.0f);
    std::vector<float> output(baseLen, 0.0f);
    
    const float* inputPtrs[1] = {input.data()};
    float* upsampledPtrs[1] = {upsampled.data()};
    const float* upsampledPtrsConst[1] = {upsampled.data()};
    float* outputPtrs[1] = {output.data()};
    
    oversampler.upsample(inputPtrs, upsampledPtrs, baseLen);
    oversampler.downsample(upsampledPtrsConst, outputPtrs, baseLen);
    
    // Measure latency using cross-correlation
    int measuredLatency = measureLatency(input, output);
    size_t expectedLatency = oversampler.getLatencySamples();
    
    std::cout << "Factor 2: Measured latency = " << measuredLatency 
              << " samples, Expected = " << expectedLatency << " samples" << std::endl;
    
    EXPECT_EQ(measuredLatency, static_cast<int>(expectedLatency));
}

TEST(OverSamplerTest, Factor4_LatencyAccurate) {
    OverSampler<float, 4> oversampler;
    oversampler.prepare(1, 256);
    
    constexpr size_t baseLen = 256;
    constexpr size_t oversampledLen = baseLen * 4;
    
    // Create impulse at start
    std::vector<float> input(baseLen, 0.0f);
    input[0] = 1.0f;
    std::vector<float> upsampled(oversampledLen, 0.0f);
    std::vector<float> output(baseLen, 0.0f);
    
    const float* inputPtrs[1] = {input.data()};
    float* upsampledPtrs[1] = {upsampled.data()};
    const float* upsampledPtrsConst[1] = {upsampled.data()};
    float* outputPtrs[1] = {output.data()};
    
    oversampler.upsample(inputPtrs, upsampledPtrs, baseLen);
    oversampler.downsample(upsampledPtrsConst, outputPtrs, baseLen);
    
    // Measure latency using cross-correlation
    int measuredLatency = measureLatency(input, output);
    size_t expectedLatency = oversampler.getLatencySamples();
    
    std::cout << "Factor 4: Measured latency = " << measuredLatency 
              << " samples, Expected = " << expectedLatency << " samples" << std::endl;
    
    EXPECT_EQ(measuredLatency, static_cast<int>(expectedLatency));
}

TEST(OverSamplerTest, Factor8_LatencyAccurate) {
    OverSampler<float, 8> oversampler;
    oversampler.prepare(1, 256);
    
    constexpr size_t baseLen = 256;
    constexpr size_t oversampledLen = baseLen * 8;
    
    // Create impulse at start
    std::vector<float> input(baseLen, 0.0f);
    input[0] = 1.0f;
    std::vector<float> upsampled(oversampledLen, 0.0f);
    std::vector<float> output(baseLen, 0.0f);
    
    const float* inputPtrs[1] = {input.data()};
    float* upsampledPtrs[1] = {upsampled.data()};
    const float* upsampledPtrsConst[1] = {upsampled.data()};
    float* outputPtrs[1] = {output.data()};
    
    oversampler.upsample(inputPtrs, upsampledPtrs, baseLen);
    oversampler.downsample(upsampledPtrsConst, outputPtrs, baseLen);
    
    // Measure latency using cross-correlation
    int measuredLatency = measureLatency(input, output);
    size_t expectedLatency = oversampler.getLatencySamples();
    
    std::cout << "Factor 8: Measured latency = " << measuredLatency 
              << " samples, Expected = " << expectedLatency << " samples" << std::endl;
    
    EXPECT_EQ(measuredLatency, static_cast<int>(expectedLatency));
}

TEST(OverSamplerTest, Factor16_LatencyAccurate) {
    OverSampler<float, 16> oversampler;
    oversampler.prepare(1, 256);
    
    constexpr size_t baseLen = 256;
    constexpr size_t oversampledLen = baseLen * 16;
    
    // Create impulse at start
    std::vector<float> input(baseLen, 0.0f);
    input[0] = 1.0f;
    std::vector<float> upsampled(oversampledLen, 0.0f);
    std::vector<float> output(baseLen, 0.0f);
    
    const float* inputPtrs[1] = {input.data()};
    float* upsampledPtrs[1] = {upsampled.data()};
    const float* upsampledPtrsConst[1] = {upsampled.data()};
    float* outputPtrs[1] = {output.data()};
    
    oversampler.upsample(inputPtrs, upsampledPtrs, baseLen);
    oversampler.downsample(upsampledPtrsConst, outputPtrs, baseLen);
    
    // Measure latency using cross-correlation
    int measuredLatency = measureLatency(input, output);
    size_t expectedLatency = oversampler.getLatencySamples();
    
    std::cout << "Factor 16: Measured latency = " << measuredLatency 
              << " samples, Expected = " << expectedLatency << " samples" << std::endl;
    
    EXPECT_EQ(measuredLatency, static_cast<int>(expectedLatency));
}
