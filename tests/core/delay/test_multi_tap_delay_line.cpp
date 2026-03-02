#include <array>
#include <gtest/gtest.h>
#include <jonssonic/core/delays/multi_tap_delay_line.h>

using namespace jnsc;

TEST(MultiTapDelayLineTest, BasicMonoProcessingWithModulation) {
    constexpr size_t NumTaps = 2;
    constexpr size_t NumChannels = 1;
    constexpr size_t NumSamples = 8;
    constexpr float sampleRate = 48000.0f;
    MultiTapDelayLine<float, NumTaps> delayLine;

    // Prepare with 1 channel, 48kHz, max delay 10ms
    delayLine.prepare(NumChannels, sampleRate, Time<float>::Milliseconds(10));

    // Set tap gains and delays (per channel, per tap, skip smoothing)
    for (size_t tap = 0; tap < NumTaps; ++tap) {
        delayLine.setTapGain(0, tap, 1.0_lin, true);                        // unity gain for all taps
        delayLine.setTapDelay(0, tap, Time<float>::Samples(tap * 2), true); // tap 0 = 0 samples, tap 1 = 2 samples
    }

    // Check the gains and delays are set correctly and instantaneous (no smoothing)
    for (size_t tap = 0; tap < NumTaps; ++tap) {
        EXPECT_FLOAT_EQ(delayLine.getCurrentTapGain(0, tap).toLinear(), 1.0f) << "Tap " << tap << " gain should be 1.0";
        EXPECT_FLOAT_EQ(delayLine.getCurrentTapDelay(0, tap).toSamples(sampleRate), tap * 2)
            << "Tap " << tap << " delay should be " << tap * 2 << " samples";
    }

    // Input: impulse
    float input[NumSamples] = {1.0f, 0, 0, 0, 0, 0, 0, 0};
    float output[NumSamples] = {0};
    std::array<float, NumTaps> modulation[NumSamples];

    // No modulation for first 4 samples, then modulate second tap by +1 sample
    for (size_t n = 0; n < NumSamples; ++n) {
        modulation[n][0] = 0.0f;
        modulation[n][1] = (n < 4) ? 0.0f : 1.0f;
    }

    // Process block
    for (size_t n = 0; n < NumSamples; ++n) {
        output[n] = delayLine.processSample(0, input[n], modulation[n]);
    }

    // Check output: impulse at tap 0 (delay 0), impulse at tap 1 (delay 2, then 3 samples)
    EXPECT_FLOAT_EQ(output[0], 1.0f); // tap 0, delay 0
    EXPECT_FLOAT_EQ(output[1], 0.0f);
    EXPECT_FLOAT_EQ(output[2], 1.0f); // tap 1, delay 2
    EXPECT_FLOAT_EQ(output[3], 0.0f);
    EXPECT_FLOAT_EQ(output[4], 0.0f); // tap 1, now delay 3 (modulated)
    EXPECT_FLOAT_EQ(output[5], 0.0f);
    EXPECT_FLOAT_EQ(output[6], 0.0f);
    EXPECT_FLOAT_EQ(output[7], 0.0f);
}