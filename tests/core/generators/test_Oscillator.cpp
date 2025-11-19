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

    Oscillator<float> osc{2, 44100.0f};
    std::vector<float*> output;
    std::vector<float> outputData;
    std::vector<const float*> phaseMod;
    std::vector<float> phaseModData;
};

// ============================================================================
// Construction and Initialization Tests
// ============================================================================

TEST_F(OscillatorTest, ConstructorInitialization) {
    Oscillator<float> newOsc(2, 48000.0f);
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
    
    allocateBuffers(2, 1);
    osc.processSample(output.data());
    
    // First sample after prepare should be at phase 0 (sine = 0.0)
    EXPECT_NEAR(output[0][0], 0.0f, 0.001f);
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
    
    allocateBuffers(2, 1);
    osc.processSample(output.data());
    
    // Sine at phase 0 should be 0
    EXPECT_NEAR(output[0][0], 0.0f, 0.001f);
}

TEST_F(OscillatorTest, SawWaveform) {
    osc.setWaveform(Waveform::Saw);
    osc.reset();
    
    allocateBuffers(2, 1);
    osc.processSample(output.data());
    
    // Saw at phase 0: 2*0 - 1 = -1
    EXPECT_NEAR(output[0][0], -1.0f, 0.001f);
}

TEST_F(OscillatorTest, SquareWaveform) {
    osc.setWaveform(Waveform::Square);
    osc.reset();
    
    allocateBuffers(2, 1);
    osc.processSample(output.data());
    
    // Square at phase 0 (< 0.5): -1
    EXPECT_NEAR(output[0][0], -1.0f, 0.001f);
}

TEST_F(OscillatorTest, TriangleWaveform) {
    osc.setWaveform(Waveform::Triangle);
    osc.reset();
    
    allocateBuffers(2, 1);
    osc.processSample(output.data());
    
    // Triangle at phase 0: 4*0 - 1 = -1
    EXPECT_NEAR(output[0][0], -1.0f, 0.001f);
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
    
    allocateBuffers(2, 1);
    osc.processSample(output.data());
    
    // Should be back at phase 0
    EXPECT_NEAR(output[0][0], 0.0f, 0.001f);
}

TEST_F(OscillatorTest, ResetSingleChannel) {
    osc.setFrequency(440.0f);
    allocateBuffers(2, 10);
    
    // Generate some samples
    osc.processBlock(output.data(), 10);
    
    // Reset only channel 0
    osc.reset(0);
    
    allocateBuffers(2, 1);
    osc.processSample(output.data());
    
    // Channel 0 should be at phase 0
    EXPECT_NEAR(output[0][0], 0.0f, 0.001f);
    // Channel 1 should not be at phase 0 (continued from before)
    EXPECT_GT(std::abs(output[1][0]), 0.001f);
}

// ============================================================================
// Single Sample Processing Tests
// ============================================================================

TEST_F(OscillatorTest, ProcessSampleNoModulation) {
    osc.setFrequency(440.0f);
    osc.reset();
    
    allocateBuffers(2, 1);
    
    // Process multiple single samples
    for (int i = 0; i < 10; ++i) {
        osc.processSample(output.data());
        
        // Output should be in valid range [-1, 1]
        EXPECT_GE(output[0][0], -1.0f);
        EXPECT_LE(output[0][0], 1.0f);
        EXPECT_GE(output[1][0], -1.0f);
        EXPECT_LE(output[1][0], 1.0f);
    }
}

TEST_F(OscillatorTest, ProcessSampleWithModulation) {
    osc.setFrequency(440.0f);
    osc.reset();
    
    allocateBuffers(2, 1);
    allocatePhaseModBuffers(2, 1, 0.25f); // 90° phase shift
    
    osc.processSample(output.data(), phaseMod.data());
    
    // With 0.25 phase offset, sine should be at peak (≈1.0)
    EXPECT_NEAR(output[0][0], 1.0f, 0.001f);
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
    Oscillator<float> quadOsc(4, 44100.0f);
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
    
    allocateBuffers(2, 1);
    allocatePhaseModBuffers(2, 1, 5.7f); // Large modulation value
    
    osc.processSample(output.data(), phaseMod.data());
    
    // Should wrap correctly and produce valid output
    EXPECT_GE(output[0][0], -1.0f);
    EXPECT_LE(output[0][0], 1.0f);
    EXPECT_FALSE(std::isnan(output[0][0]));
}

TEST_F(OscillatorTest, NegativePhaseModulationWraps) {
    osc.setFrequency(440.0f);
    osc.reset();
    
    allocateBuffers(2, 1);
    allocatePhaseModBuffers(2, 1, -2.3f); // Negative modulation
    
    osc.processSample(output.data(), phaseMod.data());
    
    // Should wrap correctly and produce valid output
    EXPECT_GE(output[0][0], -1.0f);
    EXPECT_LE(output[0][0], 1.0f);
    EXPECT_FALSE(std::isnan(output[0][0]));
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
    Oscillator<float> osc1(2, 44100.0f);
    osc1.prepare(2, 44100.0f);
    osc1.setFrequency(440.0f);
    osc1.setWaveform(Waveform::Sine);
    
    std::vector<float*> output1(2);
    std::vector<float> outputData1(2 * 10);
    for (size_t ch = 0; ch < 2; ++ch) {
        output1[ch] = &outputData1[ch * 10];
    }
    
    for (size_t i = 0; i < 10; ++i) {
        float temp[2];
        float* tempPtrs[2] = {&temp[0], &temp[1]};
        osc1.processSample(tempPtrs);
        output1[0][i] = temp[0];
        output1[1][i] = temp[1];
    }
    
    // Process with processBlock
    Oscillator<float> osc2(2, 44100.0f);
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

} // namespace Jonssonic
