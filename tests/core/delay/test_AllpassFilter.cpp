// Jonssonic - A C++ audio DSP library
// AllpassFilter unit tests
// SPDX-License-Identifier: MIT

#include <gtest/gtest.h>
#include "Jonssonic/core/delays/AllpassFilter.h"
#include "Jonssonic/utils/MathUtils.h"
#include <cmath>
#include <vector>
#include <algorithm>

using namespace Jonssonic;

class AllpassFilterTest : public ::testing::Test {
protected:
    void SetUp() override {
        sampleRate = 48000.0f;
        numChannels = 2;
        maxDelayMs = 50.0f;
    }

    float sampleRate;
    size_t numChannels;
    float maxDelayMs;
};

// Test basic initialization
TEST_F(AllpassFilterTest, Initialization) {
    AllpassFilter<float> allpass;
    allpass.prepare(numChannels, sampleRate, maxDelayMs);
    
    // Process some samples to ensure no crash
    float output = allpass.processSample(0, 1.0f);
    EXPECT_TRUE(std::isfinite(output));
}

// Test processSample without modulation
TEST_F(AllpassFilterTest, ProcessSample_NoModulation) {
    AllpassFilter<float> allpass;
    allpass.prepare(numChannels, sampleRate, maxDelayMs);
    
    // Set delay time
    float delayMs = 10.0f;
    allpass.setDelayMs(delayMs, true);
    
    // Set gain
    allpass.setGain(0.5f, true);
    
    // Send impulse
    float output1 = allpass.processSample(0, 1.0f);
    EXPECT_TRUE(std::isfinite(output1));
    EXPECT_NE(output1, 0.0f);
    
    // Process silence to let delayed signal appear
    size_t delaySamples = static_cast<size_t>(delayMs * sampleRate / 1000.0f);
    float lastOutput = 0.0f;
    for (size_t i = 0; i < delaySamples + 10; ++i) {
        lastOutput = allpass.processSample(0, 0.0f);
    }
    
    // Should have some response
    EXPECT_TRUE(std::isfinite(lastOutput));
}

// Test processSample with modulation
TEST_F(AllpassFilterTest, ProcessSample_WithModulation) {
    AllpassFilter<float> allpass;
    allpass.prepare(numChannels, sampleRate, maxDelayMs);
    
    // Set base delay and gain
    allpass.setDelayMs(10.0f, true);
    allpass.setGain(0.4f, true);
    
    // Create modulation struct
    AllpassMod::Sample<float> mod;
    mod.delayMod = 5.0f; // Add 5 samples to delay
    mod.gainMod = 1.2f; // Scale gain by 1.2
    
    // Process with modulation
    float output = allpass.processSample(0, 1.0f, mod);
    EXPECT_TRUE(std::isfinite(output));
    
    // Continue processing
    for (size_t i = 0; i < 100; ++i) {
        output = allpass.processSample(0, 0.0f, mod);
        EXPECT_TRUE(std::isfinite(output));
    }
}

// Test processBlock without modulation
TEST_F(AllpassFilterTest, ProcessBlock_NoModulation) {
    AllpassFilter<float> allpass;
    allpass.prepare(numChannels, sampleRate, maxDelayMs);
    
    // Set parameters
    allpass.setDelayMs(5.0f, true);
    allpass.setGain(0.3f, true);
    
    // Prepare buffers
    size_t blockSize = 128;
    std::vector<std::vector<float>> inputBuffer(numChannels, std::vector<float>(blockSize, 0.0f));
    std::vector<std::vector<float>> outputBuffer(numChannels, std::vector<float>(blockSize, 0.0f));
    
    // Create impulse in first sample
    inputBuffer[0][0] = 1.0f;
    inputBuffer[1][0] = 1.0f;
    
    // Create raw pointers for processing
    std::vector<const float*> inputPtrs(numChannels);
    std::vector<float*> outputPtrs(numChannels);
    for (size_t ch = 0; ch < numChannels; ++ch) {
        inputPtrs[ch] = inputBuffer[ch].data();
        outputPtrs[ch] = outputBuffer[ch].data();
    }
    
    // Process block
    allpass.processBlock(inputPtrs.data(), outputPtrs.data(), blockSize);
    
    // Verify output
    bool hasNonZero = false;
    for (size_t ch = 0; ch < numChannels; ++ch) {
        for (size_t i = 0; i < blockSize; ++i) {
            EXPECT_TRUE(std::isfinite(outputBuffer[ch][i]));
            if (std::abs(outputBuffer[ch][i]) > 1e-6f) {
                hasNonZero = true;
            }
        }
    }
    EXPECT_TRUE(hasNonZero);
}

// Test processBlock with modulation
TEST_F(AllpassFilterTest, ProcessBlock_WithModulation) {
    AllpassFilter<float> allpass;
    allpass.prepare(numChannels, sampleRate, maxDelayMs);
    
    // Set base parameters
    allpass.setDelayMs(8.0f, true);
    allpass.setGain(0.35f, true);
    
    // Prepare buffers
    size_t blockSize = 64;
    std::vector<std::vector<float>> inputBuffer(numChannels, std::vector<float>(blockSize, 0.0f));
    std::vector<std::vector<float>> outputBuffer(numChannels, std::vector<float>(blockSize, 0.0f));
    
    // Create modulation buffers
    std::vector<std::vector<float>> delayModBuffer(numChannels, std::vector<float>(blockSize, 2.0f));
    std::vector<std::vector<float>> gainModBuffer(numChannels, std::vector<float>(blockSize, 1.1f));
    
    // Add impulse
    inputBuffer[0][0] = 1.0f;
    inputBuffer[1][0] = 1.0f;
    
    // Create raw pointers
    std::vector<const float*> inputPtrs(numChannels);
    std::vector<float*> outputPtrs(numChannels);
    std::vector<const float*> delayModPtrs(numChannels);
    std::vector<const float*> gainModPtrs(numChannels);
    
    for (size_t ch = 0; ch < numChannels; ++ch) {
        inputPtrs[ch] = inputBuffer[ch].data();
        outputPtrs[ch] = outputBuffer[ch].data();
        delayModPtrs[ch] = delayModBuffer[ch].data();
        gainModPtrs[ch] = gainModBuffer[ch].data();
    }
    
    // Create modulation struct
    AllpassMod::Block<float> modStruct;
    modStruct.delayMod = delayModPtrs.data();
    modStruct.gainMod = gainModPtrs.data();
    
    // Process block with modulation
    allpass.processBlock(inputPtrs.data(), outputPtrs.data(), modStruct, blockSize);
    
    // Verify output
    bool hasNonZero = false;
    for (size_t ch = 0; ch < numChannels; ++ch) {
        for (size_t i = 0; i < blockSize; ++i) {
            EXPECT_TRUE(std::isfinite(outputBuffer[ch][i]));
            if (std::abs(outputBuffer[ch][i]) > 1e-6f) {
                hasNonZero = true;
            }
        }
    }
    EXPECT_TRUE(hasNonZero);
}

// Test clear functionality
TEST_F(AllpassFilterTest, ClearFunctionality) {
    AllpassFilter<float> allpass;
    allpass.prepare(numChannels, sampleRate, maxDelayMs);
    
    allpass.setDelayMs(10.0f, true);
    allpass.setGain(0.5f, true);
    
    // Process some samples
    for (size_t i = 0; i < 100; ++i) {
        allpass.processSample(0, 1.0f);
        allpass.processSample(1, 1.0f);
    }
    
    // Clear
    allpass.clear();
    
    // Output should be based on current input after clear (no history)
    float output = allpass.processSample(0, 1.0f);
    EXPECT_TRUE(std::isfinite(output));
}

// Test multi-channel processing
TEST_F(AllpassFilterTest, MultiChannelProcessing) {
    AllpassFilter<float> allpass;
    size_t channels = 4;
    allpass.prepare(channels, sampleRate, maxDelayMs);
    
    allpass.setDelayMs(5.0f, true);
    allpass.setGain(0.4f, true);
    
    // Process different channels with different inputs
    std::vector<float> outputs(channels);
    for (size_t ch = 0; ch < channels; ++ch) {
        outputs[ch] = allpass.processSample(ch, 1.0f + ch * 0.1f);
    }
    
    // All outputs should be finite
    for (size_t ch = 0; ch < channels; ++ch) {
        EXPECT_TRUE(std::isfinite(outputs[ch]));
    }
}

// Test allpass property: flat magnitude response
TEST_F(AllpassFilterTest, FlatMagnitudeResponse) {
    AllpassFilter<float> allpass;
    allpass.prepare(1, sampleRate, maxDelayMs);
    
    // Set delay and gain
    float delayMs = 1.0f;
    allpass.setDelayMs(delayMs, true);
    allpass.setGain(0.7f, true);
    
    // Generate impulse response
    size_t irLength = 4096;
    std::vector<float> impulseResponse(irLength);
    impulseResponse[0] = allpass.processSample(0, 1.0f);
    
    for (size_t i = 1; i < irLength; ++i) {
        impulseResponse[i] = allpass.processSample(0, 0.0f);
    }
    
    // Get magnitude spectrum
    auto magSpec = magnitudeSpectrum(impulseResponse, true, false);
    
    // For an ideal allpass filter, magnitude should be exactly 1.0 (unity gain) at all frequencies
    // Check frequencies from low to Nyquist (skip DC which might be affected by offset)
    for (size_t i = 10; i < magSpec.size() - 10; ++i) {
        EXPECT_NEAR(magSpec[i], 1.0f, 0.01f) << "Magnitude at bin " << i << " should be unity";
    }
}

// Test allpass behavior with impulse
TEST_F(AllpassFilterTest, ImpulseResponse) {
    AllpassFilter<float> allpass;
    allpass.prepare(1, sampleRate, maxDelayMs);
    
    // Set parameters
    float delaySamples = 48.0f;
    allpass.setDelaySamples(delaySamples, true);
    allpass.setGain(0.7f, true);
    
    // Send impulse and collect outputs
    std::vector<float> outputs;
    outputs.push_back(allpass.processSample(0, 1.0f));
    
    for (size_t i = 0; i < 200; ++i) {
        outputs.push_back(allpass.processSample(0, 0.0f));
    }
    
    // First output should be gain * input 
    EXPECT_NEAR(outputs[0], 0.7f, 0.05f);
    
    // At delay time, should see the delayed component
    float echoSample = outputs[static_cast<size_t>(delaySamples)];
    EXPECT_NEAR(echoSample, 1.0f - 0.7f * 0.7f, 0.05f);
    
    // Allpass should produce finite output
    for (const auto& sample : outputs) {
        EXPECT_TRUE(std::isfinite(sample));
    }
}
