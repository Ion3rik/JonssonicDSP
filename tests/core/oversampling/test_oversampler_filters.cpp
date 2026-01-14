// Jonssonic - A C++ audio DSP library
// Unit tests for the OverSamplerFilters classes
// SPDX-License-Identifier: MIT

#include <gtest/gtest.h>
#include <jonssonic/core/oversampling/detail/oversampler_filters.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

#include <jonssonic/utils/math_utils.h>

using namespace jnsc::detail;
using namespace jnsc::utils;

//==============================================================================
// FIR HALFBAND FILTER TESTS
//==============================================================================

TEST(FIRHalfbandStageTest, UpsampleDoublesSampleRate) {
    FIRHalfbandStage<float, 31> filter;
    filter.prepare(1);

    constexpr size_t inputLen = 8;
    constexpr size_t outputLen = inputLen * 2;

    std::vector<float> input(inputLen);
    std::vector<float> output(outputLen);

    for (size_t i = 0; i < inputLen; ++i) {
        input[i] = (i == 0) ? 1.0f : 0.0f;
    }

    const float* inputPtr = input.data();
    float* outputPtr = output.data();

    filter.upsample(&inputPtr, &outputPtr, inputLen);

    EXPECT_EQ(output.size(), outputLen);
    EXPECT_NE(output[0], 0.0f);
}

TEST(FIRHalfbandStageTest, UpsampleImpulseResponse) {
    FIRHalfbandStage<float, 31> filter;
    filter.prepare(1);

    constexpr size_t inputLen = 32;
    constexpr size_t outputLen = inputLen * 2;

    std::vector<float> input(inputLen, 0.0f);
    std::vector<float> output(outputLen, 0.0f);

    input[15] = 1.0f;

    const float* inputPtr = input.data();
    float* outputPtr = output.data();

    filter.upsample(&inputPtr, &outputPtr, inputLen);

    float energy = 0.0f;
    for (size_t i = 0; i < outputLen; ++i) {
        energy += output[i] * output[i];
    }
    EXPECT_GT(energy, 0.1f);
}

TEST(FIRHalfbandStageTest, DownsampleHalvesSampleRate) {
    FIRHalfbandStage<float, 31> filter;
    filter.prepare(1);

    constexpr size_t outputLen = 8;
    constexpr size_t inputLen = outputLen * 2;

    std::vector<float> input(inputLen);
    std::vector<float> output(outputLen);

    for (size_t i = 0; i < inputLen; ++i) {
        input[i] = std::sin(2.0f * 3.14159f * 0.1f * i);
    }

    const float* inputPtr = input.data();
    float* outputPtr = output.data();

    filter.downsample(&inputPtr, &outputPtr, outputLen);

    EXPECT_EQ(output.size(), outputLen);

    for (size_t i = 0; i < outputLen; ++i) {
        EXPECT_LE(std::abs(output[i]), 2.0f);
    }
}

TEST(FIRHalfbandStageTest, AmplitudePreservedInRoundTrip) {
    FIRHalfbandStage<float, 31> upsampler;
    FIRHalfbandStage<float, 31> downsampler;
    upsampler.prepare(1);
    downsampler.prepare(1);

    constexpr size_t originalLen = 256;
    constexpr size_t upsampledLen = originalLen * 2;
    constexpr float testFreq = 0.03f; // Low frequency well within passband

    std::vector<float> original(originalLen);
    std::vector<float> upsampled(upsampledLen);
    std::vector<float> reconstructed(originalLen);

    // Create test sine wave
    for (size_t i = 0; i < originalLen; ++i) {
        original[i] = std::sin(2.0f * pi<float> * testFreq * i);
    }

    const float* originalPtr = original.data();
    float* upsampledPtr = upsampled.data();
    const float* upsampledConstPtr = upsampled.data();
    float* reconstructedPtr = reconstructed.data();

    // Round trip
    upsampler.upsample(&originalPtr, &upsampledPtr, originalLen);
    downsampler.downsample(&upsampledConstPtr, &reconstructedPtr, originalLen);

    // Measure amplitude after transients settle
    float originalAmp = 0.0f;
    float reconstructedAmp = 0.0f;
    constexpr size_t skipSamples = 50; // Skip transients
    constexpr size_t measureSamples = 100;

    for (size_t i = skipSamples; i < skipSamples + measureSamples; ++i) {
        originalAmp = std::max(originalAmp, std::abs(original[i]));
        reconstructedAmp = std::max(reconstructedAmp, std::abs(reconstructed[i]));
    }

    // Amplitude should be preserved within 1 dB (approximately 0.891 to 1.122 ratio)
    float ampRatio = reconstructedAmp / originalAmp;
    EXPECT_GT(ampRatio, 0.891f); // -1 dB
    EXPECT_LT(ampRatio, 1.122f); // +1 dB
}

TEST(FIRHalfbandStageTest, PassbandRippleLessThan1dB) {
    FIRHalfbandStage<float, 31> upsampler;
    FIRHalfbandStage<float, 31> downsampler;
    upsampler.prepare(1);
    downsampler.prepare(1);

    constexpr float sampleRate = 48000.0f;
    constexpr size_t originalLen = 4096; // Power of 2 for DFT
    constexpr size_t upsampledLen = originalLen * 2;

    std::vector<float> original(originalLen);
    std::vector<float> upsampled(upsampledLen);
    std::vector<float> reconstructed(originalLen);

    // Create impulse for frequency response measurement
    original[originalLen / 2] = 1.0f;

    const float* originalPtr = original.data();
    float* upsampledPtr = upsampled.data();
    const float* upsampledConstPtr = upsampled.data();
    float* reconstructedPtr = reconstructed.data();

    // Round trip
    upsampler.upsample(&originalPtr, &upsampledPtr, originalLen);
    downsampler.downsample(&upsampledConstPtr, &reconstructedPtr, originalLen);

    // Compute magnitude spectrum in dB
    auto magSpec = magnitudeSpectrum(reconstructed, true, true);

    // Find peak in passband (normalize)
    float peakDb = -1000.0f;
    for (size_t i = 0; i < magSpec.size() / 2; ++i) { // Check up to Nyquist/2
        peakDb = std::max(peakDb, magSpec[i]);
    }

    // Check passband ripple up to 16 kHz
    float passbandFreqLimit = 16000.0f;
    size_t passbandBin = static_cast<size_t>(passbandFreqLimit / sampleRate * originalLen);

    float minDb = 1000.0f;
    float maxDb = -1000.0f;

    // Skip DC and very low frequencies (first few bins)
    for (size_t i = 5; i < std::min(passbandBin, magSpec.size()); ++i) {
        float db = magSpec[i];
        minDb = std::min(minDb, db);
        maxDb = std::max(maxDb, db);
    }

    float ripple = maxDb - minDb;

    std::cout << "Passband ripple: " << ripple << " dB (min: " << minDb << " dB, max: " << maxDb
              << " dB)" << std::endl;

    EXPECT_LT(ripple, 1.0f); // Ripple should be less than 1 dB
}

TEST(FIRHalfbandStageTest, MultiChannelUpsampling) {
    FIRHalfbandStage<float, 31> filter;
    constexpr size_t numChannels = 2;
    filter.prepare(numChannels);

    constexpr size_t inputLen = 16;
    constexpr size_t outputLen = inputLen * 2;

    std::vector<std::vector<float>> input(numChannels, std::vector<float>(inputLen));
    std::vector<std::vector<float>> output(numChannels, std::vector<float>(outputLen));

    for (size_t ch = 0; ch < numChannels; ++ch) {
        for (size_t i = 0; i < inputLen; ++i) {
            input[ch][i] = std::sin(2.0f * pi<float> * (0.1f + 0.05f * ch) * i);
        }
    }

    std::vector<const float*> inputPtrs(numChannels);
    std::vector<float*> outputPtrs(numChannels);
    for (size_t ch = 0; ch < numChannels; ++ch) {
        inputPtrs[ch] = input[ch].data();
        outputPtrs[ch] = output[ch].data();
    }

    filter.upsample(inputPtrs.data(), outputPtrs.data(), inputLen);

    for (size_t ch = 0; ch < numChannels; ++ch) {
        float energy = 0.0f;
        for (size_t i = 0; i < outputLen; ++i) {
            energy += output[ch][i] * output[ch][i];
        }
        EXPECT_GT(energy, 0.01f);
    }
}
