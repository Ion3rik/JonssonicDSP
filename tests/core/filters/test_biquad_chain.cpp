// Jonssonic - A C++ audio DSP library
// Unit tests for the BiquadChain class
// SPDX-License-Identifier: MIT

#include <gtest/gtest.h>
#include <jonssonic/core/filters/biquad_chain.h>

using namespace jonssonic::core::filters;

TEST(BiquadChain, ConstructionAndPrepare)
{
	BiquadChain<float> chain;
	chain.prepare(1, 1, 48000.0f);
	EXPECT_EQ(chain.processSample(0, 0.0f), 0.0f);
}

TEST(BiquadChain, SetParameters)
{
	BiquadChain<float> chain;
	chain.prepare(1, 1, 48000.0f);
	chain.setFreq(0, 1000.0f);
	chain.setQ(0, 0.707f);
	chain.setGainDb(0, 6.0f);
	chain.setType(0, BiquadType::Peak);
	// No assertion, just check for exceptions/crashes
}

TEST(BiquadChain, ProcessImpulse)
{
	BiquadChain<float> chain;
	chain.prepare(1, 1, 48000.0f);
	chain.setType(0, BiquadType::Lowpass);
	chain.setFreq(0, 1000.0f);
	chain.setQ(0, 0.707f);

	float input[8] = {1.0f, 0, 0, 0, 0, 0, 0, 0};
	float output[8] = {0};
	const float* inPtrs[1] = {input};
	float* outPtrs[1] = {output};

	chain.processBlock(inPtrs, outPtrs, 8);

    // Output should be nonzero and finite
    for (int i = 0; i < 8; ++i) {
        EXPECT_TRUE(std::isfinite(output[i]));
    }
    bool anyNonZero = false;
    for (int i = 0; i < 8; ++i) {
        if (std::abs(output[i]) > 1e-6f) {
            anyNonZero = true;
            break;
        }
    }
    EXPECT_TRUE(anyNonZero);
}

TEST(BiquadChain, MultipleSections)
{
	BiquadChain<float> chain;
	chain.prepare(1, 2, 48000.0f);
	// Section 0: Lowpass, Section 1: Highpass
	chain.setType(0, BiquadType::Lowpass);
	chain.setFreq(0, 5000.0f);
	chain.setQ(0, 0.707f);
	chain.setType(1, BiquadType::Highpass);
	chain.setFreq(1, 500.0f);
	chain.setQ(1, 0.707f);

	float input[8] = {1.0f, 0, 0, 0, 0, 0, 0, 0};
	float output[8] = {0};
	const float* inPtrs[1] = {input};
	float* outPtrs[1] = {output};

	chain.processBlock(inPtrs, outPtrs, 8);

    // Output should be nonzero and finite
    for (int i = 0; i < 8; ++i) {
        EXPECT_TRUE(std::isfinite(output[i]));
    }
    bool anyNonZero = false;
    for (int i = 0; i < 8; ++i) {
        if (std::abs(output[i]) > 1e-6f) {
            anyNonZero = true;
            break;
        }
    }
    EXPECT_TRUE(anyNonZero);
}
