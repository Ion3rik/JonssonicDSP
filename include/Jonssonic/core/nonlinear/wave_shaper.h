// Jonssonic - A C++ audio DSP library
// WaveShaper class header file
// SPDX-License-Identifier: MIT

#pragma once
#include "../common/AudioBuffer.h"
#include "../../utils/MathUtils.h"
#include "../common/Interpolators.h"
#include "../common/DspParam.h"
#include <functional>
#include <cmath>

namespace Jonssonic {

enum class WaveShaperType {
	None,
	HardClip,
	Atan,
	Tanh,
	FullWaveRectifier,
	HalfWaveRectifier,
	Cubic,
	Dynamic,
	Custom
};


/**
 * @brief A waveshaper class for nonlinear distortion effects
 * @tparam T Sample data type (e.g., float, double)
 * @tparam Type Type of waveshaper (from WaveShaperType enum)
 */

// Template declaration
template<typename T, WaveShaperType Type>
class WaveShaper;

// =====================================================================
// Passthrough specialization
// =====================================================================
/**
 * @brief Passthrough specialization (no processing). Outputs input directly.
 */
template<typename T>
class WaveShaper<T, WaveShaperType::None> {
public:
	T processSample(T x, T /*shape*/) const {
		return x;
	}

	void processBlock(const T* const* input, T* const* output, size_t numChannels, size_t numSamples) const {
		for (size_t ch = 0; ch < numChannels; ++ch) {
			std::memcpy(output[ch], input[ch], numSamples * sizeof(T));
		}
	}
};

// =====================================================================
// HardClip specialization
// =====================================================================
/**
 * @brief Hard clipper specialization.
 *        Clamps the signal to [-1, 1].
 */
template<typename T>
class WaveShaper<T, WaveShaperType::HardClip> {
public:
	T processSample(T x, T shape = T(0)) const {
		return std::max<T>(-1, std::min<T>(1, x));
	}

	void processBlock(const T* const* input, T* const* output, size_t numChannels, size_t numSamples) const {
		for (size_t ch = 0; ch < numChannels; ++ch) {
			for (size_t n = 0; n < numSamples; ++n) {
				output[ch][n] = processSample(input[ch][n]);
			}
		}
	}
};
// =====================================================================
// Atan specialization
// =====================================================================
/**
 * @brief Atan shaper specialization.
 *        Applies atan(x) for smooth limiting, normalized to [-1, 1].
 */
template<typename T>
class WaveShaper<T, WaveShaperType::Atan> {
public:
	T processSample(T x, T shape = T(0)) const {
		return std::atan(x) * inv_atan_1<T>;
	}

	void processBlock(const T* const* input, T* const* output, size_t numChannels, size_t numSamples) const {
		for (size_t ch = 0; ch < numChannels; ++ch) {
			for (size_t n = 0; n < numSamples; ++n) {
				output[ch][n] = processSample(input[ch][n]);
			}
		}
	}
};

// =====================================================================
// Tanh specialization
// =====================================================================
/**
 * @brief Tanh shaper specialization.
 *        Applies std::tanh(x) for smooth saturation.
 */
template<typename T>
class WaveShaper<T, WaveShaperType::Tanh> {
public:
	T processSample(T x, T shape = T(0)) const {
		return std::tanh(x);
	}

	void processBlock(const T* const* input, T* const* output, size_t numChannels, size_t numSamples) const {
		for (size_t ch = 0; ch < numChannels; ++ch) {
			for (size_t n = 0; n < numSamples; ++n) {
				output[ch][n] = processSample(input[ch][n]);
			}
		}
	}
};

// =====================================================================
// FullWaveRectifier specialization
// =====================================================================
/**
 * @brief Full-wave rectifier specialization.
 *        Outputs the absolute value of the input.
 */
template<typename T>
class WaveShaper<T, WaveShaperType::FullWaveRectifier> {
public:
	T processSample(T x, T shape = T(0)) const {
		return std::abs(x);
	}

	void processBlock(const T* const* input, T* const* output, size_t numChannels, size_t numSamples) const {
		for (size_t ch = 0; ch < numChannels; ++ch) {
			for (size_t n = 0; n < numSamples; ++n) {
				output[ch][n] = processSample(input[ch][n]);
			}
		}
	}
};

// =====================================================================
// HalfWaveRectifier specialization
// =====================================================================
/**
 * @brief Half-wave rectifier specialization.
 *        Outputs zero for negative input, passes positive input.
 */
template<typename T>
class WaveShaper<T, WaveShaperType::HalfWaveRectifier> {
public:
	T processSample(T x, T shape = T(0)) const {
		return x < 0 ? 0 : x;
	}

	void processBlock(const T* const* input, T* const* output, size_t numChannels, size_t numSamples) const {
		for (size_t ch = 0; ch < numChannels; ++ch) {
			for (size_t n = 0; n < numSamples; ++n) {
				output[ch][n] = processSample(input[ch][n]);
			}
		}
	}
};

// =====================================================================
// Cubic specialization
// =====================================================================
/**
 * @brief Cubic shaper specialization.
 *        Applies a cubic nonlinearity for soft clipping.
 * 	  f(x) = x - (1/3)x^3
 */
template<typename T>
class WaveShaper<T, WaveShaperType::Cubic> {
public:
	T processSample(T x, T shape = T(0)) const {
		return x - (T(1)/T(3)) * x * x * x;
	}

	void processBlock(const T* const* input, T* const* output, size_t numChannels, size_t numSamples) const {
		for (size_t ch = 0; ch < numChannels; ++ch) {
			for (size_t n = 0; n < numSamples; ++n) {
				output[ch][n] = processSample(input[ch][n]);
			}
		}
	}
};

// =====================================================================
// Dynamic specialization (shape parameter for soft/hard clipping)
// =====================================================================
/**
 * @brief Dynamic shaper specialization.
 *        Applies a shape-controlled clipping.
 *        Shape parameter controls the clipping curvature.
 */
template<typename T>
class WaveShaper<T, WaveShaperType::Dynamic> {
public:
	/**
	 * @param x     Input sample
	 * @param shape Raw shape parameter (usable range ~ [2, 20])
	 */
	T processSample(T x, T shape = T(0)) const {
		return x * T(1) / std::pow(T(1) + std::pow(std::abs(x), shape), T(1)/shape);
	}

	void processBlock(const T* const* input, T* const* output, const T* const* shape, size_t numChannels, size_t numSamples) const {
		for (size_t ch = 0; ch < numChannels; ++ch) {
			for (size_t n = 0; n < numSamples; ++n) {
				output[ch][n] = processSample(input[ch][n], shape[ch][n]);
			}
		}
	}
};

// =====================================================================
// Custom specialization (function pointer required at construction)
// =====================================================================    
/**
 * @brief Custom shaper specialization.
 *        Uses a user-supplied function for shaping.
 *
 * @note  Requires a function to be provided at construction.
 */
template<typename T>
class WaveShaper<T, WaveShaperType::Custom> {
public:
	using FnType = std::function<T(T)>;
	WaveShaper(FnType fn) : customFn(std::move(fn)) {}
	T processSample(T x, T shape = T(0)) const {
		return customFn ? customFn(x) : x;
	}

	void processBlock(const T* const* input, T* const* output, size_t numChannels, size_t numSamples) const {
		for (size_t ch = 0; ch < numChannels; ++ch) {
			for (size_t n = 0; n < numSamples; ++n) {
				output[ch][n] = processSample(input[ch][n]);
			}
		}
	}
private:
	FnType customFn;
};

} // namespace Jonssonic