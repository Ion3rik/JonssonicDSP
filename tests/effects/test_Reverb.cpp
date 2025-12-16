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
	Reverb<float> reverb;
	reverb.prepare(1, 48000.0f);
	reverb.setReverbTimeS(1.0f, true);
	reverb.setSize(0.5f, true);
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

	   // Stricter: Check for decaying impulse response
	   // Find the peak (should be at or near the start)
	   float peak = 0.0f;
	   size_t peakIdx = 0;
	   for (size_t i = 0; i < output.size(); ++i) {
		   if (std::abs(output[i]) > peak) {
			   peak = std::abs(output[i]);
			   peakIdx = i;
		   }
	   }
	   // Require peak is near the start
	   EXPECT_LT(peakIdx, 10u);
	   EXPECT_GT(peak, 0.1f);

	   // Check that the envelope decays: last 10% of samples should be much lower than the peak
	   float tailMax = 0.0f;
	   for (size_t i = output.size() * 9 / 10; i < output.size(); ++i) {
		   tailMax = std::max(tailMax, std::abs(output[i]));
	   }
	   EXPECT_LT(tailMax, peak * 0.1f);
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
