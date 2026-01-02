#include <gtest/gtest.h>
#include <jonssonic/core/dynamics/envelope_follower.h>
#include <vector>

using namespace jonssonic::core::dynamics;

TEST(EnvelopeFollowerTest, PeakTracksStepInput) {
	EnvelopeFollower<float, EnvelopeType::Peak> env;
	env.prepare(1, 48000.0f);
	env.setAttackTime(1.0f, true); // fast attack
	env.setReleaseTime(100.0f, true); // slow release
	env.reset(0.0f);
	float input = 1.0f;
	float last = 0.0f;
	// Attack phase
	for (int i = 0; i < 10; ++i) {
		last = env.processSample(0, input);
	}
	EXPECT_GT(last, 0.0f);
	// Release phase
	input = 0.0f;
	float release = env.processSample(0, input);
	EXPECT_LT(release, last);
}

TEST(EnvelopeFollowerTest, RMSMatchesExpected) {
	EnvelopeFollower<float, EnvelopeType::RMS> env;
	env.prepare(1, 48000.0f);
	env.setAttackTime(1.0f, true);
	env.setReleaseTime(1.0f, true);
	env.reset(0.0f);
	float input = 1.0f;
	float last = 0.0f;
	for (int i = 0; i < 100; ++i) {
		last = env.processSample(0, input);
	}
	// RMS of constant 1.0 should approach 1.0
	EXPECT_NEAR(last, 1.0f, 0.01f);
}

TEST(EnvelopeFollowerTest, ResetSetsEnvelope) {
	EnvelopeFollower<float, EnvelopeType::Peak> env;
	env.prepare(1, 48000.0f);
	env.reset(0.0f);
	float val = env.processSample(0, 0.0f);
	EXPECT_FLOAT_EQ(val, env.processSample(0, 0.0f));
}

TEST(EnvelopeFollowerTest, MultiChannelIndependence) {
	EnvelopeFollower<float, EnvelopeType::Peak> env;
	env.prepare(2, 48000.0f);
	env.reset(0.0f);
	env.processSample(0, 1.0f);
	env.processSample(1, 0.0f);
	float ch0 = env.processSample(0, 1.0f);
	float ch1 = env.processSample(1, 0.0f);
	EXPECT_GT(ch0, ch1);
}
