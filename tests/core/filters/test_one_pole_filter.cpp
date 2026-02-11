// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Unit tests for the OnePoleFilter class
// SPDX-License-Identifier: MIT

#include <gtest/gtest.h>
#include <jonssonic/core/filters/one_pole_filter.h>

using namespace jnsc;

// List of onePoleFilter variants to test
typedef ::testing::Types<OnePoleFilter<float, detail::DF1OnePoleTopology<float>>> FilterVariants;

// Typed test fixture for Filter class
template <typename Variant>
class OnePoleFilterTest : public ::testing::Test {
  protected:
    static constexpr float sampleRate = 48000.0f;
    Variant onePoleFilter;

    void SetUp() override {}
    void TearDown() override {}

    void coeffcientsChanged(AudioBuffer<float> oldCoeffs) {
        // This function checks that at least one coefficient is different after a parameter change, which indicates
        // that the design is correctly updating the engine coefficients.
        bool anyChanged = false;
        for (size_t ch = 0; ch < onePoleFilter.getNumChannels(); ++ch) {
            for (size_t s = 0; s < onePoleFilter.getNumSections(); ++s) {
                const auto& coeffs = onePoleFilter.getTopology().getCoeffs();
                for (size_t i = 0; i < 3; ++i) {
                    if (coeffs[ch][s * 3 + i] != oldCoeffs[ch][s * 3 + i])
                        anyChanged = true;
                }
            }
        }
        EXPECT_TRUE(anyChanged) << "No onePoleFilter coefficients changed after parameter update!";
    }
};

// Register the test suite with the list of onePoleFilter variants
TYPED_TEST_SUITE(OnePoleFilterTest, FilterVariants);

TYPED_TEST(OnePoleFilterTest, Routing) {
    // Test that we can instantiate both series and parallel variants of the onePoleFilter with the Routing wrapper.
    Series<OnePoleFilter<float>> seriesFilter;
    Parallel<OnePoleFilter<float>> parallelFilter;
}

TYPED_TEST(OnePoleFilterTest, PrepareFilter) {
    this->onePoleFilter.prepare(2, this->sampleRate, 3);

    EXPECT_TRUE(this->onePoleFilter.isPrepared());
    EXPECT_EQ(this->onePoleFilter.getNumChannels(), 2);
    EXPECT_EQ(this->onePoleFilter.getNumSections(), 3);
    EXPECT_EQ(this->onePoleFilter.getSampleRate(), this->sampleRate);
}

TYPED_TEST(OnePoleFilterTest, SetResponse) {
    using Response = typename TypeParam::Response;
    // Prepare the onePoleFilter
    this->onePoleFilter.prepare(1, this->sampleRate);

    // Get reference to the design and topology for testing
    auto& design = this->onePoleFilter.getDesign();
    auto& topology = this->onePoleFilter.getTopology();

    // Set a lowpass response and get intial design and coefficients
    this->onePoleFilter.setResponse(Response::Lowpass);
    auto oldCoeffs = AudioBuffer<float>(topology.getCoeffs());
    auto response = design.getResponse();
    EXPECT_EQ(response, Response::Lowpass);

    // Set a highpass response and get updated design and coefficients
    this->onePoleFilter.setResponse(Response::Highpass);
    response = design.getResponse();
    EXPECT_EQ(response, Response::Highpass);

    // Verify that the engines coefficients changed
    this->coeffcientsChanged(oldCoeffs);
}

TYPED_TEST(OnePoleFilterTest, SetFrequency) {
    using Response = typename TypeParam::Response;
    // Prepare the onePoleFilter
    float fs = this->sampleRate;
    this->onePoleFilter.prepare(1, fs);

    // Get reference to the design and topology for testing
    auto& design = this->onePoleFilter.getDesign();
    auto& topology = this->onePoleFilter.getTopology();

    this->onePoleFilter.setResponse(Response::Lowpass);

    // Set a 1000 Hz frequency and get initial design and coefficients
    this->onePoleFilter.setFrequency(1000.0_hz);
    auto oldCoeffs = AudioBuffer<float>(topology.getCoeffs());
    EXPECT_NEAR(1000.0, design.getFrequency().toHertz(fs), 1e-6f);

    // Set a 2000 Hz frequency and get updated design and coefficients
    this->onePoleFilter.setFrequency(2000.0_hz);
    EXPECT_NEAR(2000.0, design.getFrequency().toHertz(fs), 1e-6f);

    // Verify that the engines coefficients changed
    this->coeffcientsChanged(oldCoeffs);
}

TYPED_TEST(OnePoleFilterTest, SetGain) {
    using Response = typename TypeParam::Response;
    // Prepare the onePoleFilter
    float fs = this->sampleRate;
    this->onePoleFilter.prepare(1, fs);

    // Get references to design and topology for testing
    auto& design = this->onePoleFilter.getDesign();
    auto& topology = this->onePoleFilter.getTopology();

    // Set a 6 dB gain and get initial design and coefficients
    this->onePoleFilter.setResponse(Response::Lowshelf);
    this->onePoleFilter.setGain(6.0_db);
    auto oldCoeffs = AudioBuffer<float>(topology.getCoeffs());
    EXPECT_NEAR(6.0, design.getGain().toDecibels(), 1e-6f);

    // Set a -3 dB gain and get updated design and coefficients
    this->onePoleFilter.setGain(-3.0_db);
    EXPECT_NEAR(-3.0, design.getGain().toDecibels(), 1e-6f);

    // Verify that the engines coefficients changed
    this->coeffcientsChanged(oldCoeffs);
}

TYPED_TEST(OnePoleFilterTest, ProcessSample) {
    using Response = typename TypeParam::Response;
    // Prepare the onePoleFilter
    this->onePoleFilter.prepare(1, this->sampleRate);
    auto& design = this->onePoleFilter.getDesign();
    this->onePoleFilter.setResponse(Response::Lowpass);

    // Process a sample and verify that the output is not NaN or Inf
    float inputSample = 0.5f;
    float outputSample = this->onePoleFilter.processSample(0, inputSample);
    EXPECT_FALSE(std::isnan(outputSample));
    EXPECT_FALSE(std::isinf(outputSample));
}

TYPED_TEST(OnePoleFilterTest, ProcessBlock) {
    using Response = typename TypeParam::Response;
    // Prepare the onePoleFilter
    size_t numChannels = 2;
    size_t numSections = 3;
    this->onePoleFilter.prepare(numChannels, numSections, this->sampleRate);
    this->onePoleFilter.setResponse(Response::Lowpass);

    // Create input and output buffers
    size_t numSamples = 4;
    AudioBuffer<float> input(numChannels, numSamples);
    AudioBuffer<float> output(numChannels, numSamples);

    // Process the block and verify that outputs are not NaN or Inf
    this->onePoleFilter.processBlock(input.readPtrs(), output.writePtrs(), numSamples);

    for (size_t ch = 0; ch < numChannels; ++ch) {
        for (size_t n = 0; n < numSamples; ++n) {
            EXPECT_FALSE(std::isnan(output[ch][n]));
            EXPECT_FALSE(std::isinf(output[ch][n]));
        }
    }
}

TYPED_TEST(OnePoleFilterTest, Reset) {
    // Prepare the onePoleFilter and set some state
    this->onePoleFilter.prepare(2, this->sampleRate, 3);

    // Process some samples to change the state (using dummy input)
    AudioBuffer<float> input(2, 4);
    AudioBuffer<float> output(2, 4);
    this->onePoleFilter.processBlock(input.readPtrs(), output.writePtrs(), 4);

    // Reset the onePoleFilter and verify that the state buffers are cleared
    this->onePoleFilter.reset();
    const auto& state = this->onePoleFilter.getTopology().getState();
    size_t numStates = this->onePoleFilter.getTopology().STATE_VARS_PER_SECTION;
    for (size_t ch = 0; ch < this->onePoleFilter.getNumChannels(); ++ch) {
        for (size_t s = 0; s < this->onePoleFilter.getNumSections(); ++s) {
            for (size_t i = 0; i < numStates; ++i)
                EXPECT_EQ(state[ch][s * numStates + i], 0.0f); // x1
        }
    }
}

TYPED_TEST(OnePoleFilterTest, ChannelSectionProxy) {
    // Prepare the onePoleFilter
    float fs = this->sampleRate;
    this->onePoleFilter.prepare(2, fs, 3);

    // Get references to design and topology for testing
    auto& design = this->onePoleFilter.getDesign();
    auto& topology = this->onePoleFilter.getTopology();

    // Set frequency with channel-section proxy.
    auto oldCoeffs = AudioBuffer<float>(topology.getCoeffs());
    this->onePoleFilter.channel(0).section(1).setFrequency(2000.0_hz);

    // Verify the design parameters updated correctly.
    EXPECT_NEAR(2000.0, design.getFrequency().toHertz(fs), 1e-6f);

    // Verify that only the coefficients for channel 0, section 1 were updated.
    for (size_t ch = 0; ch < this->onePoleFilter.getNumChannels(); ++ch) {
        for (size_t s = 0; s < this->onePoleFilter.getNumSections(); ++s) {
            if (ch == 0 && s == 1) {
                // This section should have updated coefficients
                EXPECT_NE(topology.getCoeffs()[ch][s * 3 + 0], oldCoeffs[ch][s * 3 + 0]);
            } else {
                // Other sections should not have changed
                EXPECT_EQ(topology.getCoeffs()[ch][s * 3 + 0], oldCoeffs[ch][s * 3 + 0]);
            }
        }
    }
}

TYPED_TEST(OnePoleFilterTest, SectionProxy) {
    // Prepare the onePoleFilter
    float fs = this->sampleRate;
    this->onePoleFilter.prepare(2, fs, 3);

    // Get references to design and topology for testing
    auto& design = this->onePoleFilter.getDesign();
    auto& topology = this->onePoleFilter.getTopology();

    // Set frequency with section proxy.
    auto oldCoeffs = AudioBuffer<float>(topology.getCoeffs());
    this->onePoleFilter.section(2).setFrequency(2000.0_hz);

    // Verify the design parameters updated correctly.
    EXPECT_NEAR(2000.0, design.getFrequency().toHertz(fs), 1e-6f);

    // Verify that only the coefficients for section 2 were updated across all channels.
    for (size_t ch = 0; ch < this->onePoleFilter.getNumChannels(); ++ch) {
        for (size_t s = 0; s < this->onePoleFilter.getNumSections(); ++s) {
            if (s == 2) {
                // This section should have updated coefficients
                EXPECT_NE(topology.getCoeffs()[ch][s * 3 + 0], oldCoeffs[ch][s * 3 + 0]);
            } else {
                // Other sections should not have changed
                EXPECT_EQ(topology.getCoeffs()[ch][s * 3 + 0], oldCoeffs[ch][s * 3 + 0]);
            }
        }
    }
}

TYPED_TEST(OnePoleFilterTest, ChannelProxy) {
    // Prepare the onePoleFilter
    float fs = this->sampleRate;
    this->onePoleFilter.prepare(2, fs, 3);

    // Get references to design and topology for testing
    auto& design = this->onePoleFilter.getDesign();
    auto& topology = this->onePoleFilter.getTopology();

    // Set frequency with channel proxy.
    auto oldCoeffs = AudioBuffer<float>(topology.getCoeffs());
    this->onePoleFilter.channel(1).setFrequency(2000.0_hz);

    // Verify the design parameters updated correctly.
    EXPECT_NEAR(2000.0, design.getFrequency().toHertz(fs), 1e-6f);

    // Verify that only the coefficients for channel 1 were updated across all sections.
    for (size_t ch = 0; ch < this->onePoleFilter.getNumChannels(); ++ch) {
        for (size_t s = 0; s < this->onePoleFilter.getNumSections(); ++s) {
            if (ch == 1) {
                // This channel should have updated coefficients
                EXPECT_NE(topology.getCoeffs()[ch][s * 3 + 0], oldCoeffs[ch][s * 3 + 0]);
            } else {
                // Other channels should not have changed
                EXPECT_EQ(topology.getCoeffs()[ch][s * 3 + 0], oldCoeffs[ch][s * 3 + 0]);
            }
        }
    }
}
