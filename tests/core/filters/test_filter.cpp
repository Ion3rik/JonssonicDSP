// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Unit tests for filter class
// SPDX-License-Identifier: MIT

#include <gtest/gtest.h>
#include <jonssonic/core/filters/filter.h>

using namespace jnsc;

// List of filter topologies to test
typedef ::testing::Types<BiquadDF1<float>> FilterTopologies;

// Typed test fixture for Filter class
template <typename Topology>
class FilterTest : public ::testing::Test {
  protected:
    static constexpr float sampleRate = 48000.0f;
    Filter<float, Topology> filter;

    void SetUp() override {}
    void TearDown() override {}

    void coeffcientsChanged(AudioBuffer<float> oldCoeffs) {
        // This function checks that at least one coefficient is different after a parameter change, which indicates
        // that the design is correctly updating the engine coefficients.
        bool anyChanged = false;
        for (size_t ch = 0; ch < filter.getNumChannels(); ++ch) {
            for (size_t s = 0; s < filter.getNumSections(); ++s) {
                const auto& coeffs = filter.getEngine().getCoeffs();
                for (size_t i = 0; i < 5; ++i) {
                    if (coeffs[ch][s * 5 + i] != oldCoeffs[ch][s * 5 + i])
                        anyChanged = true;
                }
            }
        }
        EXPECT_TRUE(anyChanged) << "No filter coefficients changed after parameter update!";
    }
};

// Register the test suite with the list of filter topologies
TYPED_TEST_SUITE(FilterTest, FilterTopologies);

// =======================================================================
// Test Preparation
// =======================================================================
TYPED_TEST(FilterTest, PrepareFilter) {
    this->filter.prepare(2, 3, this->sampleRate);

    EXPECT_TRUE(this->filter.isPrepared());
    EXPECT_EQ(this->filter.getNumChannels(), 2);
    EXPECT_EQ(this->filter.getNumSections(), 3);
    EXPECT_EQ(this->filter.getSampleRate(), this->sampleRate);
}

TYPED_TEST(FilterTest, SetResponse) {
    using Response = typename Filter<float, TypeParam>::Response;
    // Prepare the filter
    this->filter.prepare(1, 1, this->sampleRate);

    // Get references to design and engine for testing
    auto& design = this->filter.getDesign();
    auto& engine = this->filter.getEngine();

    // Set a lowpass response and get intial design and coefficients
    this->filter.setResponse(Response::Lowpass);
    auto oldCoeffs = AudioBuffer<float>(engine.getCoeffs());
    auto response = design.getResponse();
    EXPECT_EQ(response, Response::Lowpass);

    // Set a highpass response and get updated design and coefficients
    this->filter.setResponse(Response::Highpass);
    response = design.getResponse();
    EXPECT_EQ(response, Response::Highpass);

    // Verify that the engines coefficients changed
    this->coeffcientsChanged(oldCoeffs);
}

TYPED_TEST(FilterTest, SetFrequency) {
    using Response = typename Filter<float, TypeParam>::Response;
    // Prepare the filter
    float fs = this->sampleRate;
    this->filter.prepare(1, 1, fs);
    this->filter.setResponse(Response::Lowpass);

    // Get references to design and engine for testing
    auto& design = this->filter.getDesign();
    auto& engine = this->filter.getEngine();

    // Set a 1000 Hz frequency and get initial design and coefficients
    this->filter.setFrequency(1000.0_hz);
    auto oldCoeffs = AudioBuffer<float>(engine.getCoeffs());
    EXPECT_NEAR(1000.0, design.getFrequency().toHertz(fs), 1e-6f);

    // Set a 2000 Hz frequency and get updated design and coefficients
    this->filter.setFrequency(2000.0_hz);
    EXPECT_NEAR(2000.0, design.getFrequency().toHertz(fs), 1e-6f);

    // Verify that the engines coefficients changed
    this->coeffcientsChanged(oldCoeffs);
}

TYPED_TEST(FilterTest, SetGain) {
    using Response = typename Filter<float, TypeParam>::Response;
    // Prepare the filter
    float fs = this->sampleRate;
    this->filter.prepare(1, 1, fs);
    this->filter.setResponse(Response::Peak);

    // Get references to design and engine for testing
    auto& design = this->filter.getDesign();
    auto& engine = this->filter.getEngine();

    // Set a 6 dB gain and get initial design and coefficients
    this->filter.setGain(6.0_db);
    auto oldCoeffs = AudioBuffer<float>(engine.getCoeffs());
    EXPECT_NEAR(6.0, design.getGain().toDecibels(), 1e-6f);

    // Set a -3 dB gain and get updated design and coefficients
    this->filter.setGain(-3.0_db);
    EXPECT_NEAR(-3.0, design.getGain().toDecibels(), 1e-6f);

    // Verify that the engines coefficients changed
    this->coeffcientsChanged(oldCoeffs);
}

TYPED_TEST(FilterTest, SetQ) {
    using Response = typename Filter<float, TypeParam>::Response;
    // Prepare the filter
    float fs = this->sampleRate;
    this->filter.prepare(1, 1, fs);
    this->filter.setResponse(Response::Lowpass);

    // Get references to design and engine for testing
    auto& design = this->filter.getDesign();
    auto& engine = this->filter.getEngine();

    // Set a Q of 0.5 and get initial design and coefficients
    this->filter.setQ(0.5f);
    auto oldCoeffs = AudioBuffer<float>(engine.getCoeffs());
    EXPECT_NEAR(0.5f, design.getQ(), 1e-6f);

    // Set a Q of 2.0 and get updated design and coefficients
    this->filter.setQ(2.0f);
    EXPECT_NEAR(2.0f, design.getQ(), 1e-6f);

    // Verify that the engines coefficients changed
    this->coeffcientsChanged(oldCoeffs);
}

TYPED_TEST(FilterTest, ProcessSample) {
    using Response = typename Filter<float, TypeParam>::Response;
    // Prepare the filter
    this->filter.prepare(1, 1, this->sampleRate);
    this->filter.setResponse(Response::Lowpass);

    // Process a sample and verify that the output is not NaN or Inf
    float inputSample = 0.5f;
    float outputSample = this->filter.processSample(0, inputSample);
    EXPECT_FALSE(std::isnan(outputSample));
    EXPECT_FALSE(std::isinf(outputSample));
}

TYPED_TEST(FilterTest, ProcessBlock) {
    using Response = typename Filter<float, TypeParam>::Response;
    // Prepare the filter
    size_t numChannels = 2;
    size_t numSections = 3;
    this->filter.prepare(numChannels, numSections, this->sampleRate);
    this->filter.setResponse(Response::Lowpass);

    // Create input and output buffers
    size_t numSamples = 4;
    AudioBuffer<float> input(numChannels, numSamples);
    AudioBuffer<float> output(numChannels, numSamples);

    // Process the block and verify that outputs are not NaN or Inf
    std::vector<const float*> inputPtrs(numChannels);
    std::vector<float*> outputPtrs(numChannels);

    this->filter.processBlock(input.readPtrs(), output.writePtrs(), numSamples);

    for (size_t ch = 0; ch < numChannels; ++ch) {
        for (size_t n = 0; n < numSamples; ++n) {
            EXPECT_FALSE(std::isnan(output[ch][n]));
            EXPECT_FALSE(std::isinf(output[ch][n]));
        }
    }
}

TYPED_TEST(FilterTest, Reset) {
    // Prepare the filter and set some state
    this->filter.prepare(2, 3, this->sampleRate);

    // Process some samples to change the state (using dummy input)
    AudioBuffer<float> input(2, 4);
    AudioBuffer<float> output(2, 4);
    this->filter.processBlock(input.readPtrs(), output.writePtrs(), 4);

    // Reset the filter and verify that the state buffers are cleared
    this->filter.reset();
    const auto& state = this->filter.getEngine().getState();
    for (size_t ch = 0; ch < this->filter.getNumChannels(); ++ch) {
        for (size_t s = 0; s < this->filter.getNumSections(); ++s) {
            EXPECT_EQ(state[ch][s * 4 + 0], 0.0f); // x1
            EXPECT_EQ(state[ch][s * 4 + 1], 0.0f); // x2
            EXPECT_EQ(state[ch][s * 4 + 2], 0.0f); // y1
            EXPECT_EQ(state[ch][s * 4 + 3], 0.0f); // y2
        }
    }
}

TYPED_TEST(FilterTest, ChannelSectionProxy) {
    // Prepare the filter
    float fs = this->sampleRate;
    this->filter.prepare(2, 3, fs);

    // Get references to design and engine for testing
    auto& design = this->filter.getDesign();
    auto& engine = this->filter.getEngine();

    // Set frequency with channel-section proxy.
    auto oldCoeffs = AudioBuffer<float>(engine.getCoeffs());
    this->filter.channel(0).section(1).setFrequency(2000.0_hz);

    // Verify the design parameters updated correctly.
    EXPECT_NEAR(2000.0, design.getFrequency().toHertz(fs), 1e-6f);

    // Verify that only the coefficients for channel 0, section 1 were updated.
    for (size_t ch = 0; ch < this->filter.getNumChannels(); ++ch) {
        for (size_t s = 0; s < this->filter.getNumSections(); ++s) {
            if (ch == 0 && s == 1) {
                // This section should have updated coefficients
                EXPECT_NE(engine.getCoeffs()[ch][s * 5 + 0], oldCoeffs[ch][s * 5 + 0]);
            } else {
                // Other sections should not have changed
                EXPECT_EQ(engine.getCoeffs()[ch][s * 5 + 0], oldCoeffs[ch][s * 5 + 0]);
            }
        }
    }
}

TYPED_TEST(FilterTest, SectionProxy) {
    // Prepare the filter
    float fs = this->sampleRate;
    this->filter.prepare(2, 3, fs);

    // Get references to design and engine for testing
    auto& design = this->filter.getDesign();
    auto& engine = this->filter.getEngine();

    // Set frequency with section proxy.
    auto oldCoeffs = AudioBuffer<float>(engine.getCoeffs());
    this->filter.section(2).setFrequency(2000.0_hz);

    // Verify the design parameters updated correctly.
    EXPECT_NEAR(2000.0, design.getFrequency().toHertz(fs), 1e-6f);

    // Verify that only the coefficients for section 2 were updated across all channels.
    for (size_t ch = 0; ch < this->filter.getNumChannels(); ++ch) {
        for (size_t s = 0; s < this->filter.getNumSections(); ++s) {
            if (s == 2) {
                // This section should have updated coefficients
                EXPECT_NE(engine.getCoeffs()[ch][s * 5 + 0], oldCoeffs[ch][s * 5 + 0]);
            } else {
                // Other sections should not have changed
                EXPECT_EQ(engine.getCoeffs()[ch][s * 5 + 0], oldCoeffs[ch][s * 5 + 0]);
            }
        }
    }
}

TYPED_TEST(FilterTest, ChannelProxy) {
    // Prepare the filter
    float fs = this->sampleRate;
    this->filter.prepare(2, 3, fs);

    // Get references to design and engine for testing
    auto& design = this->filter.getDesign();
    auto& engine = this->filter.getEngine();

    // Set frequency with channel proxy.
    auto oldCoeffs = AudioBuffer<float>(engine.getCoeffs());
    this->filter.channel(1).setFrequency(2000.0_hz);

    // Verify the design parameters updated correctly.
    EXPECT_NEAR(2000.0, design.getFrequency().toHertz(fs), 1e-6f);

    // Verify that only the coefficients for channel 1 were updated across all sections.
    for (size_t ch = 0; ch < this->filter.getNumChannels(); ++ch) {
        for (size_t s = 0; s < this->filter.getNumSections(); ++s) {
            if (ch == 1) {
                // This channel should have updated coefficients
                EXPECT_NE(engine.getCoeffs()[ch][s * 5 + 0], oldCoeffs[ch][s * 5 + 0]);
            } else {
                // Other channels should not have changed
                EXPECT_EQ(engine.getCoeffs()[ch][s * 5 + 0], oldCoeffs[ch][s * 5 + 0]);
            }
        }
    }
}
