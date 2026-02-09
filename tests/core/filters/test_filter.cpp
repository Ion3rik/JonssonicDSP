// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Unit tests for filter class
// SPDX-License-Identifier: MIT

#pragma once
#include <gtest/gtest.h>
#include <jonssonic/core/filters/filter.h>

using namespace jnsc;

// List of filter topologies to test
typedef ::testing::Types<BiquadDF1<float>> FilterTopologies;

// Typed test fixture for Filter class
template <typename Topology>
class FilterTypedTest : public ::testing::Test {
  protected:
    static constexpr float sampleRate = 48000.0f;
    Filter<float, Topology> filter;

    void SetUp() override {}
    void TearDown() override {}
};

// Register the test suite with the list of filter topologies
TYPED_TEST_SUITE(FilterTypedTest, FilterTopologies);

// Test case for preparing the filter
TYPED_TEST(FilterTypedTest, PrepareFilter) {
    filter.prepare(2, 3, sampleRate);

    EXPECT_TRUE(filter.isPrepared());
    EXPECT_EQ(filter.getNumChannels(), 2);
    EXPECT_EQ(filter.getNumSections(), 3);
    EXPECT_EQ(filter.getSampleRate(), sampleRate);
}
