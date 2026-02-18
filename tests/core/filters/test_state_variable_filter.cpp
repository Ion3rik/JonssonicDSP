// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Unit tests for the StateVariableFilter class
// SPDX-License-Identifier: MIT

#include <gtest/gtest.h>
#include <jonssonic/core/filters/state_variable_filter.h>

using namespace jnsc;

// List of stateVariableFilter variants to test
typedef ::testing::Types<StateVariableFilter<float, detail::TPTSVFTopology<float>, detail::SVFDesign<float>>>
    FilterVariants;

// Typed test fixture for Filter class
template <typename Variant>
class StateVariableFilterTest : public ::testing::Test {
  protected:
    static constexpr float sampleRate = 48000.0f;
    Variant StateVariableFilter;

    void SetUp() override {}
    void TearDown() override {}
};

// Register the test suite with the list of StateVariableFilter variants
TYPED_TEST_SUITE(StateVariableFilterTest, FilterVariants);

TYPED_TEST(StateVariableFilterTest, Routing) {
    // Test that we can instantiate both series and parallel variants of the onePoleFilter with the Routing wrapper.
    Series<StateVariableFilter<float>> seriesFilter;
    Parallel<StateVariableFilter<float>> parallelFilter;
}

TYPED_TEST(StateVariableFilterTest, PrepareFilter) {
    this->StateVariableFilter.prepare(2, this->sampleRate, 3);

    EXPECT_TRUE(this->StateVariableFilter.isPrepared());
    EXPECT_EQ(this->StateVariableFilter.getNumChannels(), 2);
    EXPECT_EQ(this->StateVariableFilter.getNumSections(), 3);
    EXPECT_EQ(this->StateVariableFilter.getSampleRate(), this->sampleRate);
}

TYPED_TEST(StateVariableFilterTest, SetResponse) {
    using Response = typename TypeParam::Response;
    // Prepare the StateVariableFilter
    this->StateVariableFilter.prepare(1, this->sampleRate);

    // Get reference to the design for testing
    auto& design = this->StateVariableFilter.getDesign();

    // Set a lowpass response and get intial design
    this->StateVariableFilter.setResponse(Response::Lowpass);
    auto response = design.getResponse();
    EXPECT_EQ(response, Response::Lowpass);

    // Set a highpass response and get updated design and coefficients
    this->StateVariableFilter.setResponse(Response::Highpass);
    response = design.getResponse();
    EXPECT_EQ(response, Response::Highpass);
}

TYPED_TEST(StateVariableFilterTest, SetFrequency) {
    using Response = typename TypeParam::Response;
    // Prepare the StateVariableFilter
    float fs = this->sampleRate;
    this->StateVariableFilter.prepare(1, fs);

    // Get reference to the design and topology for testing
    auto& design = this->StateVariableFilter.getDesign();

    this->StateVariableFilter.setResponse(Response::Lowpass);

    // Set a 1000 Hz frequency and get initial design and coefficients
    this->StateVariableFilter.setFrequency(1000.0_hz);
    EXPECT_NEAR(1000.0, design.getFrequency().toHertz(fs), 1e-6f);

    // Set a 2000 Hz frequency and get updated design and coefficients
    this->StateVariableFilter.setFrequency(2000.0_hz);
    EXPECT_NEAR(2000.0, design.getFrequency().toHertz(fs), 1e-6f);
}

TYPED_TEST(StateVariableFilterTest, SetQ) {
    using Response = typename TypeParam::Response;
    // Prepare the StateVariableFilter
    float fs = this->sampleRate;
    this->StateVariableFilter.prepare(1, fs);

    // Get references to design and topology for testing
    auto& design = this->StateVariableFilter.getDesign();

    // Set a Q of 0.5 and get initial design and coefficients
    this->StateVariableFilter.setResponse(Response::Lowpass);
    this->StateVariableFilter.setQ(0.5f);
    EXPECT_NEAR(0.5f, design.getQ(), 1e-6f);

    // Set a Q of 2.0 and get updated design and coefficients
    this->StateVariableFilter.setQ(2.0f);
    EXPECT_NEAR(2.0f, design.getQ(), 1e-6f);
}

TYPED_TEST(StateVariableFilterTest, ProcessSample) {
    using Response = typename TypeParam::Response;
    // Prepare the StateVariableFilter
    this->StateVariableFilter.prepare(1, this->sampleRate);
    auto& design = this->StateVariableFilter.getDesign();
    this->StateVariableFilter.setResponse(Response::Lowpass);

    // Process a sample and verify that the output is not NaN or Inf
    float inputSample = 0.5f;
    float outputSample = this->StateVariableFilter.processSample(0, inputSample);
    EXPECT_FALSE(std::isnan(outputSample));
    EXPECT_FALSE(std::isinf(outputSample));
}

TYPED_TEST(StateVariableFilterTest, ProcessBlock) {
    using Response = typename TypeParam::Response;
    // Prepare the StateVariableFilter
    size_t numChannels = 2;
    size_t numSections = 3;
    this->StateVariableFilter.reset();
    this->StateVariableFilter.prepare(numChannels, numSections, this->sampleRate);
    this->StateVariableFilter.setResponse(Response::Lowpass);

    // Create input and output buffers
    size_t numSamples = 1024;
    AudioBuffer<float> input(numChannels, numSamples);
    for (size_t ch = 0; ch < numChannels; ++ch)
            input[ch][0] = 1.0f; // Impulse input for testing
    AudioBuffer<float> output(numChannels, numSamples);

    // Process the block and verify that outputs are not NaN or Inf
    this->StateVariableFilter.processBlock(input.readPtrs(), output.writePtrs(), numSamples);

    for (size_t ch = 0; ch < numChannels; ++ch) {
        for (size_t n = 0; n < numSamples; ++n) {
            EXPECT_FALSE(std::isnan(output[ch][n]));
            EXPECT_FALSE(std::isinf(output[ch][n]));
        }
    }
}

TYPED_TEST(StateVariableFilterTest, Reset) {
    // Prepare the StateVariableFilter and set some state
    this->StateVariableFilter.prepare(2, this->sampleRate, 3);

    // Process some samples to change the state (using dummy input)
    AudioBuffer<float> input(2, 4);
    AudioBuffer<float> output(2, 4);
    this->StateVariableFilter.processBlock(input.readPtrs(), output.writePtrs(), 4);

    // Reset the StateVariableFilter and verify that the state buffers are cleared
    this->StateVariableFilter.reset();
    const auto& topology = this->StateVariableFilter.getTopology();
    size_t numStates = topology.STATE_VARS_PER_SECTION;
    for (size_t ch = 0; ch < this->StateVariableFilter.getNumChannels(); ++ch) {
        for (size_t s = 0; s < this->StateVariableFilter.getNumSections(); ++s) {
            for (size_t i = 0; i < numStates; ++i)
                EXPECT_EQ(topology.getState(ch, s, i), 0.0f); // x1
        }
    }
}

TYPED_TEST(StateVariableFilterTest, ChannelSectionProxy) {
    // Prepare the StateVariableFilter
    float fs = this->sampleRate;
    this->StateVariableFilter.prepare(2, fs, 3);

    // Get references to design and topology for testing
    auto& design = this->StateVariableFilter.getDesign();

    // Set frequency with channel-section proxy.
    this->StateVariableFilter.channel(0).section(1).setFrequency(2000.0_hz);

    // Verify that only the coefficients for channel 0, section 1 were updated.
    for (size_t ch = 0; ch < this->StateVariableFilter.getNumChannels(); ++ch) {
        for (size_t s = 0; s < this->StateVariableFilter.getNumSections(); ++s) {
            if (ch == 0 && s == 1) {
                // This section should have updated frequency
                EXPECT_NEAR(2000.0, design.getFrequency(ch, s).toHertz(fs), 1e-6f);
            } else {
                // Other sections should not have changed
                EXPECT_NEAR(1000.0, design.getFrequency(ch, s).toHertz(fs), 1e-6f);
            }
        }
    }
}

TYPED_TEST(StateVariableFilterTest, SectionProxy) {
    // Prepare the StateVariableFilter
    float fs = this->sampleRate;
    this->StateVariableFilter.prepare(2, fs, 3);

    // Get references to design and topology for testing
    auto& design = this->StateVariableFilter.getDesign();

    // Set frequency with section proxy.
    this->StateVariableFilter.section(2).setFrequency(2000.0_hz);

    // Verify that only the frequency of section 2 were updated across all channels.
    for (size_t ch = 0; ch < this->StateVariableFilter.getNumChannels(); ++ch) {
        for (size_t s = 0; s < this->StateVariableFilter.getNumSections(); ++s) {
            if (s == 2) {
                // This section should have updated frequency
                EXPECT_NEAR(2000.0, design.getFrequency(ch, s).toHertz(fs), 1e-6f);
            } else {
                // Other sections should not have changed
                EXPECT_NEAR(1000.0, design.getFrequency(ch, s).toHertz(fs), 1e-6f);
            }
        }
    }
}

TYPED_TEST(StateVariableFilterTest, ChannelProxy) {
    // Prepare the StateVariableFilter
    float fs = this->sampleRate;
    this->StateVariableFilter.prepare(2, fs, 3);

    // Get references to design and topology for testing
    auto& design = this->StateVariableFilter.getDesign();
    auto& topology = this->StateVariableFilter.getTopology();

    // Set frequency with channel proxy.
    this->StateVariableFilter.channel(1).setFrequency(2000.0_hz);

    // Verify that only the frequency of channel 1 were updated across all sections.
    for (size_t ch = 0; ch < this->StateVariableFilter.getNumChannels(); ++ch) {
        for (size_t s = 0; s < this->StateVariableFilter.getNumSections(); ++s) {
            if (ch == 1) {
                // This channel should have updated frequency
                EXPECT_NEAR(2000.0, design.getFrequency(ch, s).toHertz(fs), 1e-6f);
            } else {
                // Other channels should not have changed
                EXPECT_NEAR(1000.0, design.getFrequency(ch, s).toHertz(fs), 1e-6f);
            }
        }
    }
}
