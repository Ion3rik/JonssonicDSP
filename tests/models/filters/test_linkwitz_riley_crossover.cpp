// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Unit tests for the LinkwitzRileyCrossover class
// SPDX-License-Identifier: MIT

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <gtest/gtest.h>
#include <jonssonic/models/filters/_filters.h>
#include <jonssonic/utils/math_utils.h>

using namespace jnsc;
using namespace jnsc::models;

template <typename T>
class LinkwitzRileyCrossoverTest : public ::testing::Test {
  protected:
    static constexpr T sampleRate = T(48000);
    static constexpr T cutoff = T(1200);
    static constexpr size_t numSamples = 2048;
    LinkwitzRileyCrossover<T> crossover;

    void SetUp() override {
        crossover.prepare(2, sampleRate);
        crossover.setFrequency(Frequency<T>::Hertz(cutoff));
    }

    std::array<std::complex<double>, 2> response(double frequency) {
        crossover.reset();
        std::array<std::complex<double>, 2> result{};
        for (size_t n = 0; n < numSamples; ++n) {
            const auto bands = crossover.processSample(0, n == 0 ? T(1) : T(0));
            const auto phase = std::polar(1.0, -utils::two_pi<double> * frequency * n / double(sampleRate));
            result[0] += double(bands[0]) * phase;
            result[1] += double(bands[1]) * phase;
        }
        return result;
    }
};

using SampleTypes = ::testing::Types<float, double>;
TYPED_TEST_SUITE(LinkwitzRileyCrossoverTest, SampleTypes);

TYPED_TEST(LinkwitzRileyCrossoverTest, PrepareAndConstructor) {
    EXPECT_TRUE(this->crossover.isPrepared());
    EXPECT_EQ(this->crossover.getNumChannels(), 2u);
    EXPECT_EQ(this->crossover.getSampleRate(), this->sampleRate);
    EXPECT_NEAR(this->crossover.getFrequency().toHertz(this->sampleRate), this->cutoff, 0.001);
    LinkwitzRileyCrossover<TypeParam> constructed(1, this->sampleRate);
    EXPECT_TRUE(constructed.isPrepared());
    EXPECT_EQ(constructed.getNumChannels(), 1u);
    EXPECT_NEAR(constructed.getFrequency().toHertz(this->sampleRate), 1000.0, 0.001);
}

TYPED_TEST(LinkwitzRileyCrossoverTest, BranchResponsesMatchLR2) {
    for (double frequency : {0.0, 50.0, 500.0, 1200.0, 3000.0, 12000.0, 24000.0}) {
        SCOPED_TRACE(frequency);
        const auto bands = this->response(frequency);
        const double ratio = std::tan(utils::pi<double> * frequency / double(this->sampleRate)) /
                             std::tan(utils::pi<double> * double(this->cutoff) / double(this->sampleRate));
        const double lowMagnitude = 1.0 / (1.0 + ratio * ratio);
        EXPECT_NEAR(std::abs(bands[0]), lowMagnitude, 0.0003);
        EXPECT_NEAR(std::abs(bands[1]), 1.0 - lowMagnitude, 0.0003);
    }
}

TYPED_TEST(LinkwitzRileyCrossoverTest, CutoffIsMinusSixDbAndBranchesAreInPhase) {
    const auto bands = this->response(double(this->cutoff));
    EXPECT_NEAR(std::abs(bands[0]), 0.5, 0.0003);
    EXPECT_NEAR(std::abs(bands[1]), 0.5, 0.0003);
    EXPECT_NEAR(std::abs(bands[0] - bands[1]), 0.0, 0.0003);
    EXPECT_NEAR((bands[0] + bands[1]).real(), 0.0, 0.0003);
    EXPECT_NEAR((bands[0] + bands[1]).imag(), -1.0, 0.0003);
}

TYPED_TEST(LinkwitzRileyCrossoverTest, DirectSumHasFlatMagnitude) {
    for (double frequency : {0.0, 50.0, 200.0, 600.0, 1200.0, 2400.0, 8000.0, 20000.0, 24000.0}) {
        SCOPED_TRACE(frequency);
        const auto bands = this->response(frequency);
        EXPECT_NEAR(std::abs(bands[0] + bands[1]), 1.0, 0.0003);
    }
}

TYPED_TEST(LinkwitzRileyCrossoverTest, ChangingFrequencyMovesTheSplit) {
    this->crossover.setFrequency(Frequency<TypeParam>::Hertz(TypeParam(400)));
    const auto lower = this->response(1500.0);
    this->crossover.setFrequency(Frequency<TypeParam>::Hertz(TypeParam(3000)));
    const auto higher = this->response(1500.0);
    EXPECT_GT(std::abs(higher[0]), std::abs(lower[0]));
    EXPECT_LT(std::abs(higher[1]), std::abs(lower[1]));
    EXPECT_NEAR(std::abs(lower[0] + lower[1]), 1.0, 0.0003);
    EXPECT_NEAR(std::abs(higher[0] + higher[1]), 1.0, 0.0003);
}

TYPED_TEST(LinkwitzRileyCrossoverTest, ChannelsAreIndependent) {
    for (size_t n = 0; n < this->numSamples; ++n) {
        this->crossover.processSample(0, n == 0 ? TypeParam(1) : TypeParam(0));
        const auto silent = this->crossover.processSample(1, TypeParam(0));
        EXPECT_EQ(silent[0], TypeParam(0));
        EXPECT_EQ(silent[1], TypeParam(0));
    }
}

TYPED_TEST(LinkwitzRileyCrossoverTest, ResetReproducesImpulseResponse) {
    std::array<std::array<TypeParam, 2>, 128> expected{};
    for (size_t n = 0; n < expected.size(); ++n)
        expected[n] = this->crossover.processSample(0, n == 0 ? TypeParam(1) : TypeParam(0));
    this->crossover.reset();
    EXPECT_NEAR(this->crossover.getFrequency().toHertz(this->sampleRate), this->cutoff, 0.001);
    for (size_t n = 0; n < expected.size(); ++n) {
        const auto actual = this->crossover.processSample(0, n == 0 ? TypeParam(1) : TypeParam(0));
        EXPECT_EQ(actual[0], expected[n][0]);
        EXPECT_EQ(actual[1], expected[n][1]);
    }
}

TYPED_TEST(LinkwitzRileyCrossoverTest, ReprepareClearsBothBranches) {
    for (size_t n = 0; n < 128; ++n)
        this->crossover.processSample(0, TypeParam(1));
    this->crossover.prepare(2, this->sampleRate);
    for (size_t ch = 0; ch < 2; ++ch) {
        const auto bands = this->crossover.processSample(ch, TypeParam(0));
        EXPECT_EQ(bands[0], TypeParam(0));
        EXPECT_EQ(bands[1], TypeParam(0));
    }
    this->crossover.prepare(1, TypeParam(96000));
    EXPECT_EQ(this->crossover.getNumChannels(), 1u);
    EXPECT_EQ(this->crossover.getSampleRate(), TypeParam(96000));
}

TYPED_TEST(LinkwitzRileyCrossoverTest, ClampsConfigurationAndFrequency) {
    this->crossover.prepare(0, TypeParam(0));
    EXPECT_EQ(this->crossover.getNumChannels(), 1u);
    EXPECT_EQ(this->crossover.getSampleRate(), TypeParam(JONSSONIC_MIN_SAMPLE_RATE));
    for (TypeParam frequency : {TypeParam(-1), TypeParam(1e9)}) {
        this->crossover.setFrequency(Frequency<TypeParam>::Hertz(frequency));
        const auto normalized = this->crossover.getFrequency().toNormalized(this->crossover.getSampleRate());
        EXPECT_NEAR(normalized, frequency < TypeParam(0) ? 0.0001 : 0.45, 0.000001);
        this->crossover.reset();
        for (size_t n = 0; n < this->numSamples; ++n) {
            const auto bands = this->crossover.processSample(0, n == 0 ? TypeParam(1) : TypeParam(0));
            EXPECT_TRUE(std::isfinite(bands[0]));
            EXPECT_TRUE(std::isfinite(bands[1]));
        }
    }
}

TYPED_TEST(LinkwitzRileyCrossoverTest, BlockMatchesSampleProcessingAndSupportsInputAliasing) {
    constexpr size_t count = 128;
    std::array<std::array<TypeParam, count>, 2> input{}, expectedLow{}, expectedHigh{}, low{}, high{};
    for (size_t ch = 0; ch < 2; ++ch)
        for (size_t n = 0; n < count; ++n) {
            input[ch][n] = TypeParam(std::sin(double(n) * 0.17 + double(ch)));
            const auto bands = this->crossover.processSample(ch, input[ch][n]);
            expectedLow[ch][n] = bands[0];
            expectedHigh[ch][n] = bands[1];
        }
    for (int alias = 0; alias < 3; ++alias) {
        SCOPED_TRACE(alias);
        this->crossover.reset();
        auto aliasedInput = input;
        const TypeParam* sources[] = {aliasedInput[0].data(), aliasedInput[1].data()};
        TypeParam* lows[] = {low[0].data(), low[1].data()};
        TypeParam* highs[] = {high[0].data(), high[1].data()};
        if (alias == 1) {
            lows[0] = aliasedInput[0].data();
            lows[1] = aliasedInput[1].data();
        } else if (alias == 2) {
            highs[0] = aliasedInput[0].data();
            highs[1] = aliasedInput[1].data();
        }
        this->crossover.processBlock(sources, lows, highs, count);
        for (size_t ch = 0; ch < 2; ++ch)
            for (size_t n = 0; n < count; ++n) {
                EXPECT_EQ(lows[ch][n], expectedLow[ch][n]);
                EXPECT_EQ(highs[ch][n], expectedHigh[ch][n]);
            }
    }
}

TYPED_TEST(LinkwitzRileyCrossoverTest, BlockPartitionDoesNotChangeTheOutputs) {
    constexpr size_t count = 128;
    std::array<TypeParam, count> input{}, wholeLow{}, wholeHigh{}, splitLow{}, splitHigh{};
    for (size_t n = 0; n < count; ++n)
        input[n] = TypeParam(std::sin(double(n) * 0.31));
    const TypeParam* source[] = {input.data()};
    TypeParam* lows[] = {wholeLow.data()};
    TypeParam* highs[] = {wholeHigh.data()};
    this->crossover.prepare(1, this->sampleRate);
    this->crossover.processBlock(source, lows, highs, count);
    this->crossover.reset();
    for (size_t offset = 0; offset < count;) {
        const size_t size = std::min(size_t(17), count - offset);
        const TypeParam* chunk[] = {input.data() + offset};
        TypeParam* chunkLow[] = {splitLow.data() + offset};
        TypeParam* chunkHigh[] = {splitHigh.data() + offset};
        this->crossover.processBlock(chunk, chunkLow, chunkHigh, size);
        offset += size;
    }
    EXPECT_EQ(wholeLow, splitLow);
    EXPECT_EQ(wholeHigh, splitHigh);
}

TYPED_TEST(LinkwitzRileyCrossoverTest, EmptyBlockDoesNotChangeState) {
    this->crossover.processBlock(nullptr, nullptr, nullptr, 0);
    const auto actual = this->crossover.processSample(0, TypeParam(1));
    this->crossover.reset();
    const auto expected = this->crossover.processSample(0, TypeParam(1));
    EXPECT_EQ(actual, expected);
}
