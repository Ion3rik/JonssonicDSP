// Jonssonic - A C++ audio DSP library
// Unit tests for WaveShaperProcessor class
// SPDX-License-Identifier: MIT

#include <gtest/gtest.h>
#include "../../../include/Jonssonic/core/nonlinear/WaveShaperProcessor.h"
#include <cmath>

using namespace Jonssonic;

// Typedef for a simple float, hardclip distortion stage
using TestWaveShaperProcessor = WaveShaperProcessor<float, WaveShaperType::HardClip>;

TEST(WaveShaperProcessor, DefaultConstruction)
{
	TestWaveShaperProcessor stage;
	SUCCEED(); // Just ensure it constructs
}

TEST(WaveShaperProcessor, PrepareAndReset)
{
	TestWaveShaperProcessor stage;
	stage.prepare(2, 44100.0f, 5.0f);
	stage.setInputGain(2.0f, true);
	stage.setOutputGain(0.5f, true);
	stage.setBias(0.1f, true);
	stage.setAsymmetry(0.2f, true);
	stage.reset();
	SUCCEED();
}

TEST(WaveShaperProcessor, ProcessSample)
{
	TestWaveShaperProcessor stage;
	stage.prepare(1, 44100.0f, 0.0f);
	stage.setInputGain(1.0f, true);
	stage.setOutputGain(1.0f, true);
	stage.setBias(0.0f, true);
	stage.setAsymmetry(0.0f, true);
	// Should act as identity for input in [-1,1]
	EXPECT_FLOAT_EQ(stage.processSample(0, 0.5f), 0.5f);
	EXPECT_FLOAT_EQ(stage.processSample(0, -0.5f), -0.5f);
	// Should hard clip outside [-1,1]
	EXPECT_FLOAT_EQ(stage.processSample(0, 2.0f), 1.0f);
	EXPECT_FLOAT_EQ(stage.processSample(0, -2.0f), -1.0f);
}

TEST(WaveShaperProcessor, ParameterEffects)
{
	TestWaveShaperProcessor stage;
	stage.prepare(1, 44100.0f, 0.0f);
	// Input gain
	stage.setInputGain(2.0f, true);
	EXPECT_FLOAT_EQ(stage.processSample(0, 0.5f), 1.0f); // 0.5*2=1, hardclip
	// Output gain
	stage.setInputGain(1.0f, true);
	stage.setOutputGain(0.5f, true);
	EXPECT_FLOAT_EQ(stage.processSample(0, 1.0f), 0.5f); // 1*1=1, hardclip, *0.5=0.5
	// Bias
	stage.setOutputGain(1.0f, true);
	stage.setBias(1.0f, true);
	EXPECT_FLOAT_EQ(stage.processSample(0, 0.0f), 1.0f); // 0+1=1, hardclip
	// Asymmetry (positive input)
	stage.setBias(0.0f, true);
	stage.setAsymmetry(1.0f, true);
	EXPECT_FLOAT_EQ(stage.processSample(0, 0.5f), 1.0f); // 0.5*(1+1)=1, hardclip
	// Asymmetry (negative input)
	stage.setAsymmetry(1.0f, true);
	EXPECT_FLOAT_EQ(stage.processSample(0, -0.5f), 0.0f); // -0.5*(1-1)=0, hardclip
}
