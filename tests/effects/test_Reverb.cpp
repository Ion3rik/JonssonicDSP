#include <gtest/gtest.h>
#include "Jonssonic/effects/Reverb.h"
#include <vector>

using namespace Jonssonic;

TEST(ReverbTest, ConstructionAndPrepare)
{
	Reverb<float> reverb;
	EXPECT_NO_THROW(reverb.prepare(1, 48000.0f));
	EXPECT_EQ(reverb.getNumChannels(), 1u);
	EXPECT_FLOAT_EQ(reverb.getSampleRate(), 48000.0f);
}

TEST(ReverbTest, MonoImpulseResponse)
{
	Reverb<float, 2> reverb; // Using FDN_SIZE = 2 for testing (delays of 1 and 2 samples)
	reverb.prepare(1, 48000.0f);
	reverb.setReverbTimeS(1.0f, true);
	reverb.setSize(0.5f, true); // results in delay lengths of 1 and 2 samples
	reverb.setPreDelayTimeMs(0.0f, true);
	reverb.setDamping(0.5f, true);
	reverb.setLowCutFreqHz(20.0f);

	constexpr size_t numSamples = 48000;
	std::vector<float> input(numSamples, 0.0f);
	std::vector<float> output(numSamples, 0.0f);
	input[0] = 1.0f; // Impulse
	const float* inPtrs[1] = { input.data() };
	float* outPtrs[1] = { output.data() };

	reverb.processBlock(inPtrs, outPtrs, numSamples);

	   // Stricter: Check for echoes at expected sample indices for FDN_SIZE=2 (delays 1 and 2 samples)
	   // The first echo should be at sample 1, the second at sample 2, etc.
	   // We expect nonzero output at indices 0, 1, 2, ...
	   // Check first few echoes
	   EXPECT_NEAR(output[0], 0.0f, 1e-6f); // FDN output is zero at n=0 (impulse enters delay lines)
	   EXPECT_NEAR(output[1], 1.0f, 1e-3f); // First echo (delay=1)
	   EXPECT_NEAR(output[2], 1.0f, 1e-3f); // Second echo (delay=2)
	   // Check that subsequent echoes decay (roughly)
	   float prev = std::abs(output[1]);
	   for (size_t i = 2; i < 10; ++i) {
		   EXPECT_LT(std::abs(output[i]), prev + 1e-2f); // Should not grow
		   prev = std::abs(output[i]);
	   }
	   // Check that the tail is much lower than the first echo
	   float tailMax = 0.0f;
	   for (size_t i = output.size() * 9 / 10; i < output.size(); ++i) {
		   tailMax = std::max(tailMax, std::abs(output[i]));
	   }
	   EXPECT_LT(tailMax, 0.1f);
}

TEST(ReverbTest, ParameterSetters)
{
	Reverb<float> reverb;
	reverb.prepare(2, 44100.0f);
	EXPECT_NO_THROW(reverb.setReverbTimeS(2.5f));
	EXPECT_NO_THROW(reverb.setSize(0.8f));
	EXPECT_NO_THROW(reverb.setPreDelayTimeMs(100.0f));
	EXPECT_NO_THROW(reverb.setDamping(0.3f));
	EXPECT_NO_THROW(reverb.setLowCutFreqHz(200.0f));
}
