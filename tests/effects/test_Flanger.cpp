// Unit tests for Jonssonic::Flanger
// Author: Jon Fagerström
#include <gtest/gtest.h>
#include "Jonssonic/effects/Flanger.h"
#include <vector>

using namespace Jonssonic;

TEST(FlangerTest, BasicMonoProcess)
{
    Flanger<float> flanger;
    flanger.prepare(1, 48000.0f);
    flanger.setRate(0.5f);
    flanger.setDepth(0.7f);
    flanger.setFeedback(0.5f);
    flanger.setDelayMs(3.0f);
    flanger.setSpread(0.0f);

    constexpr size_t numSamples = 32;
    std::vector<float> input(numSamples, 0.0f);
    std::vector<float> output(numSamples, 0.0f);
    input[0] = 1.0f; // Impulse

    // Prepare pointers for processBlock
    std::vector<const float*> inPtrs = { input.data() };
    std::vector<float*> outPtrs = { output.data() };

    flanger.processBlock(inPtrs.data(), outPtrs.data(), numSamples);

    // Output should not be all zeros due to impulse response
    bool nonZero = false;
    for (float v : output) {
        if (std::abs(v) > 1e-6f) nonZero = true;
    }
    EXPECT_TRUE(nonZero) << "Output should not be all zeros after processing an impulse";
}

TEST(FlangerTest, StereoImpulseResponse)
{
    Flanger<float> flanger;
    flanger.prepare(2, 44100.0f);
    flanger.setRate(1.0f);
    flanger.setDepth(1.0f);
    flanger.setFeedback(0.0f);
    flanger.setDelayMs(5.0f);
    flanger.setSpread(1.0f);

    constexpr size_t numSamples = 64;
    std::vector<float> inputL(numSamples, 0.0f);
    std::vector<float> inputR(numSamples, 0.0f);
    std::vector<float> outputL(numSamples, 0.0f);
    std::vector<float> outputR(numSamples, 0.0f);
    inputL[0] = 1.0f; // Impulse left
    inputR[0] = 0.0f; // Silence right

    std::vector<const float*> inPtrs = { inputL.data(), inputR.data() };
    std::vector<float*> outPtrs = { outputL.data(), outputR.data() };

    flanger.processBlock(inPtrs.data(), outPtrs.data(), numSamples);

    // Check left output is non-zero
    bool nonZeroL = false;
    for (float v : outputL) {
        if (std::abs(v) > 1e-6f) nonZeroL = true;
    }
    EXPECT_TRUE(nonZeroL) << "Left channel output should not be all zeros";
    // Check right output is zero (no input, no feedback)
    bool nonZeroR = false;
    for (float v : outputR) {
        if (std::abs(v) > 1e-6f) nonZeroR = true;
    }
    EXPECT_FALSE(nonZeroR) << "Right channel output should be all zeros";
}

TEST(FlangerTest, NoGraininessWithContinuousModulation)
{
    // Test that the flanger doesn't produce grainy artifacts with realistic audio
    Flanger<float> flanger;
    float sampleRate = 44100.0f;
    flanger.prepare(1, sampleRate);
    
    // Typical flanger settings
    flanger.setRate(0.5f);        // 0.5 Hz LFO
    flanger.setDepth(0.7f);       // 70% depth
    flanger.setFeedback(0.5f);    // 50% feedback
    flanger.setDelayMs(3.0f);     // 3ms center delay
    flanger.setSpread(0.0f);      // Mono
    
    // Process 1 second of 1kHz sine wave
    size_t numSamples = static_cast<size_t>(sampleRate);
    std::vector<float> input(numSamples);
    std::vector<float> output(numSamples, 0.0f);
    
    // Generate clean 1kHz sine input
    for (size_t i = 0; i < numSamples; ++i) {
        input[i] = std::sin(2.0f * M_PI * 1000.0f * i / sampleRate);
    }
    
    const float* inPtrs[1] = { input.data() };
    float* outPtrs[1] = { output.data() };
    
    flanger.processBlock(inPtrs, outPtrs, numSamples);
    
    // Measure graininess by checking sample-to-sample derivative
    // Graininess shows up as high-frequency noise (sharp sample transitions)
    float maxDerivative = 0.0f;
    float avgDerivative = 0.0f;
    int derivativeCount = 0;
    
    // Skip first 1000 samples (warm-up)
    for (size_t i = 1000; i < numSamples - 1; ++i) {
        float derivative = std::abs(output[i+1] - output[i]);
        maxDerivative = std::max(maxDerivative, derivative);
        avgDerivative += derivative;
        derivativeCount++;
    }
    avgDerivative /= derivativeCount;
    
    // For a 1kHz sine at 44.1kHz, max derivative should be ~0.142
    // With flanger modulation and feedback, allow some headroom but not excessive
    // If we have graininess, derivatives will be much higher
    EXPECT_LT(maxDerivative, 0.4f) 
        << "Graininess detected: excessive max derivative " << maxDerivative;
    
    EXPECT_LT(avgDerivative, 0.12f)
        << "Graininess detected: excessive avg derivative " << avgDerivative;
}

TEST(FlangerTest, InterpolationQualityWithSlowSweep)
{
    // More sensitive test: check for interpolation artifacts with very slow modulation
    Flanger<float> flanger;
    float sampleRate = 44100.0f;
    flanger.prepare(1, sampleRate);
    
    // Very slow, gentle settings to isolate interpolation quality
    flanger.setRate(0.1f);        // 0.1 Hz - very slow LFO
    flanger.setDepth(0.3f);       // 30% depth - moderate modulation
    flanger.setFeedback(0.0f);    // No feedback to isolate delay line quality
    flanger.setDelayMs(5.0f);     // 5ms center delay
    flanger.setSpread(0.0f);
    
    // Process 2 seconds to capture full LFO cycle
    size_t numSamples = static_cast<size_t>(2 * sampleRate);
    std::vector<float> input(numSamples);
    std::vector<float> output(numSamples, 0.0f);
    
    // Generate clean 500Hz sine input (mid-range frequency)
    for (size_t i = 0; i < numSamples; ++i) {
        input[i] = std::sin(2.0f * M_PI * 500.0f * i / sampleRate);
    }
    
    const float* inPtrs[1] = { input.data() };
    float* outPtrs[1] = { output.data() };
    
    flanger.processBlock(inPtrs, outPtrs, numSamples);
    
    // Check for sudden jumps (clicks/graininess) in the output
    int clickCount = 0;
    float maxJump = 0.0f;
    
    for (size_t i = 5000; i < numSamples - 1; ++i) {
        float jump = std::abs(output[i+1] - output[i]);
        maxJump = std::max(maxJump, jump);
        
        // For 500Hz sine at 44.1kHz, max derivative should be ~0.071
        // Allow 3x headroom for modulation effects
        if (jump > 0.25f) {
            clickCount++;
        }
    }
    
    EXPECT_EQ(clickCount, 0) 
        << "Found " << clickCount << " clicks/discontinuities in output. Max jump: " << maxJump;
    
    EXPECT_LT(maxJump, 0.25f)
        << "Excessive discontinuity detected: " << maxJump;
}

TEST(FlangerTest, DopplerArtifactsOnTransients)
{
    // Test specifically for Doppler/pitch-shift artifacts that follow LFO rate
    // These are most audible on transients
    Flanger<float> flanger;
    float sampleRate = 44100.0f;
    flanger.prepare(1, sampleRate);
    
    // Settings that would show Doppler artifacts
    flanger.setRate(2.0f);        // 2 Hz LFO - faster to make Doppler obvious
    flanger.setDepth(1.0f);       // Full depth - maximum modulation
    flanger.setFeedback(0.0f);    // No feedback
    flanger.setDelayMs(3.0f);     // 3ms center
    flanger.setSpread(0.0f);
    
    // Generate impulse train (transients every 0.1 seconds)
    size_t numSamples = static_cast<size_t>(sampleRate); // 1 second
    std::vector<float> input(numSamples, 0.0f);
    std::vector<float> output(numSamples, 0.0f);
    
    // Place impulses
    for (size_t i = 0; i < 10; ++i) {
        size_t pos = i * 4410; // Every 0.1 seconds
        if (pos < numSamples) {
            input[pos] = 1.0f;
        }
    }
    
    const float* inPtrs[1] = { input.data() };
    float* outPtrs[1] = { output.data() };
    
    flanger.processBlock(inPtrs, outPtrs, numSamples);
    
    // Calculate modulation rate from LFO (2Hz = 2 cycles per second)
    // Max delay rate of change = depth * 2*pi*freq (for sine LFO)
    // With depth=1.0 (±3ms), rate=2Hz: max rate = 3ms * 2*pi*2 = 37.7 ms/s = 0.0377 samples/sample at 44.1kHz
    // This corresponds to pitch shift of ±3.77%
    
    // For each impulse response, check if the pitch shift is within acceptable bounds
    // We can detect pitch shift by measuring the rate of change of the output
    
    // Check the derivative around the transients
    float maxDopplerRate = 0.0f;
    for (size_t i = 0; i < 9; ++i) {
        size_t impulsePos = i * 4410;
        // Check 50 samples after each impulse
        for (size_t j = 1; j < 50 && (impulsePos + j + 1) < numSamples; ++j) {
            size_t idx = impulsePos + j;
            if (std::abs(output[idx]) > 0.01f) { // Only measure where there's signal
                float rate = std::abs(output[idx+1] - output[idx]) / (std::abs(output[idx]) + 1e-6f);
                maxDopplerRate = std::max(maxDopplerRate, rate);
            }
        }
    }
    
    // The Doppler rate should be bounded by the modulation
    // With 2Hz LFO and ±3ms modulation at 44.1kHz, expect max ~0.05 relative change per sample
    EXPECT_LT(maxDopplerRate, 0.1f)
        << "Excessive Doppler/pitch-shift artifacts detected: " << maxDopplerRate;
}

