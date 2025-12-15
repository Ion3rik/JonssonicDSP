// Jonssonic - A C++ audio DSP library
// MixingMatrix class header
// SPDX-License-Identifier: MIT

#pragma once
#include <array>
#include <cstddef>

namespace Jonssonic {

// Supported matrix types at compile time
enum class MixingMatrixType {
	Identity,
	Hadamard,
	Householder,
    RandomOrthogonal
	// Add more as needed
};

/**
 * @brief Compile-time mixing matrix for FDN and other multichannel mixing tasks.
 * @tparam T Sample type (float, double, etc.)
 * @tparam N Input size
 * @tparam Type Matrix type (Identity, Hadamard, etc.)
 */
template <
        typename T, 
        size_t NumInputs,
        size_t NumOutputs,
        MixingMatrixType Type = MixingMatrixType::Hadamard>
class MixingMatrix;

// ===============================================================================
// Identity specialization
// ===============================================================================
template <typename T, size_t NumInputs, size_t NumOutputs>
class MixingMatrix<T, NumInputs, NumOutputs, MixingMatrixType::Identity> {
public:
    static_assert(NumInputs == NumOutputs, "Identity matrix requires equal number of inputs and outputs.");
	static constexpr std::size_t N = NumInputs;

    /**
     * @brief Process single sample of all channels.
     * @param input Input sample array
     * @param output Output sample array
     */

	/**
	 * @brief Process a single sample for all channels and internal nodes (planar buffer).
	 * @param input 2D input buffer: input[channel][node]
	 * @param output 2D output buffer: output[channel][node]
	 * @param numChannels Number of audio channels
	 * @param fdnSize Number of internal nodes per channel
	 */
	void processSample(const T* const* input, T** output, size_t idx) const {
		for (size_t ch = 0; ch < N; ++ch) {
				output[ch][idx] = input[ch][idx];
			}
		}
	}


};
// ===============================================================================
// Hadamard specialization (Sylvester's construction, N must be power of 2)
// ===============================================================================
template <typename T, std::size_t NumInputs, std::size_t NumOutputs>
class MixingMatrix<T, NumInputs, NumOutputs, MixingMatrixType::Hadamard> {
public:
	static constexpr std::size_t size = N;

    /**
     * @brief Process single sample of all channels.
     * @param input Input sample array
     * @param output Output sample array
     */
	void processSample(const T* input, T* output) const {
		hadamardTransform(input, output);
	}

private:
	// In-place Hadamard transform (recursive, output must not alias input)
	void hadamardTransform(const T* input, T* output) const {
		for (std::size_t i = 0; i < N; ++i) output[i] = input[i];
		for (std::size_t len = 1; len < N; len <<= 1) {
			for (std::size_t i = 0; i < N; i += (len << 1)) {
				for (std::size_t j = 0; j < len; ++j) {
					T u = output[i + j];
					T v = output[i + j + len];
					output[i + j] = u + v;
					output[i + j + len] = u - v;
				}
			}
		}
		// Normalize
		T norm = static_cast<T>(1) / static_cast<T>(std::sqrt(N));
		for (std::size_t i = 0; i < N; ++i) output[i] *= norm;
	}
};

// ===============================================================================
// Householder specialization (reflection across a vector, here: all-ones vector)
// ===============================================================================
template <typename T, std::size_t NumInputs, std::size_t NumOutputs>
class MixingMatrix<T, NumInputs, NumOutputs, MixingMatrixType::Householder> {
public:
	static constexpr std::size_t size = N;

	// Applies the Householder matrix: H = I - 2vv^T/(v^T v), v = [1,1,...,1]
	void apply(const T* input, T* output) const {
		T sum = 0;
		for (std::size_t i = 0; i < N; ++i) sum += input[i];
		for (std::size_t i = 0; i < N; ++i) {
			output[i] = input[i] - (2 * sum) / static_cast<T>(N);
		}
	}
};

} // namespace Jonssonic
