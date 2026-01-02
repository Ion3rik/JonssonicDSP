// Jonssonic - A C++ audio DSP library
// Unit tests for the FirstOrderCore class
// SPDX-License-Identifier: MIT

#include <gtest/gtest.h>
#include <cmath>
#include <jonssonic/core/filters/detail/first_order_core.h>

using namespace jonssonic::core::filters::detail;

class FirstOrderCoreTest : public ::testing::Test {
protected:
    float sampleRate = 44100.0f;
    void SetUp() override {
        filter.prepare(2, 1); // 2 channels, 1 section
        multiSectionFilter.prepare(2, 2); // 2 channels, 2 sections
    }
    FirstOrderCore<float> filter;
    FirstOrderCore<float> multiSectionFilter;
};

TEST_F(FirstOrderCoreTest, Prepare) {
    FirstOrderCore<float> f;
    EXPECT_NO_THROW(f.prepare(2, 1));
    EXPECT_NO_THROW(f.prepare(1, 3));
    EXPECT_NO_THROW(f.prepare(8, 4));
}

TEST_F(FirstOrderCoreTest, SetCoefficients) {
    EXPECT_NO_THROW(filter.setSectionCoeffs(0, 1.0f, 0.0f, 0.0f));
}

TEST_F(FirstOrderCoreTest, ProcessSampleUnityGain) {
    filter.setSectionCoeffs(0, 1.0f, 0.0f, 0.0f);
    float input = 0.5f;
    float output = filter.processSample(0, input);
    EXPECT_FLOAT_EQ(output, input);
}

TEST_F(FirstOrderCoreTest, ProcessSampleWithGain) {
    filter.setSectionCoeffs(0, 2.0f, 0.0f, 0.0f);
    float input = 0.5f;
    float output = filter.processSample(0, input);
    EXPECT_FLOAT_EQ(output, 1.0f);
}

TEST_F(FirstOrderCoreTest, StatePersistence) {
    filter.setSectionCoeffs(0, 0.0f, 1.0f, 0.0f);
    float out1 = filter.processSample(0, 1.0f);
    float out2 = filter.processSample(0, 2.0f);
    float out3 = filter.processSample(0, 3.0f);
    EXPECT_FLOAT_EQ(out1, 0.0f);
    EXPECT_FLOAT_EQ(out2, 1.0f);
    EXPECT_FLOAT_EQ(out3, 2.0f);
}

TEST_F(FirstOrderCoreTest, ClearResetsState) {
    filter.setSectionCoeffs(0, 0.0f, 1.0f, 0.0f);
    filter.processSample(0, 1.0f);
    filter.processSample(0, 2.0f);
    filter.reset();
    float out = filter.processSample(0, 3.0f);
    EXPECT_FLOAT_EQ(out, 0.0f);
}

TEST_F(FirstOrderCoreTest, MultiChannelIndependence) {
    filter.setSectionCoeffs(0, 1.0f, 0.0f, 0.0f);
    float out0 = filter.processSample(0, 1.0f);
    float out1 = filter.processSample(1, 2.0f);
    EXPECT_FLOAT_EQ(out0, 1.0f);
    EXPECT_FLOAT_EQ(out1, 2.0f);
    filter.reset();
    filter.setSectionCoeffs(0, 0.0f, 1.0f, 0.0f);
    filter.processSample(0, 5.0f);
    float out0_delayed = filter.processSample(0, 0.0f);
    float out1_delayed = filter.processSample(1, 0.0f);
    EXPECT_FLOAT_EQ(out0_delayed, 5.0f);
    EXPECT_FLOAT_EQ(out1_delayed, 0.0f);
}

TEST_F(FirstOrderCoreTest, CascadedSections) {
    multiSectionFilter.setSectionCoeffs(0, 2.0f, 0.0f, 0.0f);
    multiSectionFilter.setSectionCoeffs(1, 2.0f, 0.0f, 0.0f);
    float input = 0.5f;
    float output = multiSectionFilter.processSample(0, input);
    EXPECT_FLOAT_EQ(output, 2.0f);
}

TEST_F(FirstOrderCoreTest, SimpleLowpassBehavior) {
    filter.setSectionCoeffs(0, 0.5f, 0.5f, 0.0f);
    float out1 = filter.processSample(0, 1.0f);
    float out2 = filter.processSample(0, 1.0f);
    float out3 = filter.processSample(0, 1.0f);
    EXPECT_NEAR(out3, 1.0f, 0.01f);
}

TEST_F(FirstOrderCoreTest, FeedbackBehavior) {
    filter.setSectionCoeffs(0, 1.0f, 0.0f, -0.5f);
    float out1 = filter.processSample(0, 1.0f);
    float out2 = filter.processSample(0, 0.0f);
    float out3 = filter.processSample(0, 0.0f);
    EXPECT_FLOAT_EQ(out1, 1.0f);
    EXPECT_NEAR(out2, 0.5f, 0.001f);
    EXPECT_NEAR(out3, 0.25f, 0.001f);
}

TEST_F(FirstOrderCoreTest, Constants) {
    EXPECT_EQ(FirstOrderCore<float>::COEFFS_PER_SECTION, 3);
    EXPECT_EQ(FirstOrderCore<float>::STATE_VARS_PER_SECTION, 2);
}

