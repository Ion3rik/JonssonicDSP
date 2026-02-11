// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Unit tests for the biquadFilter class
// SPDX-License-Identifier: MIT

#include <gtest/gtest.h>
#include <jonssonic/core/filters/biquad_filter.h>

using namespace jnsc;

// List of biquadFilter variants to test
typedef ::testing::Types<BiquadFilter<float, detail::DF2TTopology<float>>,
                         BiquadFilter<float, detail::DF1Topology<float>>>
    FilterVariants;

// Typed test fixture for Filter class
template <typename Variant>
class BiquadFilterTest : public ::testing::Test {
  protected:
    static constexpr float sampleRate = 48000.0f;
    Variant biquadFilter;

    void SetUp() override {}
    void TearDown() override {}

    void coeffcientsChanged(AudioBuffer<float> oldCoeffs) {
        // This function checks that at least one coefficient is different after a parameter change, which indicates
        // that the design is correctly updating the engine coefficients.
        bool anyChanged = false;
        for (size_t ch = 0; ch < biquadFilter.getNumChannels(); ++ch) {
            for (size_t s = 0; s < biquadFilter.getNumSections(); ++s) {
                const auto& coeffs = biquadFilter.getTopology().getCoeffs();
                for (size_t i = 0; i < 5; ++i) {
                    if (coeffs[ch][s * 5 + i] != oldCoeffs[ch][s * 5 + i])
                        anyChanged = true;
                }
            }
        }
        EXPECT_TRUE(anyChanged) << "No biquadFilter coefficients changed after parameter update!";
    }
};

// Register the test suite with the list of biquadFilter variants
TYPED_TEST_SUITE(BiquadFilterTest, FilterVariants);

TYPED_TEST(BiquadFilterTest, PrepareFilter) {
    this->biquadFilter.prepare(2, this->sampleRate, 3);

    EXPECT_TRUE(this->biquadFilter.isPrepared());
    EXPECT_EQ(this->biquadFilter.getNumChannels(), 2);
    EXPECT_EQ(this->biquadFilter.getNumSections(), 3);
    EXPECT_EQ(this->biquadFilter.getSampleRate(), this->sampleRate);
}

TYPED_TEST(BiquadFilterTest, SetResponse) {
    using Response = typename TypeParam::Response;
    // Prepare the biquadFilter
    this->biquadFilter.prepare(1, this->sampleRate);

    // Get reference to the design and topology for testing
    auto& design = this->biquadFilter.getDesign();
    auto& topology = this->biquadFilter.getTopology();

    // Set a lowpass response and get intial design and coefficients
    this->biquadFilter.setResponse(Response::Lowpass);
    auto oldCoeffs = AudioBuffer<float>(topology.getCoeffs());
    auto response = design.getResponse();
    EXPECT_EQ(response, Response::Lowpass);

    // Set a highpass response and get updated design and coefficients
    this->biquadFilter.setResponse(Response::Highpass);
    response = design.getResponse();
    EXPECT_EQ(response, Response::Highpass);

    // Verify that the engines coefficients changed
    this->coeffcientsChanged(oldCoeffs);
}

TYPED_TEST(BiquadFilterTest, SetFrequency) {
    using Response = typename TypeParam::Response;
    // Prepare the biquadFilter
    float fs = this->sampleRate;
    this->biquadFilter.prepare(1, fs);

    // Get reference to the design and topology for testing
    auto& design = this->biquadFilter.getDesign();
    auto& topology = this->biquadFilter.getTopology();

    this->biquadFilter.setResponse(Response::Lowpass);

    // Set a 1000 Hz frequency and get initial design and coefficients
    this->biquadFilter.setFrequency(1000.0_hz);
    auto oldCoeffs = AudioBuffer<float>(topology.getCoeffs());
    EXPECT_NEAR(1000.0, design.getFrequency().toHertz(fs), 1e-6f);

    // Set a 2000 Hz frequency and get updated design and coefficients
    this->biquadFilter.setFrequency(2000.0_hz);
    EXPECT_NEAR(2000.0, design.getFrequency().toHertz(fs), 1e-6f);

    // Verify that the engines coefficients changed
    this->coeffcientsChanged(oldCoeffs);
}

TYPED_TEST(BiquadFilterTest, SetGain) {
    using Response = typename TypeParam::Response;
    // Prepare the biquadFilter
    float fs = this->sampleRate;
    this->biquadFilter.prepare(1, fs);

    // Get references to design and topology for testing
    auto& design = this->biquadFilter.getDesign();
    auto& topology = this->biquadFilter.getTopology();

    // Set a 6 dB gain and get initial design and coefficients
    this->biquadFilter.setResponse(Response::Peak);
    this->biquadFilter.setGain(6.0_db);
    auto oldCoeffs = AudioBuffer<float>(topology.getCoeffs());
    EXPECT_NEAR(6.0, design.getGain().toDecibels(), 1e-6f);

    // Set a -3 dB gain and get updated design and coefficients
    this->biquadFilter.setGain(-3.0_db);
    EXPECT_NEAR(-3.0, design.getGain().toDecibels(), 1e-6f);

    // Verify that the engines coefficients changed
    this->coeffcientsChanged(oldCoeffs);
}

TYPED_TEST(BiquadFilterTest, SetQ) {
    using Response = typename TypeParam::Response;
    // Prepare the biquadFilter
    float fs = this->sampleRate;
    this->biquadFilter.prepare(1, fs);

    // Get references to design and topology for testing
    auto& design = this->biquadFilter.getDesign();
    auto& topology = this->biquadFilter.getTopology();

    // Set a Q of 0.5 and get initial design and coefficients
    this->biquadFilter.setResponse(Response::Lowpass);
    this->biquadFilter.setQ(0.5f);
    auto oldCoeffs = AudioBuffer<float>(topology.getCoeffs());
    EXPECT_NEAR(0.5f, design.getQ(), 1e-6f);

    // Set a Q of 2.0 and get updated design and coefficients
    this->biquadFilter.setQ(2.0f);
    EXPECT_NEAR(2.0f, design.getQ(), 1e-6f);

    // Verify that the engines coefficients changed
    this->coeffcientsChanged(oldCoeffs);
}

TYPED_TEST(BiquadFilterTest, ProcessSample) {
    using Response = typename TypeParam::Response;
    // Prepare the biquadFilter
    this->biquadFilter.prepare(1, this->sampleRate);
    auto& design = this->biquadFilter.getDesign();
    this->biquadFilter.setResponse(Response::Lowpass);

    // Process a sample and verify that the output is not NaN or Inf
    float inputSample = 0.5f;
    float outputSample = this->biquadFilter.processSample(0, inputSample);
    EXPECT_FALSE(std::isnan(outputSample));
    EXPECT_FALSE(std::isinf(outputSample));
}

TYPED_TEST(BiquadFilterTest, ProcessBlock) {
    using Response = typename TypeParam::Response;
    // Prepare the biquadFilter
    size_t numChannels = 2;
    size_t numSections = 3;
    this->biquadFilter.prepare(numChannels, numSections, this->sampleRate);
    this->biquadFilter.setResponse(Response::Lowpass);

    // Create input and output buffers
    size_t numSamples = 4;
    AudioBuffer<float> input(numChannels, numSamples);
    AudioBuffer<float> output(numChannels, numSamples);

    // Process the block and verify that outputs are not NaN or Inf
    this->biquadFilter.processBlock(input.readPtrs(), output.writePtrs(), numSamples);

    for (size_t ch = 0; ch < numChannels; ++ch) {
        for (size_t n = 0; n < numSamples; ++n) {
            EXPECT_FALSE(std::isnan(output[ch][n]));
            EXPECT_FALSE(std::isinf(output[ch][n]));
        }
    }
}

TYPED_TEST(BiquadFilterTest, Reset) {
    // Prepare the biquadFilter and set some state
    this->biquadFilter.prepare(2, this->sampleRate, 3);

    // Process some samples to change the state (using dummy input)
    AudioBuffer<float> input(2, 4);
    AudioBuffer<float> output(2, 4);
    this->biquadFilter.processBlock(input.readPtrs(), output.writePtrs(), 4);

    // Reset the biquadFilter and verify that the state buffers are cleared
    this->biquadFilter.reset();
    const auto& state = this->biquadFilter.getTopology().getState();
    size_t numStates = this->biquadFilter.getTopology().STATE_VARS_PER_SECTION;
    for (size_t ch = 0; ch < this->biquadFilter.getNumChannels(); ++ch) {
        for (size_t s = 0; s < this->biquadFilter.getNumSections(); ++s) {
            for (size_t i = 0; i < numStates; ++i)
                EXPECT_EQ(state[ch][s * numStates + i], 0.0f); // x1
        }
    }
}

TYPED_TEST(BiquadFilterTest, ChannelSectionProxy) {
    // Prepare the biquadFilter
    float fs = this->sampleRate;
    this->biquadFilter.prepare(2, fs, 3);

    // Get references to design and topology for testing
    auto& design = this->biquadFilter.getDesign();
    auto& topology = this->biquadFilter.getTopology();

    // Set frequency with channel-section proxy.
    auto oldCoeffs = AudioBuffer<float>(topology.getCoeffs());
    this->biquadFilter.channel(0).section(1).setFrequency(2000.0_hz);

    // Verify the design parameters updated correctly.
    EXPECT_NEAR(2000.0, design.getFrequency().toHertz(fs), 1e-6f);

    // Verify that only the coefficients for channel 0, section 1 were updated.
    for (size_t ch = 0; ch < this->biquadFilter.getNumChannels(); ++ch) {
        for (size_t s = 0; s < this->biquadFilter.getNumSections(); ++s) {
            if (ch == 0 && s == 1) {
                // This section should have updated coefficients
                EXPECT_NE(topology.getCoeffs()[ch][s * 5 + 0], oldCoeffs[ch][s * 5 + 0]);
            } else {
                // Other sections should not have changed
                EXPECT_EQ(topology.getCoeffs()[ch][s * 5 + 0], oldCoeffs[ch][s * 5 + 0]);
            }
        }
    }
}

TYPED_TEST(BiquadFilterTest, SectionProxy) {
    // Prepare the biquadFilter
    float fs = this->sampleRate;
    this->biquadFilter.prepare(2, fs, 3);

    // Get references to design and topology for testing
    auto& design = this->biquadFilter.getDesign();
    auto& topology = this->biquadFilter.getTopology();

    // Set frequency with section proxy.
    auto oldCoeffs = AudioBuffer<float>(topology.getCoeffs());
    this->biquadFilter.section(2).setFrequency(2000.0_hz);

    // Verify the design parameters updated correctly.
    EXPECT_NEAR(2000.0, design.getFrequency().toHertz(fs), 1e-6f);

    // Verify that only the coefficients for section 2 were updated across all channels.
    for (size_t ch = 0; ch < this->biquadFilter.getNumChannels(); ++ch) {
        for (size_t s = 0; s < this->biquadFilter.getNumSections(); ++s) {
            if (s == 2) {
                // This section should have updated coefficients
                EXPECT_NE(topology.getCoeffs()[ch][s * 5 + 0], oldCoeffs[ch][s * 5 + 0]);
            } else {
                // Other sections should not have changed
                EXPECT_EQ(topology.getCoeffs()[ch][s * 5 + 0], oldCoeffs[ch][s * 5 + 0]);
            }
        }
    }
}

TYPED_TEST(BiquadFilterTest, ChannelProxy) {
    // Prepare the biquadFilter
    float fs = this->sampleRate;
    this->biquadFilter.prepare(2, fs, 3);

    // Get references to design and topology for testing
    auto& design = this->biquadFilter.getDesign();
    auto& topology = this->biquadFilter.getTopology();

    // Set frequency with channel proxy.
    auto oldCoeffs = AudioBuffer<float>(topology.getCoeffs());
    this->biquadFilter.channel(1).setFrequency(2000.0_hz);

    // Verify the design parameters updated correctly.
    EXPECT_NEAR(2000.0, design.getFrequency().toHertz(fs), 1e-6f);

    // Verify that only the coefficients for channel 1 were updated across all sections.
    for (size_t ch = 0; ch < this->biquadFilter.getNumChannels(); ++ch) {
        for (size_t s = 0; s < this->biquadFilter.getNumSections(); ++s) {
            if (ch == 1) {
                // This channel should have updated coefficients
                EXPECT_NE(topology.getCoeffs()[ch][s * 5 + 0], oldCoeffs[ch][s * 5 + 0]);
            } else {
                // Other channels should not have changed
                EXPECT_EQ(topology.getCoeffs()[ch][s * 5 + 0], oldCoeffs[ch][s * 5 + 0]);
            }
        }
    }
}
