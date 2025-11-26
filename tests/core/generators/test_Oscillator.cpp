// Jonssonic - A C++ audio DSP library
// Unit tests for the Oscillator class
// Author: Jon Fagerström
// Update: 19.11.2025

#include <gtest/gtest.h>
#include <cmath>
#include "Jonssonic/core/generators/Oscillator.h"

namespace Jonssonic {

class OscillatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize oscillators with different configurations
        osc.prepare(2, 44100.0f);
        osc.setFrequency(440.0f);
        osc.setWaveform(Waveform::Sine);
    }

    void TearDown() override {
    }

    // Helper: Allocate output buffers
    void allocateBuffers(size_t numChannels, size_t numSamples) {
        output.resize(numChannels);
        outputData.resize(numChannels * numSamples);
        
        for (size_t ch = 0; ch < numChannels; ++ch) {
            output[ch] = &outputData[ch * numSamples];
        }
    }

    // Helper: Allocate phase modulation buffers
    void allocatePhaseModBuffers(size_t numChannels, size_t numSamples, float modValue = 0.0f) {
        phaseMod.resize(numChannels);
        phaseModData.resize(numChannels * numSamples, modValue);
        
        for (size_t ch = 0; ch < numChannels; ++ch) {
            phaseMod[ch] = &phaseModData[ch * numSamples];
        }
    }

    Oscillator<float> osc;
    std::vector<float*> output;
    std::vector<float> outputData;
    std::vector<const float*> phaseMod;
    std::vector<float> phaseModData;
};

// ============================================================================
// Construction and Initialization Tests
// ============================================================================

TEST_F(OscillatorTest, ConstructorInitialization) {
    Oscillator<float> newOsc;
    newOsc.prepare(2, 48000.0f);
    // Should construct without errors
    SUCCEED();
}

TEST_F(OscillatorTest, PrepareResetsState) {
    osc.setFrequency(440.0f);
    allocateBuffers(2, 10);
    
    // Generate some samples
    osc.processBlock(output.data(), 10);
    
    // Prepare should reset phase
    osc.prepare(2, 44100.0f);
    osc.setFrequency(440.0f);
    
    // First sample after prepare should be at phase 0 (sine = 0.0)
    float outputSample = osc.processSample(0);
    EXPECT_NEAR(outputSample, 0.0f, 0.001f);
}

// ============================================================================
// Frequency Setting Tests
// ============================================================================

TEST_F(OscillatorTest, SetFrequencyAllChannels) {
    osc.setFrequency(1000.0f);
    
    allocateBuffers(2, 44100); // 1 second at 1000 Hz = 1000 cycles
    osc.processBlock(output.data(), 44100);
    
    // After 1 second at 1000 Hz, should complete 1000 cycles
    // Allow larger tolerance due to numerical accumulation
    EXPECT_NEAR(output[0][44099], output[0][0], 0.2f);
}

TEST_F(OscillatorTest, SetFrequencyPerChannel) {
    osc.setFrequency(440.0f, 0);  // Left channel: A4
    osc.setFrequency(880.0f, 1);  // Right channel: A5 (octave higher)
    
    allocateBuffers(2, 100);
    osc.processBlock(output.data(), 100);
    
    // Right channel should oscillate twice as fast
    // Frequencies should differ
    EXPECT_NE(output[0][50], output[1][50]);
}

// ============================================================================
// Waveform Generation Tests
// ============================================================================

TEST_F(OscillatorTest, SineWaveform) {
    osc.setWaveform(Waveform::Sine);
    osc.setFrequency(440.0f);
    osc.reset();
    
    
    float outputSample = osc.processSample(0);
    
    // Sine at phase 0 should be 0
    EXPECT_NEAR(outputSample, 0.0f, 0.001f);
}

TEST_F(OscillatorTest, SawWaveform) {
    osc.setWaveform(Waveform::Saw);
    osc.reset();
    
    float outputSample = osc.processSample(0);
    // Saw at phase 0: 2*0 - 1 = -1
    EXPECT_NEAR(outputSample, -1.0f, 0.001f);
}

TEST_F(OscillatorTest, SquareWaveform) {
    osc.setWaveform(Waveform::Square);
    osc.reset();
    
    float outputSample = osc.processSample(0);
    // Square at phase 0 (< 0.5): -1
    EXPECT_NEAR(outputSample, -1.0f, 0.001f);
}

TEST_F(OscillatorTest, TriangleWaveform) {
    osc.setWaveform(Waveform::Triangle);
    osc.reset();
    
    float outputSample = osc.processSample(0);
    // Triangle at phase 0: 4*0 - 1 = -1
    EXPECT_NEAR(outputSample, -1.0f, 0.001f);
}

TEST_F(OscillatorTest, SineWaveSymmetry) {
    osc.setWaveform(Waveform::Sine);
    osc.setFrequency(1000.0f);
    osc.reset();
    
    // Generate one full cycle (44.1 samples at 1000 Hz)
    size_t samplesPerCycle = 44;
    allocateBuffers(2, samplesPerCycle);
    osc.processBlock(output.data(), samplesPerCycle);
    
    // Sine wave should be symmetric: sample[quarter] ≈ -sample[3*quarter]
    size_t quarter = samplesPerCycle / 4;
    EXPECT_NEAR(output[0][quarter], -output[0][3 * quarter], 0.1f);
}

TEST_F(OscillatorTest, SawWaveRamps) {
    osc.setWaveform(Waveform::Saw);
    osc.setFrequency(1000.0f);
    osc.reset();
    
    allocateBuffers(2, 100);
    osc.processBlock(output.data(), 100);
    
    // Sawtooth should generally increase within a cycle
    // (except at the discontinuity)
    bool foundIncrease = false;
    for (size_t i = 1; i < 100; ++i) {
        if (output[0][i] > output[0][i-1]) {
            foundIncrease = true;
            break;
        }
    }
    EXPECT_TRUE(foundIncrease);
}

// ============================================================================
// Phase Reset Tests
// ============================================================================

TEST_F(OscillatorTest, ResetAllChannels) {
    osc.setFrequency(440.0f);
    allocateBuffers(2, 10);
    
    // Generate some samples
    osc.processBlock(output.data(), 10);
    
    // Reset
    osc.reset();
    
    float outputSample = osc.processSample(0);
    
    // Should be back at phase 0
    EXPECT_NEAR(outputSample, 0.0f, 0.001f);
}

TEST_F(OscillatorTest, ResetSingleChannel) {
    osc.setFrequency(440.0f);
    allocateBuffers(2, 10);
    
    // Generate some samples
    osc.processBlock(output.data(), 10);
    
    // Reset only channel 0
    osc.reset(0);
    
    float outputSample0 = osc.processSample(0);
    float outputSample1 = osc.processSample(1);
    
    // Channel 0 should be at phase 0
    EXPECT_NEAR(outputSample0, 0.0f, 0.001f);
    // Channel 1 should not be at phase 0 (continued from before)
    EXPECT_GT(std::abs(outputSample1), 0.001f);
}

// ============================================================================
// Single Sample Processing Tests
// ============================================================================

TEST_F(OscillatorTest, ProcessSampleNoModulation) {
    osc.setFrequency(440.0f);
    osc.reset();
    
    float outputSample[2];
    
    // Process multiple single samples
    for (int i = 0; i < 10; ++i) {
        outputSample[0] = osc.processSample(0);
        outputSample[1] = osc.processSample(1);
        
        // Output should be in valid range [-1, 1]
        EXPECT_GE(outputSample[0], -1.0f);
        EXPECT_LE(outputSample[0], 1.0f);
        EXPECT_GE(outputSample[1], -1.0f);
        EXPECT_LE(outputSample[1], 1.0f);
    }
}

TEST_F(OscillatorTest, ProcessSampleWithModulation) {
    osc.setFrequency(440.0f);
    osc.reset();
    
    float phaseMod[2] = {0.25f, 0.25f}; // 90° phase shift
    
    float outputSample = osc.processSample(0, phaseMod[0]);
    
    // With 0.25 phase offset, sine should be at peak (≈1.0)
    EXPECT_NEAR(outputSample, 1.0f, 0.001f);
}

// ============================================================================
// Block Processing Tests
// ============================================================================

TEST_F(OscillatorTest, ProcessBlockNoModulation) {
    osc.setFrequency(440.0f);
    osc.reset();
    
    size_t blockSize = 512;
    allocateBuffers(2, blockSize);
    
    osc.processBlock(output.data(), blockSize);
    
    // All samples should be in valid range
    for (size_t i = 0; i < blockSize; ++i) {
        EXPECT_GE(output[0][i], -1.0f) << "Sample " << i;
        EXPECT_LE(output[0][i], 1.0f) << "Sample " << i;
    }
}

TEST_F(OscillatorTest, ProcessBlockWithModulation) {
    osc.setFrequency(440.0f);
    osc.reset();
    
    size_t blockSize = 100;
    allocateBuffers(2, blockSize);
    allocatePhaseModBuffers(2, blockSize, 0.0f);
    
    // Add 180° phase shift modulation throughout
    for (size_t i = 0; i < 100; ++i) {
        phaseModData[i] = 0.5f; // 180° phase shift
        phaseModData[100 + i] = 0.5f;
    }
    
    osc.processBlock(output.data(), phaseMod.data(), blockSize);
    
    // With 180° phase shift, output should be roughly inverted compared to no modulation
    // Just verify modulation is having an effect
    bool hasVariation = false;
    for (size_t i = 1; i < blockSize; ++i) {
        if (std::abs(output[0][i] - output[0][i-1]) > 0.01f) {
            hasVariation = true;
            break;
        }
    }
    EXPECT_TRUE(hasVariation);
}

// ============================================================================
// Multi-Channel Tests
// ============================================================================

TEST_F(OscillatorTest, MultiChannelIndependence) {
    Oscillator<float> quadOsc;
    quadOsc.prepare(4, 44100.0f);
    
    // Set different frequencies for each channel
    quadOsc.setFrequency(440.0f, 0);
    quadOsc.setFrequency(880.0f, 1);
    quadOsc.setFrequency(1320.0f, 2);
    quadOsc.setFrequency(1760.0f, 3);
    quadOsc.setWaveform(Waveform::Sine);
    
    std::vector<float*> quadOutput(4);
    std::vector<float> quadOutputData(4 * 100);
    for (size_t ch = 0; ch < 4; ++ch) {
        quadOutput[ch] = &quadOutputData[ch * 100];
    }
    
    quadOsc.processBlock(quadOutput.data(), 100);
    
    // Each channel should have different output
    EXPECT_NE(quadOutput[0][50], quadOutput[1][50]);
    EXPECT_NE(quadOutput[1][50], quadOutput[2][50]);
    EXPECT_NE(quadOutput[2][50], quadOutput[3][50]);
}

// ============================================================================
// Phase Wrapping Tests
// ============================================================================

TEST_F(OscillatorTest, PhaseWrapsCorrectly) {
    osc.setFrequency(44100.0f); // Very high frequency
    osc.reset();
    
    allocateBuffers(2, 100);
    osc.processBlock(output.data(), 100);
    
    // Should not have any NaN or Inf values despite many wraps
    for (size_t i = 0; i < 100; ++i) {
        EXPECT_FALSE(std::isnan(output[0][i])) << "NaN at sample " << i;
        EXPECT_FALSE(std::isinf(output[0][i])) << "Inf at sample " << i;
        EXPECT_GE(output[0][i], -1.1f) << "Out of range at sample " << i;
        EXPECT_LE(output[0][i], 1.1f) << "Out of range at sample " << i;
    }
}

TEST_F(OscillatorTest, LargePhaseModulationWraps) {
    osc.setFrequency(440.0f);
    osc.reset();
    
    float phaseMod[2] = {5.7f, 5.7f}; // Large modulation value
    
    float outputSample = osc.processSample(0, phaseMod[0]);
    
    // Should wrap correctly and produce valid output
    EXPECT_GE(outputSample, -1.0f);
    EXPECT_LE(outputSample, 1.0f);
    EXPECT_FALSE(std::isnan(outputSample));
}

TEST_F(OscillatorTest, NegativePhaseModulationWraps) {
    osc.setFrequency(440.0f);
    osc.reset();
    
    float phaseMod[2] = {-2.3f, -2.3f}; // Negative modulation
    float outputSample = osc.processSample(0, phaseMod[0]);
    
    // Should wrap correctly and produce valid output
    EXPECT_GE(outputSample, -1.0f);
    EXPECT_LE(outputSample, 1.0f);
    EXPECT_FALSE(std::isnan(outputSample));
}

// ============================================================================
// Edge Cases and Robustness Tests
// ============================================================================

TEST_F(OscillatorTest, ZeroFrequency) {
    osc.setFrequency(0.0f);
    osc.reset();
    
    allocateBuffers(2, 100);
    osc.processBlock(output.data(), 100);
    
    // All samples should be the same (DC at phase 0)
    for (size_t i = 1; i < 100; ++i) {
        EXPECT_FLOAT_EQ(output[0][i], output[0][0]);
    }
}

TEST_F(OscillatorTest, VeryLowFrequency) {
    osc.setFrequency(0.1f); // 0.1 Hz
    osc.reset();
    
    allocateBuffers(2, 100);
    osc.processBlock(output.data(), 100);
    
    // Should produce valid output
    for (size_t i = 0; i < 100; ++i) {
        EXPECT_FALSE(std::isnan(output[0][i]));
        EXPECT_GE(output[0][i], -1.0f);
        EXPECT_LE(output[0][i], 1.0f);
    }
}

TEST_F(OscillatorTest, NyquistFrequency) {
    osc.setFrequency(22050.0f); // Nyquist at 44100 Hz
    osc.reset();
    
    allocateBuffers(2, 100);
    osc.processBlock(output.data(), 100);
    
    // Should alternate between extremes (aliasing expected without anti-aliasing)
    EXPECT_FALSE(std::isnan(output[0][50]));
}

// ============================================================================
// Consistency Tests
// ============================================================================

TEST_F(OscillatorTest, ProcessSampleMatchesProcessBlock) {
    osc.setFrequency(440.0f);
    osc.setWaveform(Waveform::Sine);
    
    // Process with processSample
    Oscillator<float> osc1;
    osc1.prepare(2, 44100.0f);
    osc1.setFrequency(440.0f);
    osc1.setWaveform(Waveform::Sine);
    
    std::vector<float*> output1(2);
    std::vector<float> outputData1(2 * 10);
    for (size_t ch = 0; ch < 2; ++ch) {
        output1[ch] = &outputData1[ch * 10];
    }
    
    for (size_t i = 0; i < 10; ++i) {
        output1[0][i] = osc1.processSample(0, 0.0f);
        output1[1][i] = osc1.processSample(1, 0.0f);
    }
    
    // Process with processBlock
    Oscillator<float> osc2;
    osc2.prepare(2, 44100.0f);
    osc2.setFrequency(440.0f);
    osc2.setWaveform(Waveform::Sine);
    
    std::vector<float*> output2(2);
    std::vector<float> outputData2(2 * 10);
    for (size_t ch = 0; ch < 2; ++ch) {
        output2[ch] = &outputData2[ch * 10];
    }
    
    osc2.processBlock(output2.data(), 10);
    
    // Should produce identical output
    for (size_t i = 0; i < 10; ++i) {
        EXPECT_FLOAT_EQ(output1[0][i], output2[0][i]) << "Sample " << i;
        EXPECT_FLOAT_EQ(output1[1][i], output2[1][i]) << "Sample " << i;
    }
}

// ============================================================================
// Discontinuity Detection Tests
// ============================================================================

TEST_F(OscillatorTest, NoDiscontinuityInSineWave) {
    osc.setWaveform(Waveform::Sine);
    osc.setFrequency(440.0f);
    osc.reset();
    
    allocateBuffers(2, 1000);
    osc.processBlock(output.data(), 1000);
    
    // Check for abrupt discontinuities (jumps > expected max derivative)
    // For sine wave at 440Hz @ 44100Hz sample rate:
    // Max derivative ≈ 2*pi*f*amplitude = 2*pi*440*1 ≈ 2764
    // Per sample: 2764/44100 ≈ 0.063
    float maxExpectedJump = 0.1f; // Allow some margin
    
    for (size_t i = 1; i < 1000; ++i) {
        float diff = std::abs(output[0][i] - output[0][i-1]);
        EXPECT_LT(diff, maxExpectedJump) 
            << "Discontinuity detected at sample " << i 
            << ": jump of " << diff;
    }
}

TEST_F(OscillatorTest, FrequencyChangeDoesNotCausePhaseJump) {
    // This test checks that the VALUE doesn't jump (phase continuity)
    // but NOTE: the derivative WILL jump without smoothing
    osc.setWaveform(Waveform::Sine);
    osc.setFrequency(440.0f);
    osc.reset();
    
    allocateBuffers(2, 200);
    
    // Generate first half at 440Hz
    osc.processBlock(output.data(), 100);
    float lastSample = output[0][99];
    
    // Change frequency and continue
    osc.setFrequency(880.0f);
    
    // Generate second half at 880Hz
    float** outputSecondHalf = new float*[2];
    outputSecondHalf[0] = output[0] + 100;
    outputSecondHalf[1] = output[1] + 100;
    osc.processBlock(outputSecondHalf, 100);
    delete[] outputSecondHalf;
    
    float firstSampleAfterChange = output[0][100];
    
    // The VALUE should be continuous (no phase jump)
    // For sine at 440Hz, max change per sample is ~0.063
    // For sine at 880Hz, max change per sample is ~0.126
    float maxExpectedJump = 0.15f; // Allow for higher frequency
    float actualJump = std::abs(firstSampleAfterChange - lastSample);
    
    EXPECT_LT(actualJump, maxExpectedJump) 
        << "Phase discontinuity detected when changing frequency: jump of " << actualJump;
}

TEST_F(OscillatorTest, FrequencyChangeShouldBeSmoothNotAbrupt) {
    // FAIL TEST: This test should FAIL until frequency smoothing is implemented
    // It verifies that frequency changes are smooth (no derivative discontinuity)
    osc.setWaveform(Waveform::Sine);
    osc.setFrequency(440.0f);
    osc.reset();
    
    std::vector<float> samples(200);
    
    // Generate 100 samples at 440Hz
    for (int i = 0; i < 100; ++i) {
        samples[i] = osc.processSample(0);
    }
    
    // Calculate average derivative magnitude before change
    float avgDerivativeBefore = 0.0f;
    for (int i = 90; i < 99; ++i) {
        avgDerivativeBefore += std::abs(samples[i+1] - samples[i]);
    }
    avgDerivativeBefore /= 9.0f;
    
    // Change frequency to double
    osc.setFrequency(880.0f);
    
    // Generate 100 more samples at 880Hz
    for (int i = 100; i < 200; ++i) {
        samples[i] = osc.processSample(0);
    }
    
    // Calculate average derivative magnitude after change
    float avgDerivativeAfter = 0.0f;
    for (int i = 101; i < 110; ++i) {
        avgDerivativeAfter += std::abs(samples[i+1] - samples[i]);
    }
    avgDerivativeAfter /= 9.0f;
    
    float derivativeRatio = avgDerivativeAfter / avgDerivativeBefore;
    
    // With proper smoothing, derivative should transition gradually
    // Ratio should be close to 1.0 initially (gradual ramp to new frequency)
    // WITHOUT smoothing, it jumps to ~1.6-2.0 immediately (AUDIBLE CLICK)
    EXPECT_NEAR(derivativeRatio, 1.0f, 0.3f) 
        << "AUDIBLE BUG: Frequency change causes derivative discontinuity (click/pop). "
        << "Derivative ratio: " << derivativeRatio 
        << " (expected ~1.0 with smoothing, got " << derivativeRatio << ")"
        << "\nImplement frequency smoothing to fix this.";
}

TEST_F(OscillatorTest, WaveformChangePreservesPhase) {
    osc.setFrequency(440.0f);
    osc.setWaveform(Waveform::Sine);
    osc.reset();
    
    // Generate some samples
    allocateBuffers(2, 50);
    osc.processBlock(output.data(), 50);
    
    // Store the last value before waveform change
    float lastSineValue = output[0][49];
    
    // Change to triangle waveform
    osc.setWaveform(Waveform::Triangle);
    
    // Generate more samples
    float** outputSecondHalf = new float*[2];
    outputSecondHalf[0] = output[0] + 50;
    outputSecondHalf[1] = output[1] + 50;
    osc.processBlock(outputSecondHalf, 50);
    delete[] outputSecondHalf;
    
    // The phase should have continued (not reset)
    // Verify by checking that the oscillation continues
    // (not starting from phase 0)
    float firstTriangleValue = output[0][50];
    
    // Triangle at phase 0 is -1.0
    // If phase was preserved, we shouldn't be exactly at -1.0
    // (unless we were at exactly the right phase, which is unlikely)
    bool phaseWasPreserved = std::abs(firstTriangleValue - (-1.0f)) > 0.1f;
    
    EXPECT_TRUE(phaseWasPreserved) 
        << "Phase appears to have been reset when changing waveform";
}

TEST_F(OscillatorTest, NoDCOffset) {
    osc.setWaveform(Waveform::Sine);
    osc.setFrequency(440.0f);
    osc.reset();
    
    // Generate many cycles
    size_t numSamples = 44100; // 1 second
    allocateBuffers(2, numSamples);
    osc.processBlock(output.data(), numSamples);
    
    // Calculate DC offset (average value)
    float sum = 0.0f;
    for (size_t i = 0; i < numSamples; ++i) {
        sum += output[0][i];
    }
    float dcOffset = sum / numSamples;
    
    // For a sine wave, DC offset should be very close to 0
    EXPECT_NEAR(dcOffset, 0.0f, 0.01f) 
        << "Significant DC offset detected: " << dcOffset;
}

TEST_F(OscillatorTest, SawtoothNoDCOffset) {
    osc.setWaveform(Waveform::Saw);
    osc.setFrequency(440.0f);
    osc.reset();
    
    size_t numSamples = 44100;
    allocateBuffers(2, numSamples);
    osc.processBlock(output.data(), numSamples);
    
    float sum = 0.0f;
    for (size_t i = 0; i < numSamples; ++i) {
        sum += output[0][i];
    }
    float dcOffset = sum / numSamples;
    
    EXPECT_NEAR(dcOffset, 0.0f, 0.01f) 
        << "DC offset in sawtooth: " << dcOffset;
}

TEST_F(OscillatorTest, SineWaveZeroCrossings) {
    osc.setWaveform(Waveform::Sine);
    osc.setFrequency(1000.0f); // 1000 Hz
    osc.reset();
    
    // At 44100 Hz sample rate, 1000 Hz sine should have ~44.1 samples per cycle
    // So in 4410 samples (0.1 seconds), we should have ~100 cycles = 200 zero crossings
    size_t numSamples = 4410;
    allocateBuffers(2, numSamples);
    osc.processBlock(output.data(), numSamples);
    
    // Count zero crossings
    int zeroCrossings = 0;
    for (size_t i = 1; i < numSamples; ++i) {
        if ((output[0][i-1] < 0.0f && output[0][i] >= 0.0f) ||
            (output[0][i-1] >= 0.0f && output[0][i] < 0.0f)) {
            zeroCrossings++;
        }
    }
    
    // Should have approximately 200 zero crossings (±10 for tolerance)
    EXPECT_NEAR(zeroCrossings, 200, 10) 
        << "Unexpected number of zero crossings: " << zeroCrossings;
}

TEST_F(OscillatorTest, PhaseModulationContinuity) {
    osc.setWaveform(Waveform::Sine);
    osc.setFrequency(440.0f);
    osc.reset();
    
    size_t numSamples = 1000;
    allocateBuffers(2, numSamples);
    allocatePhaseModBuffers(2, numSamples, 0.0f);
    
    // Apply smooth phase modulation (sine wave modulation)
    for (size_t i = 0; i < numSamples; ++i) {
        // Modulate with slow sine wave (1 Hz)
        float modPhase = static_cast<float>(i) / 44100.0f;
        float modAmount = 0.1f * std::sin(2.0f * M_PI * modPhase);
        phaseModData[i] = modAmount;
        phaseModData[numSamples + i] = modAmount;
    }
    
    osc.processBlock(output.data(), phaseMod.data(), numSamples);
    
    // Check for discontinuities
    float maxExpectedJump = 0.15f; // Higher tolerance due to modulation
    for (size_t i = 1; i < numSamples; ++i) {
        float diff = std::abs(output[0][i] - output[0][i-1]);
        EXPECT_LT(diff, maxExpectedJump) 
            << "Discontinuity with phase modulation at sample " << i;
    }
}

TEST_F(OscillatorTest, RapidFrequencyChangesNoCrash) {
    osc.setWaveform(Waveform::Sine);
    osc.reset();
    
    allocateBuffers(2, 1000);
    
    // Rapidly change frequency while processing
    for (size_t i = 0; i < 1000; i += 10) {
        float freq = 220.0f + (i % 100) * 10.0f; // Vary between 220-1210 Hz
        osc.setFrequency(freq);
        
        float** outputChunk = new float*[2];
        outputChunk[0] = output[0] + i;
        outputChunk[1] = output[1] + i;
        osc.processBlock(outputChunk, 10);
        delete[] outputChunk;
    }
    
    // Should not crash or produce NaN/Inf
    for (size_t i = 0; i < 1000; ++i) {
        EXPECT_FALSE(std::isnan(output[0][i])) << "NaN at sample " << i;
        EXPECT_FALSE(std::isinf(output[0][i])) << "Inf at sample " << i;
        EXPECT_GE(output[0][i], -1.1f);
        EXPECT_LE(output[0][i], 1.1f);
    }
}

TEST_F(OscillatorTest, SquareWaveTransitionSmoothnessCheck) {
    osc.setWaveform(Waveform::Square);
    osc.setFrequency(440.0f);
    osc.reset();
    
    allocateBuffers(2, 1000);
    osc.processBlock(output.data(), 1000);
    
    // Square wave should only have transitions of ±2.0 (from -1 to +1 or vice versa)
    // All other consecutive samples should be identical
    for (size_t i = 1; i < 1000; ++i) {
        float diff = std::abs(output[0][i] - output[0][i-1]);
        // Should be either 0 (same value) or 2.0 (transition)
        EXPECT_TRUE(diff < 0.01f || std::abs(diff - 2.0f) < 0.01f)
            << "Unexpected transition in square wave at sample " << i 
            << ": diff = " << diff;
    }
}

TEST_F(OscillatorTest, TriangleWaveMonotonicSegments) {
    osc.setWaveform(Waveform::Triangle);
    osc.setFrequency(440.0f);
    osc.reset();
    
    allocateBuffers(2, 500);
    osc.processBlock(output.data(), 500);
    
    // Triangle wave should be monotonic (always increasing or always decreasing)
    // within each half-cycle, except at the peaks
    int directionChanges = 0;
    int lastDirection = 0; // 1 = increasing, -1 = decreasing
    
    for (size_t i = 1; i < 500; ++i) {
        float diff = output[0][i] - output[0][i-1];
        if (std::abs(diff) > 0.001f) { // Ignore tiny differences
            int currentDirection = (diff > 0) ? 1 : -1;
            if (lastDirection != 0 && currentDirection != lastDirection) {
                directionChanges++;
            }
            lastDirection = currentDirection;
        }
    }
    
    // Should have approximately the right number of direction changes
    // At 440 Hz and 44100 sample rate: ~100 samples per cycle
    // 500 samples = 5 cycles = 10 direction changes (2 per cycle)
    EXPECT_NEAR(directionChanges, 10, 3) 
        << "Unexpected number of direction changes in triangle wave: " << directionChanges;
}

TEST_F(OscillatorTest, PhaseModulationExtremeValues) {
    osc.setWaveform(Waveform::Sine);
    osc.setFrequency(440.0f);
    osc.reset();
    
    size_t numSamples = 100;
    allocateBuffers(2, numSamples);
    allocatePhaseModBuffers(2, numSamples, 0.0f);
    
    // Apply extreme modulation values
    for (size_t i = 0; i < numSamples; ++i) {
        // Random-like modulation between -10 and +10
        float modValue = -10.0f + (i % 20);
        phaseModData[i] = modValue;
        phaseModData[numSamples + i] = modValue;
    }
    
    osc.processBlock(output.data(), phaseMod.data(), numSamples);
    
    // Should handle extreme modulation without NaN/Inf
    for (size_t i = 0; i < numSamples; ++i) {
        EXPECT_FALSE(std::isnan(output[0][i]));
        EXPECT_FALSE(std::isinf(output[0][i]));
        EXPECT_GE(output[0][i], -1.1f);
        EXPECT_LE(output[0][i], 1.1f);
    }
}

TEST_F(OscillatorTest, LongRunningStabilityTest) {
    osc.setWaveform(Waveform::Sine);
    osc.setFrequency(440.0f);
    osc.reset();
    
    // Process many samples to check for phase accumulation errors
    size_t numSamples = 441000; // 10 seconds
    allocateBuffers(2, numSamples);
    osc.processBlock(output.data(), numSamples);
    
    // Check last few samples for validity
    for (size_t i = numSamples - 100; i < numSamples; ++i) {
        EXPECT_FALSE(std::isnan(output[0][i])) << "NaN at sample " << i;
        EXPECT_FALSE(std::isinf(output[0][i])) << "Inf at sample " << i;
        EXPECT_GE(output[0][i], -1.1f) << "Out of range at sample " << i;
        EXPECT_LE(output[0][i], 1.1f) << "Out of range at sample " << i;
    }
    
    // Check that we still have proper oscillation (not stuck at a value)
    float maxValue = -2.0f;
    float minValue = 2.0f;
    for (size_t i = numSamples - 1000; i < numSamples; ++i) {
        maxValue = std::max(maxValue, output[0][i]);
        minValue = std::min(minValue, output[0][i]);
    }
    
    // Should have full amplitude range
    EXPECT_GT(maxValue, 0.9f) << "Oscillator amplitude decreased over time";
    EXPECT_LT(minValue, -0.9f) << "Oscillator amplitude decreased over time";
}

TEST_F(OscillatorTest, MultiChannelPhaseAlignment) {
    osc.prepare(2, 44100.0f);
    osc.setFrequency(440.0f); // Same frequency for both channels
    osc.setWaveform(Waveform::Sine);
    osc.reset(); // Both start at phase 0
    
    allocateBuffers(2, 1000);
    osc.processBlock(output.data(), 1000);
    
    // Both channels should produce identical output
    for (size_t i = 0; i < 1000; ++i) {
        EXPECT_FLOAT_EQ(output[0][i], output[1][i]) 
            << "Channels out of phase at sample " << i;
    }
}

TEST_F(OscillatorTest, SawtoothDiscontinuityAtReset) {
    osc.setWaveform(Waveform::Saw);
    osc.setFrequency(1000.0f);
    osc.reset();
    
    allocateBuffers(2, 100);
    osc.processBlock(output.data(), 100);
    
    // Sawtooth should have discontinuities at phase wrap (expected)
    // But these should be exactly from peak to valley
    int largeJumps = 0;
    for (size_t i = 1; i < 100; ++i) {
        float diff = output[0][i] - output[0][i-1];
        if (std::abs(diff) > 1.5f) { // Large jump detected
            largeJumps++;
            // Jump should be approximately -2.0 (from +1 to -1)
            EXPECT_NEAR(diff, -2.0f, 0.1f) 
                << "Unexpected discontinuity magnitude at sample " << i;
        }
    }
    
    // Should have some discontinuities (phase wraps)
    EXPECT_GT(largeJumps, 0) << "No phase wraps detected in sawtooth";
}

TEST_F(OscillatorTest, FrequencyTransitionShouldNotCreateClicks) {
    // FAIL TEST: Detects high-frequency artifacts (clicks) from abrupt freq changes
    osc.setWaveform(Waveform::Sine);
    osc.setFrequency(440.0f);
    osc.reset();
    
    std::vector<float> samples(2048);
    
    // Generate 1024 samples at 440Hz (stable)
    for (int i = 0; i < 1024; ++i) {
        samples[i] = osc.processSample(0);
    }
    
    // Calculate high-frequency energy (second derivative) in stable region
    float stableHighFreqEnergy = 0.0f;
    for (int i = 900; i < 1020; ++i) {
        float secondDiff = (samples[i+1] - samples[i]) - (samples[i] - samples[i-1]);
        stableHighFreqEnergy += secondDiff * secondDiff;
    }
    stableHighFreqEnergy /= 120.0f;
    
    // Now make an abrupt frequency change
    osc.setFrequency(880.0f);
    
    // Generate samples around the transition
    for (int i = 1024; i < 2048; ++i) {
        samples[i] = osc.processSample(0);
    }
    
    // Calculate high-frequency energy right at the transition
    float transitionHighFreqEnergy = 0.0f;
    for (int i = 1019; i < 1029; ++i) {
        float secondDiff = (samples[i+1] - samples[i]) - (samples[i] - samples[i-1]);
        transitionHighFreqEnergy += secondDiff * secondDiff;
    }
    transitionHighFreqEnergy /= 10.0f;
    
    float energyRatio = transitionHighFreqEnergy / (stableHighFreqEnergy + 1e-10f);
    
    // With proper smoothing, transition energy should be similar to stable
    // WITHOUT smoothing, you get a spike (the audible "click")
    EXPECT_LT(energyRatio, 1.5f)
        << "AUDIBLE BUG: Frequency change creates high-frequency artifacts (click). "
        << "Energy ratio: " << energyRatio << " (expected <1.5 with smoothing)"
        << "\nStable energy: " << stableHighFreqEnergy 
        << ", Transition energy: " << transitionHighFreqEnergy;
}

TEST_F(OscillatorTest, RapidFrequencyModulationShouldBeSmooth) {
    // FAIL TEST: Rapid freq changes without smoothing create roughness/artifacts
    osc.setWaveform(Waveform::Sine);
    osc.setFrequency(440.0f);
    osc.reset();
    
    std::vector<float> samples(1000);
    
    // Generate with rapidly changing frequency (like an LFO modulating freq)
    for (int i = 0; i < 1000; ++i) {
        float freq = 440.0f + 200.0f * std::sin(2.0f * M_PI * i / 100.0f);
        osc.setFrequency(freq);
        samples[i] = osc.processSample(0);
    }
    
    // Check for NaN/Inf (basic stability)
    for (int i = 0; i < 1000; ++i) {
        EXPECT_FALSE(std::isnan(samples[i]));
        EXPECT_FALSE(std::isinf(samples[i]));
    }
    
    // Measure "roughness" from rapid derivative changes (jerk)
    float avgJerk = 0.0f;
    for (int i = 2; i < 1000; ++i) {
        float deriv1 = samples[i] - samples[i-1];
        float deriv2 = samples[i-1] - samples[i-2];
        float jerk = std::abs(deriv1 - deriv2);
        avgJerk += jerk;
    }
    avgJerk /= 998.0f;
    
    // For comparison, generate stable output
    osc.setFrequency(440.0f);
    osc.reset();
    std::vector<float> stableSamples(1000);
    for (int i = 0; i < 1000; ++i) {
        stableSamples[i] = osc.processSample(0);
    }
    
    float stableJerk = 0.0f;
    for (int i = 2; i < 1000; ++i) {
        float deriv1 = stableSamples[i] - stableSamples[i-1];
        float deriv2 = stableSamples[i-1] - stableSamples[i-2];
        float jerk = std::abs(deriv1 - deriv2);
        stableJerk += jerk;
    }
    stableJerk /= 998.0f;
    
    float jerkRatio = avgJerk / stableJerk;
    
    // With proper smoothing, jerk should be similar to stable
    // WITHOUT smoothing, we get excessive roughness/artifacts
    EXPECT_LT(jerkRatio, 1.15f)
        << "AUDIBLE BUG: Rapid frequency changes create roughness/artifacts. "
        << "Jerk ratio: " << jerkRatio << " (expected <1.15 with smoothing)"
        << "\nModulated jerk: " << avgJerk << ", Stable jerk: " << stableJerk;
}

} // namespace Jonssonic
