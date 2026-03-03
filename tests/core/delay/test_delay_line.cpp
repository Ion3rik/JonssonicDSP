// Jonssonic - A C++ audio DSP library
// Unit tests for the DelayLine class
// Author: Jon Fagerström
// Update: 18.11.2025

#include <cmath>
#include <gtest/gtest.h>
#include <jonssonic/core/common/audio_buffer.h>
#include <jonssonic/core/common/quantities.h>
#include <jonssonic/core/delays/delay_line.h>
#include <jonssonic/utils/math_utils.h>

using namespace jnsc;

class DelayLineTest : public ::testing::Test {
  protected:
    float sampleRate = 44100;

    void SetUp() override {
        dlNearest.reset();
        dlLinear.reset();
        dlLagrange.reset();
    }

    void TearDown() override {
        dlNearest.reset();
        dlLinear.reset();
        dlLagrange.reset();
    }

    // Helper: Process and verify output for given input and expected values
    template <typename DelayLineType>
    void processAndVerify(DelayLineType& dl,
                          const float* const* input,
                          float* const* output,
                          const float* const* expected,
                          size_t numSamples) {
        size_t numChannels = dl.getNumChannels();
        dl.processBlock(input, output, numSamples);
        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t i = 0; i < numSamples; ++i) {
                EXPECT_FLOAT_EQ(output[ch][i], expected[ch][i]) << "Channel " << ch << " Sample " << i;
            }
        }
    }

    // Helper: Process with modulation and verify
    template <typename DelayLineType>
    void processModulatedAndVerify(DelayLineType& dl,
                                   const float* const* input,
                                   float* const* output,
                                   const float* const* modulation,
                                   const float* const* expected,
                                   size_t numSamples) {
        size_t numChannels = dl.getNumChannels();
        dl.processBlock(input, output, modulation, numSamples);

        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t i = 0; i < numSamples; ++i) {
                EXPECT_FLOAT_EQ(output[ch][i], expected[ch][i]) << "Channel " << ch << " Sample " << i;
            }
        }
    }

    // Helper: Initialize input, output, expected, and modulation buffers.
    void initBuffers(size_t numChannels, size_t numSamples) {
        input.resize(numChannels, numSamples);
        output.resize(numChannels, numSamples);
        expected.resize(numChannels, numSamples);
        modulation.resize(numChannels, numSamples);
    }

    // DelayLine instance for testing
    NearestDelayLine<float> dlNearest;
    LinearDelayLine<float> dlLinear;
    LagrangeDelayLine<float> dlLagrange;

    // Buffers for testing (Note: Now we are dependent on the AudioBuffer class which is tested separately, but this
    // allows us to test the DelayLine in a more realistic way)
    AudioBuffer<float> input, output, expected, modulation;
};

// Test: Different delays per channel with no modulation (fixed delay)
TEST_F(DelayLineTest, SetPerChannelDelay) {
    // Prepare delay line
    size_t numChannels = 2;
    size_t numSamples = 5;
    dlNearest.prepare(numChannels, sampleRate, 4.0_samples);

    // Set different delays for each channel (in samples)
    dlNearest.setDelay(0u, 2.0_samples, true); // left
    dlNearest.setDelay(1u, 4.0_samples, true); // right

    // Verify the delays are set correctly
    EXPECT_FLOAT_EQ(dlNearest.getCurrentDelay(0u).toSamples(sampleRate), 2.0f)
        << "Left channel delay should be 2 samples";
    EXPECT_FLOAT_EQ(dlNearest.getCurrentDelay(1u).toSamples(sampleRate), 4.0f)
        << "Right channel delay should be 4 samples";

    // Initialize buffers
    initBuffers(numChannels, numSamples);

    // Make an impulse in the input for both channels
    for (size_t ch = 0; ch < numChannels; ++ch) {
        input[ch][0] = 1.0f;
    }

    // Expected Output
    expected[0][2] = 1.0f; // left channel should have impulse at sample 2
    expected[1][4] = 1.0f; // right channel should have impulse at sample 4

    // Process and verify
    processAndVerify(dlNearest, input.readPtrs(), output.writePtrs(), expected.readPtrs(), numSamples);
}

// Test: Different modulated delay times per channel
TEST_F(DelayLineTest, PerChannelModulatedDelay) {
    // Prepare delay line
    size_t numChannels = 2;
    size_t numSamples = 4;
    dlNearest.prepare(numChannels, sampleRate, 5.0_samples);

    // Set different base delays for each channel (in samples)
    dlNearest.setDelay(0u, 2.0_samples, true); // left
    dlNearest.setDelay(1u, 4.0_samples, true); // right

    // Verify the delays are set correctly
    EXPECT_FLOAT_EQ(dlNearest.getCurrentDelay(0u).toSamples(sampleRate), 2.0f)
        << "Left channel delay should be 2 samples";
    EXPECT_FLOAT_EQ(dlNearest.getCurrentDelay(1u).toSamples(sampleRate), 4.0f)
        << "Right channel delay should be 4 samples";

    // Initialize buffers
    initBuffers(numChannels, numSamples);

    // Make an impulse in the input for both channels
    for (size_t ch = 0; ch < numChannels; ++ch) {
        input[ch][0] = 1.0f;
    }

    // Modulation: For left channel, add 1 sample; for right channel, subtract 1 sample
    for (size_t ch = 0; ch < numChannels; ++ch) {
        for (size_t i = 0; i < numSamples; ++i) {
            modulation[ch][i] = (ch == 0) ? 1.0f : -1.0f; // left +1 sample, right -1 sample
        }
    }

    // Expected Output
    expected[0][3] = 1.0f; // left channel should have impulse at sample 3 (2 base + 1 modulation)
    expected[1][3] = 1.0f; // right channel should have impulse at sample 3 (4 base - 1 modulation)

    // Process and verify
    processModulatedAndVerify(dlNearest,
                              input.readPtrs(),
                              output.writePtrs(),
                              modulation.readPtrs(),
                              expected.readPtrs(),
                              numSamples);
}

// Test modulated delay with fractional modulation (tests interpolation)
TEST_F(DelayLineTest, ProcessSingleSampleModulatedDelayFractionalModulation) {
    // Prepare delay line
    size_t numChannels = 1;
    size_t numSamples = 4;
    dlLinear.prepare(numChannels, sampleRate, 5.0_samples);

    // Set base delay for the channel (in samples)
    dlLinear.setDelay(0u, 2.0_samples, true); // 2 samples base delay

    // Verify the delay is set correctly
    EXPECT_FLOAT_EQ(dlLinear.getCurrentDelay(0u).toSamples(sampleRate), 2.0f) << "Channel delay should be 2 samples";

    // Initialize buffers
    initBuffers(numChannels, numSamples);

    // Make an impulse in the input
    input[0][0] = 1.0f;

    // Modulation: Add 0.5 samples to the delay
    for (size_t i = 0; i < numSamples; ++i) {
        modulation[0][i] = 0.5f; // add 0.5 sample modulation
    }

    // Expected Output: The impulse should be heard at 2.5 samples, which will be interpolated between sample 2 and 3.
    expected[0][2] = 0.5f; // half of the impulse at sample 2
    expected[0][3] = 0.5f; // half of the impulse at sample 3

    // Process and verify
    processModulatedAndVerify(dlLinear,
                              input.readPtrs(),
                              output.writePtrs(),
                              modulation.readPtrs(),
                              expected.readPtrs(),
                              numSamples);
}
// Edge case: Zero delay (immediate pass-through)
TEST_F(DelayLineTest, ZeroDelay) {
    // Prepare delay line
    size_t numChannels = 1;
    size_t numSamples = 4;
    dlNearest.prepare(numChannels, sampleRate, 5.0_samples);

    // Set zero delay for the channel
    dlNearest.setDelay(0u, 0.0_samples, true);

    // Verify the delay is set correctly
    EXPECT_FLOAT_EQ(dlNearest.getCurrentDelay(0u).toSamples(sampleRate), 0.0f) << "Channel delay should be 0 samples";

    // Initialize buffers
    initBuffers(numChannels, numSamples);

    // Make an impulse in the input
    input[0][0] = 1.0f;

    // Expected Output: The impulse should be heard immediately at sample 0.
    expected[0][0] = 1.0f;

    // Process and verify
    processAndVerify(dlNearest, input.readPtrs(), output.writePtrs(), expected.readPtrs(), numSamples);
}

// Edge case: Maximum delay (buffer size - 1)
TEST_F(DelayLineTest, MaximumDelay) {
    // Prepare delay line
    size_t numChannels = 1;
    size_t numSamples = 5;
    dlNearest.prepare(numChannels, sampleRate, 4.0_samples);

    // Set maximum delay for the channel (4 samples)
    dlNearest.setDelay(0u, 4.0_samples, true);

    // Verify the delay is set correctly
    EXPECT_FLOAT_EQ(dlNearest.getCurrentDelay(0u).toSamples(sampleRate), 4.0f) << "Channel delay should be 4 samples";

    // Initialize buffers
    initBuffers(numChannels, numSamples);

    // Make an impulse in the input
    input[0][0] = 1.0f;

    // Expected Output: The impulse should be heard at sample 4.
    expected[0][4] = 1.0f;

    // Process and verify
    processAndVerify(dlNearest, input.readPtrs(), output.writePtrs(), expected.readPtrs(), numSamples);
}

// Edge case: Modulated delay with extreme negative modulation (clamped to 0)
TEST_F(DelayLineTest, ModulatedDelayClampedToZero) {
    // Prepare delay line
    size_t numChannels = 1;
    size_t numSamples = 4;
    dlLinear.prepare(numChannels, sampleRate, 4.0_samples);
    dlLinear.setDelay(0u, 2.0_samples, true);

    // Set base delay
    dlLinear.setDelay(0u, 2.0_samples, true);

    // Initialize buffers
    initBuffers(numChannels, numSamples);

    // Make an impulse in the input
    input[0][0] = 1.0f;

    // Out of bounds negative modulation: -3 samples modulation, should clamp to 0
    for (size_t i = 0; i < numSamples; ++i) {
        modulation[0][i] = -3.0f;
    }

    // Expected Output: The impulse should be heard immediately at sample 0 (clamped to zero delay).
    expected[0][0] = 1.0f;

    // Process and verify
    processModulatedAndVerify(dlLinear,
                              input.readPtrs(),
                              output.writePtrs(),
                              modulation.readPtrs(),
                              expected.readPtrs(),
                              numSamples);
}

// Edge case: Modulated delay with extreme positive modulation (clamped to max delay)
TEST_F(DelayLineTest, ModulatedDelayClampedToMaxDelay) {
    // Prepare delay line
    size_t numChannels = 1;
    size_t numSamples = 5;
    dlLinear.prepare(numChannels, sampleRate, 4.0_samples);
    dlLinear.setDelay(0u, 2.0_samples, true);

    // Initialize buffers
    initBuffers(numChannels, numSamples);

    // Make an impulse in the input
    input[0][0] = 1.0f;

    // Out of bounds positive modulation: +3 samples modulation, should clamp to max delay (4 samples)
    for (size_t i = 0; i < numSamples; ++i) {
        modulation[0][i] = 3.0f;
    }

    // Expected Output: The impulse should be heard at sample 4 (clamped to max delay).
    expected[0][4] = 1.0f;

    // Process and verify
    processModulatedAndVerify(dlLinear,
                              input.readPtrs(),
                              output.writePtrs(),
                              modulation.readPtrs(),
                              expected.readPtrs(),
                              numSamples);
}