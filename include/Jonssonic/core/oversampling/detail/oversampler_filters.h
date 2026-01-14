// Jonssonic - A C++ audio DSP library
// OversamplerFilter classes
// SPDX-License-Identifier: MIT

#pragma once
#include <jonssonic/core/common/circular_audio_buffer.h>
#include <jonssonic/utils/math_utils.h>
#include <numeric>
#include <vector>
namespace jnsc::detail {

//==============================================================================
// FIR HALFBAND FILTER STAGE
// =============================================================================
/**
 * @brief FIR Halfband filter stage for 2x up/downsampling
 * @tparam T Sample data type
 * @tparam FIRTaps Number of FIR filter taps
 */
template <typename T, size_t FIRTaps = 31>
class FIRHalfbandStage {
    // Compile-time checks
    static_assert(FIRTaps == 31, "Only 31-tap FIR halfband filter is supported currently");

  public:
    FIRHalfbandStage() = default;
    ~FIRHalfbandStage() = default;

    // No copy or move semantics
    FIRHalfbandStage(const FIRHalfbandStage&) = delete;
    FIRHalfbandStage& operator=(const FIRHalfbandStage&) = delete;
    FIRHalfbandStage(FIRHalfbandStage&&) = delete;
    FIRHalfbandStage& operator=(FIRHalfbandStage&&) = delete;

    void prepare(size_t newNumChannels) {
        numChannels = newNumChannels;

        // Allocate buffers
        upsamplerBuffer.resize(newNumChannels, FIRTaps);
        // Downsampler uses interleaved polyphase branch channels: even indices for even branch, odd
        // for odd branch
        downsamplerBuffer.resize(newNumChannels * 2, FIRTaps / 2 + 1);

        // Initialize filter coefficients
        prepareCoeffs();
    }

    void reset() {
        upsamplerBuffer.clear();
        downsamplerBuffer.clear();
    }

    /**
     * @brief Upsample input signal by 2x using FIR halfband filter
     * @param input Input audio buffer (deinterleaved)
     * @param output Output audio buffer (deinterleaved)
     * @param numInputSamples Number of input samples per channel
     * @note Output buffer must have space for 2 * numInputSamples samples per channel
     */
    void upsample(const T* const* input, T* const* output, size_t numInputSamples) {

        for (size_t ch = 0; ch < numChannels; ++ch) {
            for (size_t n = 0; n < numInputSamples; ++n) {
                // Push new sample into the circular buffer
                upsamplerBuffer.write(ch, input[ch][n]);

                // Polyphase filtering - even branch (symmetric optimization)
                T y0 = T(0);
                for (size_t k = 0; k < K0; ++k) {
                    T sum = upsamplerBuffer.read(ch, k) + upsamplerBuffer.read(ch, halfFIRTaps - k);
                    y0 += coeffs0[k] * sum;
                }

                // Polyphase filtering - odd branch
                // For halfband filters, only the center tap (0.5) is non-zero
                T y1 = T(0.5) * upsamplerBuffer.read(ch, centerTapIdx);

                // Upsampled output (with 2x gain compensation)
                output[ch][2 * n] = 2 * y0;
                output[ch][2 * n + 1] = 2 * y1;
            }
        }
    }

    /**
     * @brief Downsample input signal by 2x using FIR halfband filter
     * @param input Input audio buffer (deinterleaved)
     * @param output Output audio buffer (deinterleaved)
     * @param numOutputSamples Number of output samples per channel
     * @note Input buffer must have at least 2 * numOutputSamples samples per channel
     * @note This applies the full anti-aliasing filter then decimates by 2
     */
    void downsample(const T* const* input, T* const* output, size_t numOutputSamples) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            // Polyphase branch channels: even branch at ch*2, odd branch at ch*2+1
            const size_t evenCh = ch * 2;
            const size_t oddCh = ch * 2 + 1;

            for (size_t n = 0; n < numOutputSamples; ++n) {
                // Push even and odd samples into separate polyphase branches
                downsamplerBuffer.write(evenCh, input[ch][2 * n]);    // even sample
                downsamplerBuffer.write(oddCh, input[ch][2 * n + 1]); // odd sample

                // Filter even branch (h0 coefficients, symmetric optimization)
                T y0 = T(0);
                for (size_t k = 0; k < K0; ++k) {
                    T sum = downsamplerBuffer.read(evenCh, k) +
                            downsamplerBuffer.read(evenCh, halfFIRTaps - k);
                    y0 += coeffs0[k] * sum;
                }

                // Filter odd branch - only center tap (0.5) with one-sample delay
                // The +1 accounts for the z^-1 delay in the downsampler odd branch
                T y1 = T(0.5) * downsamplerBuffer.read(oddCh, centerTapIdx + 1);

                // Combine polyphase branches
                output[ch][n] = y0 + y1;
            }
        }
    }

    constexpr T getLatencySamples() const {
        // Latency is (FIRTaps - 1) / 2 samples due to linear phase FIR filter
        return (FIRTaps - 1) / 2;
    }

  private:
    size_t numChannels = 0; // number of channels

    // BUFFERS
    CircularAudioBuffer<T> upsamplerBuffer;   // Circular audio buffer for upsampler filter state
    CircularAudioBuffer<T> downsamplerBuffer; // Polyphase buffer: even channels for even branch,
                                              // odd channels for odd branch

    // COEFFICIENTS
    static constexpr size_t K0 =
        (FIRTaps / 2 + 1) /
        2; // Number of unique symmetric even polyphase coefficients actually stored
    static constexpr size_t centerTapIdx = FIRTaps / 4; // Center tap index in odd polyphase branch
    static constexpr size_t halfFIRTaps = FIRTaps / 2;  // Half the number of FIR taps
    std::array<T, K0> coeffs0; // Even polyphase coefficients (odd branch is just 0.5 * center tap)

    /**
     * @brief Extract polyphase components from full filter coefficients
     * @param fullCoeffs Full halfband filter coefficients
     */
    void extractPolyphaseComponents(const std::array<T, 31>& fullCoeffs) {
        // Store only the first half of the even coefficients (symmetric),
        // since h[k] == h[N-1-k] for linear-phase FIR
        for (size_t i = 0; i < K0; ++i) {
            coeffs0[i] = fullCoeffs[i * 2];
        }
    }

    /**
     * @brief Get FIR halfband filter coefficients.
     * @return Vector of filter coefficients
     * @note Only a 31-tap filter is implemented for now.
     */
    void prepareCoeffs() {
        if constexpr (FIRTaps == 31) {
            // Predefined 31-tap halfband filter coefficients
            static constexpr std::array<T, 31> coeffs = {
                T(-0.0004), T(0), T(0.0018),  T(0), T(-0.0051), T(0), T(0.0116),  T(0),
                T(-0.0237), T(0), T(0.046),   T(0), T(-0.0945), T(0), T(0.3143),
                T(0.5), // center tap
                T(0.3143),  T(0), T(-0.0945), T(0), T(0.046),   T(0), T(-0.0237), T(0),
                T(0.0116),  T(0), T(-0.0051), T(0), T(0.0018),  T(0), T(-0.0004)};

            assert(std::abs(coeffs[FIRTaps / 2] - T(0.5)) < T(1e-6) && "Center tap must be 0.5");
            extractPolyphaseComponents(coeffs);
        } else {
            // Other filter lengths can be added here
        }
    }
};

//==============================================================================
// IIR FILTER STAGE
// =============================================================================

} // namespace jnsc::detail