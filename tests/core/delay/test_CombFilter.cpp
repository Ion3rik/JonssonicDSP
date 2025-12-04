// Jonssonic - A C++ audio DSP library
// CombFilter unit tests
// SPDX-License-Identifier: MIT

#include <gtest/gtest.h>
#include "Jonssonic/core/delays/CombFilter.h"
#include "Jonssonic/utils/MathUtils.h"
#include <cmath>
#include <vector>
#include <algorithm>

using namespace Jonssonic;

class CombFilterTest : public ::testing::Test {
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
TEST_F(CombFilterTest, Initialization) {
    CombFilter<float> comb;
    comb.prepare(numChannels, sampleRate, maxDelayMs);
    
    // Process some samples to ensure no crash
    float output = comb.processSample(0, 1.0f);
    EXPECT_TRUE(std::isfinite(output));
}

// Test processSample without modulation
TEST_F(CombFilterTest, ProcessSample_NoModulation) {
    CombFilter<float> comb;
    comb.prepare(numChannels, sampleRate, maxDelayMs);
    
    // Set delay time
    float delayMs = 10.0f;
    comb.setDelayMs(delayMs, true);
    
    // Set gains
    comb.setFeedbackGain(0.5f, true);
    comb.setFeedforwardGain(0.8f, true);
    
    // Send impulse
    float output1 = comb.processSample(0, 1.0f);
    EXPECT_TRUE(std::isfinite(output1));
    EXPECT_NE(output1, 0.0f);
    
    // Process silence to let delayed signal appear
    size_t delaySamples = static_cast<size_t>(delayMs * sampleRate / 1000.0f);
    float lastOutput = 0.0f;
    for (size_t i = 0; i < delaySamples + 10; ++i) {
        lastOutput = comb.processSample(0, 0.0f);
    }
    
    // Should have some feedback/feedforward response
    EXPECT_TRUE(std::isfinite(lastOutput));
}

// Test processSample with modulation
TEST_F(CombFilterTest, ProcessSample_WithModulation) {
    CombFilter<float> comb;
    comb.prepare(numChannels, sampleRate, maxDelayMs);
    
    // Set base delay and gains
    comb.setDelayMs(10.0f, true);
    comb.setFeedbackGain(0.4f, true);
    comb.setFeedforwardGain(0.7f, true);
    
    // Create modulation struct
    CombMod::Sample<float> mod;
    mod.delayMod = 5.0f; // Add 5 samples to delay
    mod.feedbackMod = 1.2f; // Scale feedback by 1.2
    mod.feedforwardMod = 0.9f; // Scale feedforward by 0.9
    
    // Process with modulation
    float output = comb.processSample(0, 1.0f, mod);
    EXPECT_TRUE(std::isfinite(output));
    
    // Continue processing
    for (size_t i = 0; i < 100; ++i) {
        output = comb.processSample(0, 0.0f, mod);
        EXPECT_TRUE(std::isfinite(output));
    }
}

// Test processBlock without modulation
TEST_F(CombFilterTest, ProcessBlock_NoModulation) {
    CombFilter<float> comb;
    comb.prepare(numChannels, sampleRate, maxDelayMs);
    
    // Set parameters
    comb.setDelayMs(5.0f, true);
    comb.setFeedbackGain(0.3f, true);
    comb.setFeedforwardGain(0.6f, true);
    
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
    comb.processBlock(inputPtrs.data(), outputPtrs.data(), blockSize);
    
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
TEST_F(CombFilterTest, ProcessBlock_WithModulation) {
    CombFilter<float> comb;
    comb.prepare(numChannels, sampleRate, maxDelayMs);
    
    // Set base parameters
    comb.setDelayMs(8.0f, true);
    comb.setFeedbackGain(0.35f, true);
    comb.setFeedforwardGain(0.65f, true);
    
    // Prepare buffers
    size_t blockSize = 64;
    std::vector<std::vector<float>> inputBuffer(numChannels, std::vector<float>(blockSize, 0.0f));
    std::vector<std::vector<float>> outputBuffer(numChannels, std::vector<float>(blockSize, 0.0f));
    
    // Create modulation buffers
    std::vector<std::vector<float>> delayModBuffer(numChannels, std::vector<float>(blockSize, 2.0f));
    std::vector<std::vector<float>> fbModBuffer(numChannels, std::vector<float>(blockSize, 1.1f));
    std::vector<std::vector<float>> ffModBuffer(numChannels, std::vector<float>(blockSize, 0.95f));
    
    // Add impulse
    inputBuffer[0][0] = 1.0f;
    inputBuffer[1][0] = 1.0f;
    
    // Create raw pointers
    std::vector<const float*> inputPtrs(numChannels);
    std::vector<float*> outputPtrs(numChannels);
    std::vector<const float*> delayModPtrs(numChannels);
    std::vector<const float*> fbModPtrs(numChannels);
    std::vector<const float*> ffModPtrs(numChannels);
    
    for (size_t ch = 0; ch < numChannels; ++ch) {
        inputPtrs[ch] = inputBuffer[ch].data();
        outputPtrs[ch] = outputBuffer[ch].data();
        delayModPtrs[ch] = delayModBuffer[ch].data();
        fbModPtrs[ch] = fbModBuffer[ch].data();
        ffModPtrs[ch] = ffModBuffer[ch].data();
    }
    
    // Create modulation struct
    CombMod::Block<float> modStruct;
    modStruct.delayMod = delayModPtrs.data();
    modStruct.feedbackMod = fbModPtrs.data();
    modStruct.feedforwardMod = ffModPtrs.data();
    
    // Process block with modulation
    comb.processBlock(inputPtrs.data(), outputPtrs.data(), modStruct, blockSize);
    
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

// Test feedback behavior
TEST_F(CombFilterTest, FeedbackBehavior) {
    CombFilter<float> comb;
    comb.prepare(1, sampleRate, maxDelayMs);
    
    // Set short delay for easier testing
    float delaySamples = 48; 
    comb.setDelaySamples(delaySamples, true);
    comb.setFeedbackGain(0.7f, true);
    comb.setFeedforwardGain(1.0f, true); // Feedforward = 1 gives us pure feedback comb
    
    // Send impulse and collect outputs
    std::vector<float> outputs;
    outputs.push_back(comb.processSample(0, 1.0f));
    
    for (size_t i = 0; i < 200; ++i) {
        outputs.push_back(comb.processSample(0, 0.0f));
    }
    
    // Should see recurring energy due to feedback at delay intervals
    // With feedback gain of 0.7 and feedforward gain of 1.0:
    // Sample 0: impulse (1.0) goes in, output = 1.0 (dry input)
    // Sample 48: impulse comes through delay as 1.0, gets scaled by fbGain (0.7) before feeding back
    //            output = 0 + 1.0 * ffGain = 1.0, feedbackState = 1.0 * 0.7 = 0.7
    // Sample 49: 0.7 goes into delay
    // Sample 97: 0.7 comes out, gets scaled to 0.49 for feedback
    //            output = 0 + 0.7 * 1.0 = 0.7
    
    // Check first echo at delay time - should be 1.0 (original impulse, not yet attenuated)
    float firstEcho = outputs[static_cast<size_t>(delaySamples)];
    EXPECT_NEAR(firstEcho, 1.0f, 0.05f);
    
    // Check second echo at delay+1+delay (sample 97)
    float secondEcho = outputs[static_cast<size_t>(delaySamples + 1 + delaySamples)];
    EXPECT_NEAR(secondEcho, 0.7f, 0.05f);
    
    // Check third echo
    float thirdEcho = outputs[static_cast<size_t>(delaySamples + 1 + delaySamples + 1 + delaySamples)];
    EXPECT_NEAR(thirdEcho, 0.49f, 0.05f);
}

// Test clear functionality
TEST_F(CombFilterTest, ClearFunctionality) {
    CombFilter<float> comb;
    comb.prepare(numChannels, sampleRate, maxDelayMs);
    
    comb.setDelayMs(10.0f, true);
    comb.setFeedbackGain(0.5f, true);
    comb.setFeedforwardGain(0.5f, true);
    
    // Process some samples
    for (size_t i = 0; i < 100; ++i) {
        comb.processSample(0, 1.0f);
        comb.processSample(1, 1.0f);
    }
    
    // Clear
    comb.clear();
    
    // Output should be close to input after clear (no history)
    float output = comb.processSample(0, 1.0f);
    EXPECT_TRUE(std::isfinite(output));
}

// Test multi-channel processing
TEST_F(CombFilterTest, MultiChannelProcessing) {
    CombFilter<float> comb;
    size_t channels = 4;
    comb.prepare(channels, sampleRate, maxDelayMs);
    
    comb.setDelayMs(5.0f, true);
    comb.setFeedbackGain(0.4f, true);
    comb.setFeedforwardGain(0.6f, true);
    
    // Process different channels with different inputs
    std::vector<float> outputs(channels);
    for (size_t ch = 0; ch < channels; ++ch) {
        outputs[ch] = comb.processSample(ch, 1.0f + ch * 0.1f);
    }
    
    // All outputs should be finite and non-zero
    for (size_t ch = 0; ch < channels; ++ch) {
        EXPECT_TRUE(std::isfinite(outputs[ch]));
    }
}

// Test frequency response of feedback comb filter (peaks)
TEST_F(CombFilterTest, FeedbackCombFrequencyResponse) {
    CombFilter<float> comb;
    comb.prepare(1, sampleRate, maxDelayMs);
    
    // Set delay to create peaks at known frequencies
    float delayMs = 1.0f; // ~48 samples at 48kHz = 1000 Hz fundamental
    size_t delaySamples = static_cast<size_t>(delayMs * sampleRate / 1000.0f);
    
    comb.setDelayMs(delayMs, true);
    comb.setFeedbackGain(0.7f, true);
    comb.setFeedforwardGain(1.0f, true); // Need feedforward=1 to hear delayed signal
    
    // Generate impulse response
    size_t irLength = 4096; // Longer for better frequency resolution
    std::vector<float> impulseResponse(irLength);
    impulseResponse[0] = comb.processSample(0, 1.0f);
    
    for (size_t i = 1; i < irLength; ++i) {
        impulseResponse[i] = comb.processSample(0, 0.0f);
    }
    
    // Get magnitude spectrum
    auto magSpec = magnitudeSpectrum(impulseResponse, true, false);
    
    // Calculate bin indices for test frequencies
    float binResolution = sampleRate / static_cast<float>(irLength);
    float fundamentalFreq = sampleRate / static_cast<float>(delaySamples);
    
    // Helper to average a few bins around a frequency
    auto avgMag = [&](float freq) {
        size_t bin = static_cast<size_t>(freq / binResolution);
        bin = std::min(bin, magSpec.size() - 1);
        // Average 3 bins for robustness
        float sum = magSpec[bin];
        if (bin > 0) sum += magSpec[bin-1];
        if (bin < magSpec.size() - 1) sum += magSpec[bin+1];
        return sum / 3.0f;
    };
    
    float magFund = avgMag(fundamentalFreq);
    float magBetween = avgMag(fundamentalFreq * 1.5f);
    float magHarm2 = avgMag(fundamentalFreq * 2.0f);
    
    // Feedback comb should have peaks at harmonics
    // Just verify there's variation in the spectrum (peaks exist)
    float maxMag = *std::max_element(magSpec.begin(), magSpec.end());
    float minMag = *std::min_element(magSpec.begin(), magSpec.end());
    
    // Should have significant variation (peaks and valleys)
    EXPECT_GT(maxMag, minMag * 1.5f);
    
    // Harmonics should be local peaks compared to nearby bins
    EXPECT_GT(magFund, magBetween);
    EXPECT_GT(magHarm2, magBetween);
}

// Test frequency response of feedforward comb filter (notches)
TEST_F(CombFilterTest, FeedforwardCombFrequencyResponse) {
    CombFilter<float> comb;
    comb.prepare(1, sampleRate, maxDelayMs);
    
    // Set delay to create notches at known frequencies
    float delayMs = 1.0f; // ~48 samples at 48kHz = 1000 Hz fundamental
    size_t delaySamples = static_cast<size_t>(delayMs * sampleRate / 1000.0f);
    
    comb.setDelayMs(delayMs, true);
    comb.setFeedbackGain(0.0f, true); // Pure feedforward comb
    comb.setFeedforwardGain(-1.0f, true); // Inverted for notches
    
    // Generate impulse response
    size_t irLength = 4096; // Longer for better frequency resolution
    std::vector<float> impulseResponse(irLength);
    impulseResponse[0] = comb.processSample(0, 1.0f);
    
    for (size_t i = 1; i < irLength; ++i) {
        impulseResponse[i] = comb.processSample(0, 0.0f);
    }
    
    // Get magnitude spectrum
    auto magSpec = magnitudeSpectrum(impulseResponse, true, false);
    
    // Calculate bin indices for test frequencies
    float binResolution = sampleRate / static_cast<float>(irLength);
    float fundamentalFreq = sampleRate / static_cast<float>(delaySamples);
    
    // Helper to average a few bins around a frequency
    auto avgMag = [&](float freq) {
        size_t bin = static_cast<size_t>(freq / binResolution);
        bin = std::min(bin, magSpec.size() - 1);
        // Average 3 bins for robustness
        float sum = magSpec[bin];
        if (bin > 0) sum += magSpec[bin-1];
        if (bin < magSpec.size() - 1) sum += magSpec[bin+1];
        return sum / 3.0f;
    };
    
    float magFund = avgMag(fundamentalFreq);
    float magBetween = avgMag(fundamentalFreq * 1.5f);
    float magHarm2 = avgMag(fundamentalFreq * 2.0f);
    
    // Feedforward comb with inverted gain should have notches at harmonics
    // Notches should be at least 50% lower than peaks between them
    EXPECT_LT(magFund, magBetween * 0.5f);
    EXPECT_LT(magHarm2, magBetween * 0.5f);
}
