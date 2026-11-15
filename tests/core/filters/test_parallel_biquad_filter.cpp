// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Unit tests for parallel biquad section outputs
// SPDX-License-Identifier: MIT

#include <array>
#include <cmath>
#include <gtest/gtest.h>
#include <jonssonic/core/filters/biquad_filter.h>
#include <type_traits>
#include <utility>

using namespace jnsc;

namespace {
template <typename Filter>
using SampleType = decltype(std::declval<Filter>().getSampleRate());

template <typename Filter, typename = void>
struct HasScalarSample : std::false_type {};
template <typename Filter>
struct HasScalarSample<Filter, std::void_t<decltype(std::declval<Filter&>().processSample(
                                   size_t(0), SampleType<Filter>(0)))>> : std::true_type {};

template <typename Filter, typename = void>
struct HasSectionSamples : std::false_type {};
template <typename Filter>
struct HasSectionSamples<Filter, std::void_t<decltype(std::declval<Filter&>().processSample(
                                    size_t(0), SampleType<Filter>(0), static_cast<SampleType<Filter>*>(nullptr)))>>
    : std::true_type {};

template <typename Filter, typename = void>
struct HasScalarBlock : std::false_type {};
template <typename Filter>
struct HasScalarBlock<Filter, std::void_t<decltype(std::declval<Filter&>().processBlock(
                                  static_cast<const SampleType<Filter>* const*>(nullptr),
                                  static_cast<SampleType<Filter>* const*>(nullptr), size_t(0)))>> : std::true_type {};

template <typename Filter, typename = void>
struct HasSectionBlocks : std::false_type {};
template <typename Filter>
struct HasSectionBlocks<Filter, std::void_t<decltype(std::declval<Filter&>().processBlock(
                                    static_cast<const SampleType<Filter>* const*>(nullptr),
                                    static_cast<SampleType<Filter>* const* const*>(nullptr), size_t(0)))>>
    : std::true_type {};
} // namespace

TEST(BiquadProcessingApiTest, RoutingSelectsSampleAndBlockSignatures) {
    using SeriesFilter = Series<BiquadFilter<double>>;
    using ParallelFilter = Parallel<BiquadFilter<double>>;
    static_assert(HasScalarSample<SeriesFilter>::value);
    static_assert(!HasSectionSamples<SeriesFilter>::value);
    static_assert(HasScalarBlock<SeriesFilter>::value);
    static_assert(!HasSectionBlocks<SeriesFilter>::value);
    static_assert(!HasScalarSample<ParallelFilter>::value);
    static_assert(HasSectionSamples<ParallelFilter>::value);
    static_assert(!HasScalarBlock<ParallelFilter>::value);
    static_assert(HasSectionBlocks<ParallelFilter>::value);
    static_assert(std::is_same_v<decltype(std::declval<SeriesFilter&>().processSample(0, 0.0)), double>);
}

using ParallelBiquadVariants = ::testing::Types<
    Parallel<BiquadFilter<float, detail::DF2TBiquadTopology<float>>>,
    Parallel<BiquadFilter<float, detail::DF1BiquadTopology<float>>>,
    Parallel<BiquadFilter<double, detail::DF2TBiquadTopology<double>>>,
    Parallel<BiquadFilter<double, detail::DF1BiquadTopology<double>>>>;

template <typename Variant>
class ParallelBiquadFilterTest : public ::testing::Test {
  protected:
    using T = SampleType<Variant>;
    Variant filter;
    std::array<Series<Variant>, 3> reference;

    void SetUp() override {
        using Response = typename Variant::Response;
        const std::array<Response, 3> responses{Response::Lowpass, Response::Highpass, Response::Peak};
        filter.prepare(2, T(48000), 3);
        filter.setQ(T(0.5));
        for (size_t s = 0; s < 3; ++s) {
            const auto frequency = Frequency<T>::Hertz(T(600 * (s + 1)));
            filter.section(s).setResponse(responses[s]);
            filter.section(s).setFrequency(frequency);
            filter.section(s).setGain(Gain<T>::Decibels(T(6)));
            reference[s].prepare(2, T(48000));
            reference[s].setResponse(responses[s]);
            reference[s].setFrequency(frequency);
            reference[s].setQ(T(0.5));
            reference[s].setGain(Gain<T>::Decibels(T(6)));
        }
    }
};

TYPED_TEST_SUITE(ParallelBiquadFilterTest, ParallelBiquadVariants);

TYPED_TEST(ParallelBiquadFilterTest, SampleOutputsMatchIndependentBranches) {
    using T = SampleType<TypeParam>;
    for (size_t ch = 0; ch < 2; ++ch) {
        for (size_t n = 0; n < 256; ++n) {
            const T input = n == ch * 7 ? T(ch + 1) : T(0);
            std::array<T, 3> outputs;
            this->filter.processSample(ch, input, outputs.data());
            for (size_t s = 0; s < outputs.size(); ++s)
                EXPECT_EQ(outputs[s], this->reference[s].processSample(ch, input));
        }
    }
}

TYPED_TEST(ParallelBiquadFilterTest, BlockOutputsSupportInputAliasingAnyBranch) {
    using T = SampleType<TypeParam>;
    std::array<std::array<T, 128>, 2> input{};
    for (size_t ch = 0; ch < 2; ++ch)
        for (size_t n = 0; n < 128; ++n)
            input[ch][n] = T(std::sin(double(n) * 0.17 + double(ch)));

    for (int alias = -1; alias < 3; ++alias) {
        SCOPED_TRACE(alias);
        this->filter.reset();
        for (auto& branch : this->reference)
            branch.reset();
        auto aliasedInput = input;
        std::array<std::array<std::array<T, 128>, 2>, 3> storage{};
        std::array<std::array<T*, 2>, 3> channels;
        for (size_t s = 0; s < 3; ++s)
            for (size_t ch = 0; ch < 2; ++ch)
                channels[s][ch] = int(s) == alias ? aliasedInput[ch].data() : storage[s][ch].data();
        const T* sources[] = {aliasedInput[0].data(), aliasedInput[1].data()};
        T* const* const outputs[] = {channels[0].data(), channels[1].data(), channels[2].data()};
        this->filter.processBlock(sources, outputs, 128);
        for (size_t ch = 0; ch < 2; ++ch)
            for (size_t n = 0; n < 128; ++n)
                for (size_t s = 0; s < 3; ++s)
                    EXPECT_EQ(channels[s][ch][n], this->reference[s].processSample(ch, input[ch][n]));
    }
}

TYPED_TEST(ParallelBiquadFilterTest, EmptyBlockDoesNotChangeState) {
    using T = SampleType<TypeParam>;
    this->filter.processBlock(nullptr, nullptr, 0);
    std::array<T, 3> outputs;
    this->filter.processSample(0, T(1), outputs.data());
    for (size_t s = 0; s < outputs.size(); ++s)
        EXPECT_EQ(outputs[s], this->reference[s].processSample(0, T(1)));
}
