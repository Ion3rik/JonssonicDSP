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
	HardClip,
	SoftClip,
	Tanh,
	FullWaveRectifier,
	HalfWaveRectifier,
	Custom
};

/**
 * @brief A waveshaper class for nonlinear distortion effects
 * @tparam SampleType Sample data type (e.g., float, double)
 * @tparam Type Type of waveshaper (from WaveShaperType enum)
 */

// Template declaration
template<typename SampleType, WaveShaperType Type>
class WaveShaper;

// =====================================================================
// HardClip specialization
// =====================================================================
/**
 * @brief Hard clipper specialization.
 *        Clamps the signal to [-1, 1].
 */
template<typename SampleType>
class WaveShaper<SampleType, WaveShaperType::HardClip> {
public:
	SampleType processSample(SampleType x) const {
		return std::max<SampleType>(-1, std::min<SampleType>(1, x));
	}
	void processBlock(SampleType* data, size_t numSamples) const {
		for (size_t i = 0; i < numSamples; ++i)
			data[i] = processSample(data[i]);
	}
};
// =====================================================================
// SoftClip specialization
// =====================================================================
/**
 * @brief Soft clipper specialization.
 *        Applies atan(x) for smooth limiting.
 */
template<typename SampleType>
class WaveShaper<SampleType, WaveShaperType::SoftClip> {
public:
	SampleType processSample(SampleType x) const {
		return std::atan(x);
	}
	void processBlock(SampleType* data, size_t numSamples) const {
		for (size_t i = 0; i < numSamples; ++i)
			data[i] = processSample(data[i]);
	}
};

// =====================================================================
// Tanh specialization
// =====================================================================
/**
 * @brief Tanh shaper specialization.
 *        Applies std::tanh(x) for smooth saturation.
 */
template<typename SampleType>
class WaveShaper<SampleType, WaveShaperType::Tanh> {
public:
	SampleType processSample(SampleType x) const {
		return std::tanh(x);
	}
	void processBlock(SampleType* data, size_t numSamples) const {
		for (size_t i = 0; i < numSamples; ++i)
			data[i] = processSample(data[i]);
	}
};

// =====================================================================
// FullWaveRectifier specialization
// =====================================================================
/**
 * @brief Full-wave rectifier specialization.
 *        Outputs the absolute value of the input.
 */
template<typename SampleType>
class WaveShaper<SampleType, WaveShaperType::FullWaveRectifier> {
public:
	SampleType processSample(SampleType x) const {
		return std::abs(x);
	}
	void processBlock(SampleType* data, size_t numSamples) const {
		for (size_t i = 0; i < numSamples; ++i)
			data[i] = processSample(data[i]);
	}
};

// =====================================================================
// HalfWaveRectifier specialization
// =====================================================================
/**
 * @brief Half-wave rectifier specialization.
 *        Outputs zero for negative input, passes positive input.
 */
template<typename SampleType>
class WaveShaper<SampleType, WaveShaperType::HalfWaveRectifier> {
public:
	SampleType processSample(SampleType x) const {
		return x < 0 ? 0 : x;
	}
	void processBlock(SampleType* data, size_t numSamples) const {
		for (size_t i = 0; i < numSamples; ++i)
			data[i] = processSample(data[i]);
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
template<typename SampleType>
class WaveShaper<SampleType, WaveShaperType::Custom> {
public:
	using FnType = std::function<SampleType(SampleType)>;
	WaveShaper(FnType fn) : customFn(std::move(fn)) {}
	SampleType processSample(SampleType x) const {
		return customFn ? customFn(x) : x;
	}
	void processBlock(SampleType* data, size_t numSamples) const {
		for (size_t i = 0; i < numSamples; ++i)
			data[i] = processSample(data[i]);
	}
private:
	FnType customFn;
};

} // namespace Jonssonic