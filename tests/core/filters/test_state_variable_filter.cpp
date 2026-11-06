// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Unit tests for the StateVariableFilter class
// SPDX-License-Identifier: MIT

#include <gtest/gtest.h>
#include <jonssonic/core/common/audio_buffer.h>
#include <jonssonic/core/filters/state_variable_filter.h>
#include <complex>
#include <type_traits>
#include <vector>

using namespace jnsc;

template <typename T>
class StateVariableFilterTest : public ::testing::Test {};
using SampleTypes = ::testing::Types<float, double>;
TYPED_TEST_SUITE(StateVariableFilterTest, SampleTypes);

TYPED_TEST(StateVariableFilterTest, PrepareResetAndChannelIsolation) {
    using T = TypeParam;
    StateVariableFilter<T> filter;
    EXPECT_FALSE(filter.isPrepared());
    filter.prepare(2, T(48000));
    EXPECT_TRUE(filter.isPrepared());
    EXPECT_EQ(filter.getNumChannels(), 2);
    EXPECT_EQ(filter.getSampleRate(), T(48000));
    const auto first = filter.processSample(0, T(1), T(1000), T(0.707));
    for (size_t n = 0; n < 100; ++n) {
        filter.processSample(0, T(0), T(1000), T(0.707));
        for (T value : filter.processSample(1, T(0), T(1000), T(0.707)))
            EXPECT_EQ(value, T(0));
    }
    filter.reset();
    EXPECT_EQ(filter.processSample(0, T(1), T(1000), T(0.707)), first);
    filter.prepare(1, T(48000));
    EXPECT_EQ(filter.processSample(0, T(1), T(1000), T(0.707)), first);
}

TYPED_TEST(StateVariableFilterTest, AllFourResponsesMatchTheirTransferFunctions) {
    using T = TypeParam;
    constexpr double fs = 48000.0, pi = utils::pi<double>;
    constexpr size_t length = 16384;
    const double tolerance = std::is_same<T, float>::value ? 2e-4 : 1e-9;
    for (double center : {200.0, 1000.0, 8000.0}) {
        for (double q : {0.5, 2.0}) {
            StateVariableFilter<T> filter(1, T(fs));
            std::vector<std::array<T, 4>> impulse(length);
            for (size_t n = 0; n < length; ++n)
                impulse[n] = filter.processSample(0, n == 0 ? T(1) : T(0), T(center), T(q));
            for (double frequency : {0.0, 100.0, center, 5000.0, 20000.0}) {
                // Analog SVF responses evaluated through the bilinear transform.
                std::complex<double> s(0.0, std::tan(pi * frequency / fs) / std::tan(pi * center / fs));
                auto damping = s / q;
                auto denominator = s * s + damping + 1.0;
                std::array<std::complex<double>, 4> expected = {
                    1.0 / denominator, s * s / denominator, s / denominator,
                    (s * s - damping + 1.0) / denominator};
                std::array<std::complex<double>, 4> measured{};
                for (size_t n = 0; n < length; ++n) {
                    auto phase = std::polar(1.0, -2.0 * pi * frequency * n / fs);
                    for (size_t response = 0; response < 4; ++response)
                        measured[response] += double(impulse[n][response]) * phase;
                }
                for (size_t response = 0; response < 4; ++response)
                    EXPECT_LT(std::abs(measured[response] - expected[response]), tolerance)
                        << "response=" << response << " frequency=" << frequency << " center=" << center << " Q=" << q;
            }
        }
    }
}

template <typename T, Routing routing>
void checkModulatedBlock() {
    constexpr size_t channels = 2, length = 257;
    AudioBuffer<T> input(channels, length), frequency(channels, length), q(channels, length);
    for (size_t ch = 0; ch < channels; ++ch)
        for (size_t n = 0; n < length; ++n) {
            input[ch][n] = T(std::sin(double(n) * 0.13 + ch));
            frequency[ch][n] = T(200 + n * (ch + 1) * 20);
            q[ch][n] = T(0.5 + double(n) / length * (ch + 1));
        }
    // Verify each output can alias input; the last iteration uses separate input.
    for (size_t alias = 0; alias <= 4; ++alias) {
        StateVariableFilter<T, routing> sampleFilter(channels, T(48000), 3), blockFilter(channels, T(48000), 3);
        std::array<AudioBuffer<T>, 4> actual;
        for (auto& output : actual)
            output.resize(channels, length);
        if (alias < 4)
            for (size_t ch = 0; ch < channels; ++ch)
                for (size_t n = 0; n < length; ++n)
                    actual[alias][ch][n] = input[ch][n];
        const auto& source = alias < 4 ? actual[alias] : input;
        // Split the block to verify persistent state across calls.
        for (size_t offset = 0; offset < length;) {
            size_t count = std::min(size_t(37), length - offset);
            const T* in[] = {source.readChannelPtr(0) + offset, source.readChannelPtr(1) + offset};
            const T* f[] = {frequency.readChannelPtr(0) + offset, frequency.readChannelPtr(1) + offset};
            const T* Q[] = {q.readChannelPtr(0) + offset, q.readChannelPtr(1) + offset};
            T* outputs[4][channels];
            for (size_t response = 0; response < 4; ++response)
                for (size_t ch = 0; ch < channels; ++ch)
                    outputs[response][ch] = actual[response].writeChannelPtr(ch) + offset;
            blockFilter.processBlock(in, f, Q, outputs[0], outputs[1], outputs[2], outputs[3], count);
            offset += count;
        }
        for (size_t ch = 0; ch < channels; ++ch)
            for (size_t n = 0; n < length; ++n) {
                const auto expected = sampleFilter.processSample(ch, input[ch][n], frequency[ch][n], q[ch][n]);
                for (size_t response = 0; response < 4; ++response)
                    EXPECT_EQ(actual[response][ch][n], expected[response]);
            }
    }
}

TYPED_TEST(StateVariableFilterTest, ModulatedSeriesBlockMatchesSamplesForEveryResponseInPlace) {
    checkModulatedBlock<TypeParam, Routing::Series>();
}

TYPED_TEST(StateVariableFilterTest, ModulatedParallelBlockMatchesSamplesForEveryResponseInPlace) {
    checkModulatedBlock<TypeParam, Routing::Parallel>();
}

TYPED_TEST(StateVariableFilterTest, RoutingMatchesIndependentSectionsForAllResponses) {
    using T = TypeParam;
    constexpr size_t channels = 2, sections = 3;
    Series<StateVariableFilter<T>> series(channels, T(48000), sections);
    Parallel<StateVariableFilter<T>> parallel(channels, T(48000), sections);
    StateVariableFilter<T> single(channels, T(48000));
    // Each response has its own reference cascade of single-section filters.
    std::array<std::array<StateVariableFilter<T>, sections>, 4> cascades;
    for (auto& cascade : cascades)
        for (auto& section : cascade)
            section.prepare(channels, T(48000));
    EXPECT_EQ(series.getNumSections(), sections);
    EXPECT_EQ(parallel.getNumSections(), sections);
    for (size_t n = 0; n < 512; ++n) {
        for (size_t ch = 0; ch < channels; ++ch) {
            const T input = T(std::sin(double(n) * 0.17 + ch));
            const T f = T(200 + n * (ch + 1) * 10);
            const T q = T(0.5 + double(n) / 512);
            const auto serial = series.processSample(ch, input, f, q);
            const auto summed = parallel.processSample(ch, input, f, q);
            const auto singleOutput = single.processSample(ch, input, f, q);
            for (size_t response = 0; response < 4; ++response) {
                T expected = input;
                for (auto& section : cascades[response])
                    expected = section.processSample(ch, expected, f, q)[response];
                EXPECT_EQ(serial[response], expected);
                EXPECT_NEAR(summed[response], T(sections) * singleOutput[response],
                            T(16) * std::numeric_limits<T>::epsilon() *
                            std::max(T(1), std::abs(summed[response])));
            }
        }
    }
    series.reset();
    parallel.reset();
    for (size_t ch = 0; ch < channels; ++ch) {
        EXPECT_EQ(series.processSample(ch, T(0), T(1000), T(0.7)), (std::array<T, 4>{}));
        EXPECT_EQ(parallel.processSample(ch, T(0), T(1000), T(0.7)), (std::array<T, 4>{}));
    }
    series.prepare(channels, T(48000), 0);
    EXPECT_EQ(series.getNumSections(), 1);
    parallel.prepare(channels, T(48000), 1000);
    EXPECT_EQ(parallel.getNumSections(), detail::FilterLimits<T>::MAX_SECTIONS);
    EXPECT_EQ(parallel.processSample(0, T(0), T(1000), T(0.7)), (std::array<T, 4>{}));
}

TYPED_TEST(StateVariableFilterTest, FrequencyAndQAreAppliedImmediatelyAndClamped) {
    using T = TypeParam;
    StateVariableFilter<T> filter(1, T(48000)), reference(1, T(48000));
    EXPECT_EQ(filter.processSample(0, T(1), T(-100), T(0)),
              reference.processSample(0, T(1), T(48000) * detail::FilterLimits<T>::MIN_FREQ_NORM,
                                      detail::FilterLimits<T>::MIN_Q));
    filter.reset();
    reference.reset();
    EXPECT_EQ(filter.processSample(0, T(1), T(96000), T(100)),
              reference.processSample(0, T(1), T(48000) * detail::FilterLimits<T>::MAX_FREQ_NORM,
                                      detail::FilterLimits<T>::MAX_Q));
    filter.reset();
    reference.reset();
    const auto low = filter.processSample(0, T(1), T(100), T(0.5));
    const auto high = reference.processSample(0, T(1), T(8000), T(2));
    EXPECT_NE(low, high);
}
