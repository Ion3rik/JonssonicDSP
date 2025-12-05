// Jonssonic - A C++ audio DSP library
// Unit tests for the SOSFilter class
// SPDX-License-Identifier: MIT

#include <gtest/gtest.h>
#include <cmath>
#include "Jonssonic/core/filters/SOSFilter.h"

namespace Jonssonic {

class SOSFilterTest : public ::testing::Test {
protected:
    float sampleRate = 44100.0f;
    
    void SetUp() override {
        filter.prepare(2, 1); // 2 channels, 1 section
        multiSectionFilter.prepare(2, 2); // 2 channels, 2 sections
    }
    
    SOSFilter<float> filter;
    SOSFilter<float> multiSectionFilter;
};

// Test basic preparation
TEST_F(SOSFilterTest, Prepare) {
    SOSFilter<float> f;
    EXPECT_NO_THROW(f.prepare(2, 1));
    EXPECT_NO_THROW(f.prepare(1, 3));
    EXPECT_NO_THROW(f.prepare(8, 4));
}

// Test setting coefficients
TEST_F(SOSFilterTest, SetCoefficients) {
    // Set simple unity gain coefficients (b0=1, others=0)
    EXPECT_NO_THROW(filter.setSectionCoeffs(0, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f));
}

// Test processing with unity gain (passthrough)
TEST_F(SOSFilterTest, ProcessSampleUnityGain) {
    // Unity gain: b0=1, all others=0
    filter.setSectionCoeffs(0, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    
    float input = 0.5f;
    float output = filter.processSample(0, input);
    
    EXPECT_FLOAT_EQ(output, input);
}

// Test processing with gain
TEST_F(SOSFilterTest, ProcessSampleWithGain) {
    // Simple gain: b0=2.0, all others=0
    filter.setSectionCoeffs(0, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    
    float input = 0.5f;
    float output = filter.processSample(0, input);
    
    EXPECT_FLOAT_EQ(output, 1.0f);
}

// Test state persistence across samples
TEST_F(SOSFilterTest, StatePersistence) {
    // Simple one-sample delay: b0=0, b1=1, others=0
    filter.setSectionCoeffs(0, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
    
    float out1 = filter.processSample(0, 1.0f);
    float out2 = filter.processSample(0, 2.0f);
    float out3 = filter.processSample(0, 3.0f);
    
    EXPECT_FLOAT_EQ(out1, 0.0f); // First output is 0 (no history)
    EXPECT_FLOAT_EQ(out2, 1.0f); // Second output is previous input
    EXPECT_FLOAT_EQ(out3, 2.0f); // Third output is previous input
}

// Test clear resets state
TEST_F(SOSFilterTest, ClearResetsState) {
    filter.setSectionCoeffs(0, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
    
    filter.processSample(0, 1.0f);
    filter.processSample(0, 2.0f);
    
    filter.clear();
    
    float out = filter.processSample(0, 3.0f);
    EXPECT_FLOAT_EQ(out, 0.0f); // State should be reset
}

// Test multi-channel independence
TEST_F(SOSFilterTest, MultiChannelIndependence) {
    // Unity gain
    filter.setSectionCoeffs(0, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    
    float out0 = filter.processSample(0, 1.0f);
    float out1 = filter.processSample(1, 2.0f);
    
    EXPECT_FLOAT_EQ(out0, 1.0f);
    EXPECT_FLOAT_EQ(out1, 2.0f);
    
    // Check state independence with a fresh filter
    filter.clear(); // Reset state
    filter.setSectionCoeffs(0, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
    
    // Process only on channel 0
    filter.processSample(0, 5.0f);
    
    // Channel 0 should output its stored value
    float out0_delayed = filter.processSample(0, 0.0f);
    // Channel 1 should output 0 (no history)
    float out1_delayed = filter.processSample(1, 0.0f);
    
    EXPECT_FLOAT_EQ(out0_delayed, 5.0f);
    EXPECT_FLOAT_EQ(out1_delayed, 0.0f); // Channel 1 shouldn't have channel 0's history
}

// Test cascaded sections
TEST_F(SOSFilterTest, CascadedSections) {
    // Two sections, each with gain of 2.0
    multiSectionFilter.setSectionCoeffs(0, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    multiSectionFilter.setSectionCoeffs(1, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    
    float input = 0.5f;
    float output = multiSectionFilter.processSample(0, input);
    
    EXPECT_FLOAT_EQ(output, 2.0f); // 0.5 * 2.0 * 2.0 = 2.0
}

// Test simple lowpass behavior (DC gain = 1, high freq attenuation)
TEST_F(SOSFilterTest, SimpleLowpassBehavior) {
    // Simple lowpass-like coefficients
    // b0 = 0.5, b1 = 0.5, a1 = 0 (moving average)
    filter.setSectionCoeffs(0, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f);
    
    // DC input (constant 1.0)
    float out1 = filter.processSample(0, 1.0f);
    float out2 = filter.processSample(0, 1.0f);
    float out3 = filter.processSample(0, 1.0f);
    
    // Should approach 1.0 (DC gain)
    EXPECT_NEAR(out3, 1.0f, 0.01f);
}

// Test feedback (IIR behavior)
TEST_F(SOSFilterTest, FeedbackBehavior) {
    // Simple feedback: b0=1, a1=-0.5 (exponential decay)
    filter.setSectionCoeffs(0, 1.0f, 0.0f, 0.0f, -0.5f, 0.0f);
    
    float out1 = filter.processSample(0, 1.0f); // Impulse
    float out2 = filter.processSample(0, 0.0f);
    float out3 = filter.processSample(0, 0.0f);
    
    EXPECT_FLOAT_EQ(out1, 1.0f);
    EXPECT_NEAR(out2, 0.5f, 0.001f);   // Feedback: -a1 * y1
    EXPECT_NEAR(out3, 0.25f, 0.001f);  // Continues to decay
}

// Test constants are correct
TEST_F(SOSFilterTest, Constants) {
    EXPECT_EQ(SOSFilter<float>::COEFFS_PER_SECTION, 5);
    EXPECT_EQ(SOSFilter<float>::STATE_VARS_PER_SECTION, 4);
}

} // namespace Jonssonic
