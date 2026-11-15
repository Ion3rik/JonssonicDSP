// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Unit tests for quantities and unit conversions
// SPDX-License-Identifier: MIT

#include <algorithm>
#include <array>
#include <cmath>
#include <gtest/gtest.h>
#include <jonssonic/core/common/quantities.h>
#include <limits>

using namespace jnsc;

template <typename T>
class QuantitiesTest : public ::testing::Test {
  protected:
    void expectNear(T actual, T expected) {
        const T tolerance = T(16) * std::numeric_limits<T>::epsilon() * std::abs(expected);
        EXPECT_NEAR(actual, expected, tolerance);
    }
};

using QuantitySampleTypes = ::testing::Types<float, double>;
TYPED_TEST_SUITE(QuantitiesTest, QuantitySampleTypes);

TYPED_TEST(QuantitiesTest, AllTimeUnitConversions) {
    for (TypeParam sampleRate : {TypeParam(1000), TypeParam(44100), TypeParam(48000),
                                TypeParam(96000), TypeParam(192000)}) {
        SCOPED_TRACE(sampleRate);
        for (long double seconds : {0.0L, 0.001L, 0.125L, 1.5L, 10.0L}) {
            SCOPED_TRACE(seconds);
            const TypeParam expectedSeconds = TypeParam(seconds);
            const TypeParam expectedMilliseconds = TypeParam(seconds * 1000.0L);
            const TypeParam expectedSamples = TypeParam(seconds * sampleRate);
            const std::array<Time<TypeParam>, 3> sources{
                Time<TypeParam>::Seconds(expectedSeconds),
                Time<TypeParam>::Milliseconds(expectedMilliseconds),
                Time<TypeParam>::Samples(expectedSamples)};
            for (const auto& source : sources) {
                SCOPED_TRACE(static_cast<int>(source.unit));
                this->expectNear(source.toSeconds(sampleRate), expectedSeconds);
                this->expectNear(source.toMilliseconds(sampleRate), expectedMilliseconds);
                this->expectNear(source.toSamples(sampleRate), expectedSamples);
            }
        }
    }
}

TYPED_TEST(QuantitiesTest, TimeFactoriesPreserveUnitsAndClampNegativeValues) {
    EXPECT_EQ(Time<TypeParam>::Samples(TypeParam(2)).unit, TimeUnit::Samples);
    EXPECT_EQ(Time<TypeParam>::Milliseconds(TypeParam(2)).unit, TimeUnit::Milliseconds);
    EXPECT_EQ(Time<TypeParam>::Seconds(TypeParam(2)).unit, TimeUnit::Seconds);
    for (TypeParam value : {TypeParam(-100), TypeParam(-1), TypeParam(0), TypeParam(2)}) {
        const TypeParam expected = std::max(value, TypeParam(0));
        EXPECT_EQ(Time<TypeParam>::Samples(value).value, expected);
        EXPECT_EQ(Time<TypeParam>::Milliseconds(value).value, expected);
        EXPECT_EQ(Time<TypeParam>::Seconds(value).value, expected);
    }
}

TYPED_TEST(QuantitiesTest, AllFrequencyUnitConversions) {
    constexpr long double twoPi = 6.283185307179586476925286766559L;
    for (TypeParam sampleRate : {TypeParam(1000), TypeParam(44100), TypeParam(48000),
                                TypeParam(96000), TypeParam(192000)}) {
        SCOPED_TRACE(sampleRate);
        for (long double normalized : {0.0L, 0.0001L, 0.025L, 0.125L, 0.45L, 0.5L, 0.75L}) {
            SCOPED_TRACE(normalized);
            const TypeParam expectedHertz = TypeParam(normalized * sampleRate);
            const TypeParam expectedKilohertz = TypeParam(normalized * sampleRate / 1000.0L);
            const TypeParam expectedNormalized = TypeParam(normalized);
            const TypeParam expectedRadians = TypeParam(normalized * twoPi);
            const std::array<Frequency<TypeParam>, 4> sources{
                Frequency<TypeParam>::Hertz(expectedHertz),
                Frequency<TypeParam>::Kilohertz(expectedKilohertz),
                Frequency<TypeParam>::Normalized(expectedNormalized),
                Frequency<TypeParam>::Radians(expectedRadians)};
            for (const auto& source : sources) {
                SCOPED_TRACE(static_cast<int>(source.unit));
                this->expectNear(source.toHertz(sampleRate), expectedHertz);
                this->expectNear(source.toKilohertz(sampleRate), expectedKilohertz);
                this->expectNear(source.toNormalized(sampleRate), expectedNormalized);
                this->expectNear(source.toRadians(sampleRate), expectedRadians);
            }
        }
    }
}

TYPED_TEST(QuantitiesTest, RadiansToNormalizedIsIndependentOfSampleRate) {
    const auto frequency = Frequency<TypeParam>::Radians(TypeParam(0.157079632679489661923L));
    for (TypeParam sampleRate : {TypeParam(44100), TypeParam(48000), TypeParam(96000)}) {
        SCOPED_TRACE(sampleRate);
        this->expectNear(frequency.toNormalized(sampleRate), TypeParam(0.025));
    }
}

TYPED_TEST(QuantitiesTest, FrequencyFactoriesPreserveUnitsAndClampNegativeValues) {
    EXPECT_EQ(Frequency<TypeParam>::Hertz(TypeParam(2)).unit, FrequencyUnit::Hertz);
    EXPECT_EQ(Frequency<TypeParam>::Kilohertz(TypeParam(2)).unit, FrequencyUnit::Kilohertz);
    EXPECT_EQ(Frequency<TypeParam>::Normalized(TypeParam(2)).unit, FrequencyUnit::Normalized);
    EXPECT_EQ(Frequency<TypeParam>::Radians(TypeParam(2)).unit, FrequencyUnit::Radians);
    for (TypeParam value : {TypeParam(-100), TypeParam(-1), TypeParam(0), TypeParam(2)}) {
        const TypeParam expected = std::max(value, TypeParam(0));
        EXPECT_EQ(Frequency<TypeParam>::Hertz(value).value, expected);
        EXPECT_EQ(Frequency<TypeParam>::Kilohertz(value).value, expected);
        EXPECT_EQ(Frequency<TypeParam>::Normalized(value).value, expected);
        EXPECT_EQ(Frequency<TypeParam>::Radians(value).value, expected);
    }
}

TYPED_TEST(QuantitiesTest, FrequencyFollowsSampleRateOnlyForSampleBasedUnits) {
    this->expectNear(Frequency<TypeParam>::Normalized(TypeParam(0.125)).toHertz(TypeParam(48000)), TypeParam(6000));
    this->expectNear(Frequency<TypeParam>::Normalized(TypeParam(0.125)).toHertz(TypeParam(96000)), TypeParam(12000));
    this->expectNear(Frequency<TypeParam>::Hertz(TypeParam(6000)).toNormalized(TypeParam(48000)), TypeParam(0.125));
    this->expectNear(Frequency<TypeParam>::Hertz(TypeParam(6000)).toNormalized(TypeParam(96000)), TypeParam(0.0625));
}

TYPED_TEST(QuantitiesTest, GainConversionsForBoostCutAndUnity) {
    for (long double decibels : {-60.0L, -12.0L, -6.0L, 0.0L, 6.0L, 12.0L, 60.0L}) {
        SCOPED_TRACE(decibels);
        const TypeParam expectedDb = TypeParam(decibels);
        const TypeParam expectedLinear = TypeParam(std::pow(10.0L, decibels / 20.0L));
        const auto fromDb = Gain<TypeParam>::Decibels(expectedDb);
        const auto fromLinear = Gain<TypeParam>::Linear(expectedLinear);
        EXPECT_EQ(fromDb.unit, GainUnit::Decibels);
        EXPECT_EQ(fromLinear.unit, GainUnit::Linear);
        this->expectNear(fromDb.toLinear(), expectedLinear);
        EXPECT_EQ(fromDb.toDecibels(), expectedDb);
        EXPECT_EQ(fromLinear.toLinear(), expectedLinear);
        this->expectNear(fromLinear.toDecibels(), expectedDb);
    }
}

TYPED_TEST(QuantitiesTest, LinearGainPreservesPolarityAndUsesMagnitudeForDecibels) {
    for (TypeParam magnitude : {TypeParam(0.25), TypeParam(1), TypeParam(2), TypeParam(4)}) {
        const auto positive = Gain<TypeParam>::Linear(magnitude);
        const auto negative = Gain<TypeParam>::Linear(-magnitude);
        EXPECT_EQ(negative.toLinear(), -magnitude);
        EXPECT_EQ(negative.toDecibels(), positive.toDecibels());
        EXPECT_EQ((-positive).toLinear(), -magnitude);
        EXPECT_EQ((-negative).toLinear(), magnitude);
    }
}

TYPED_TEST(QuantitiesTest, ZeroAndSubEpsilonLinearGainHaveFiniteDecibels) {
    const TypeParam minimum = std::numeric_limits<TypeParam>::epsilon();
    const TypeParam expectedDb = TypeParam(20) * std::log10(minimum);
    for (TypeParam value : {TypeParam(0), minimum / TypeParam(2), -minimum / TypeParam(2)}) {
        const auto gain = Gain<TypeParam>::Linear(value);
        EXPECT_EQ(std::abs(gain.toLinear()), minimum);
        EXPECT_TRUE(std::isfinite(gain.toDecibels()));
        this->expectNear(gain.toDecibels(), expectedDb);
    }
}

TEST(QuantityLiteralsTest, TimeLiteralsPreserveUnitsAndValues) {
    const auto milliseconds = 125.0_ms;
    const auto seconds = 0.125_s;
    const auto samples = 6000.0_samples;
    EXPECT_EQ(milliseconds.unit, TimeUnit::Milliseconds);
    EXPECT_EQ(seconds.unit, TimeUnit::Seconds);
    EXPECT_EQ(samples.unit, TimeUnit::Samples);
    EXPECT_FLOAT_EQ(milliseconds.toSamples(48000.0f), 6000.0f);
    EXPECT_FLOAT_EQ(seconds.toSamples(48000.0f), 6000.0f);
    EXPECT_FLOAT_EQ(samples.toSeconds(48000.0f), 0.125f);
}

TEST(QuantityLiteralsTest, FrequencyLiteralsPreserveUnitsAndValues) {
    const auto hertz = 1200.0_hz;
    const auto kilohertz = 1.2_khz;
    const auto normalized = 0.025_norm;
    const auto radians = 0.15707963267948966_rad;
    EXPECT_EQ(hertz.unit, FrequencyUnit::Hertz);
    EXPECT_EQ(kilohertz.unit, FrequencyUnit::Kilohertz);
    EXPECT_EQ(normalized.unit, FrequencyUnit::Normalized);
    EXPECT_EQ(radians.unit, FrequencyUnit::Radians);
    EXPECT_FLOAT_EQ(hertz.toKilohertz(48000.0f), 1.2f);
    EXPECT_FLOAT_EQ(kilohertz.toHertz(48000.0f), 1200.0f);
    EXPECT_FLOAT_EQ(normalized.toHertz(48000.0f), 1200.0f);
    EXPECT_NEAR(radians.toHertz(48000.0f), 1200.0f, 0.001f);
}

TEST(QuantityLiteralsTest, GainLiteralsPreserveUnitsAndValues) {
    const auto decibels = -6.0_db;
    const auto linear = 0.5_lin;
    EXPECT_EQ(decibels.unit, GainUnit::Decibels);
    EXPECT_EQ(linear.unit, GainUnit::Linear);
    EXPECT_FLOAT_EQ(decibels.toDecibels(), -6.0f);
    EXPECT_FLOAT_EQ(linear.toLinear(), 0.5f);
}
