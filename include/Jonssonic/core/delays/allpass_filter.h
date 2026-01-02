// Jonssonic - A C++ audio DSP library
// Allpass filter class header file
// SPDX-License-Identifier: MIT

#pragma once
#include <jonssonic/core/delays/delay_line.h>
#include <jonssonic/core/common/interpolators.h>
#include <jonssonic/core/common/modulation.h>

namespace jonssonic::core::delays
{
// MODULATION STRUCTURES
namespace AllpassMod{
/** 
 * @brief Modulation input structure for AllpassFilter single sample processing
 * @param T Sample data type (e.g., float, double)
 */
template<typename T>
struct Sample: public common::ModulationInput<T> {
    T delayMod = T(0);
    T gainMod = T(1);
};

/**
 * @brief Modulation input structure for AllpassFilter block processing
 * @param T Sample data type (e.g., float, double)
 */
template<typename T>
struct Block: public common::ModulationInput<T> {
    const T* const* delayMod = nullptr;         // modulation buffer for delay time (in samples)
    const T* const* gainMod = nullptr;          // modulation buffer for gain
};
} // namespace AllpassMod

/**
 * @brief Allpass Filter
 * @param T Sample data type (e.g., float, double)
 * @param Interpolator Interpolator type for fractional delay support (default: LinearInterpolator)
 * @param SmootherType Type of smoother for delay time modulation (default: OnePole)
 * @param SmootherOrder Order of the smoother for delay time modulation (default: 1)
 * @param SmoothingTimeMs Smoothing time in milliseconds for delay time changes (default: 10ms)
 */
template<
	typename T,
	typename Interpolator = common::LinearInterpolator<T>,
	common::SmootherType SmootherType = common::SmootherType::OnePole,
	int SmootherOrder = 1
>
class AllpassFilter{
/// Type aliases for convenience, readability, and future-proofing
using DelayLineType = DelayLine<T, Interpolator, SmootherType, SmootherOrder>;
using DspParamType = common::DspParam<T, SmootherType, SmootherOrder>;
public:
    /// Default constructor
	AllpassFilter() = default;
    /**
     * Parmetrized constructor that calls @ref prepare.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     * @param newMaxDelayMs Maximum delay time in milliseconds
     */
    AllpassFilter(size_t newNumChannels, T newSampleRate, T newMaxDelayMs)
    {
        prepare(newNumChannels, newSampleRate, newMaxDelayMs);
    }

    /// Default destructor
	~AllpassFilter() = default;

    /// No copy semantics nor move semantics
    AllpassFilter(const AllpassFilter&) = delete;
    const AllpassFilter& operator=(const AllpassFilter&) = delete;
    AllpassFilter(AllpassFilter&&) = delete;
    const AllpassFilter& operator=(AllpassFilter&&) = delete;

    /**
     * @brief Prepare the allpass filter for processing.
     * @param newNumChannels Number of channels
     * @param newSampleRate Sample rate in Hz
     * @param newMaxDelayMs Maximum delay time in milliseconds
     */
	void prepare(size_t newNumChannels, T newSampleRate, T newMaxDelayMs)
	{
		delayLine.prepare(newNumChannels, newSampleRate, newMaxDelayMs);
		numChannels = newNumChannels;
		gain.prepare(numChannels, newSampleRate);
		gain.setBounds(T(-1), T(1));
		gain.setTarget(T(0), true);
	}

    /// Reset the allpass filter state
	void reset()
	{
		delayLine.reset();
		gain.reset();
	}

	/**
	 * @brief Process a single sample for a specific channel.
	 * @param ch Channel index
	 * @param input Input sample
	 * @return Output sample
	 */
	T processSample(size_t ch, T input)
	{
		// Get smoothed gain value
		T gainValue = gain.getNextValue(ch);

        // Read delayed feedback state
        T delayed = delayLine.readSample(ch);

        // Compute output
        T output = gainValue * input + delayed;

        // Compute and write new feedback state
        T newFeedback = input - gainValue * output;
        delayLine.writeSample(ch, newFeedback);

		return output;
	}

    /**
     * @brief Process a single sample for a specific channel, with modulation.
     * @param ch Channel index
     * @param input Input sample
     * @param modStruct Modulation structure containing modulation data
     * @return Output sample
     */
    T processSample(size_t ch, T input, AllpassMod::Sample<T>& modStruct)
    {
        // Apply modulation to feedback and feedforward gains
        T modulatedGain = gain.applyMultiplicativeMod(ch,modStruct.gainMod);
        
        // Read delayed feedback state with modulation
        T delayed = delayLine.readSample(ch, modStruct.delayMod);

        // Compute output
        T output = modulatedGain * input + delayed;

        // Compute and write new feedback state
        T newFeedback = input - modulatedGain * output;
        delayLine.writeSample(ch, newFeedback);

		return output;   
    }

    /**
	 * @brief Process a block of samples for all channels (no modulation).
	 * @param input Input sample pointers (one per channel)
	 * @param output Output sample pointers (one per channel)
	 * @param numSamples Number of samples to process
	 * @note Input and output must have the same number of channels as prepared.
	 */
	void processBlock(const T* const* input, T* const* output, size_t numSamples)
	{
		for (size_t ch = 0; ch < numChannels; ++ch)
		{
			for (size_t i = 0; i < numSamples; ++i)
			{
				// Get smoothed gain value
                T gainValue = gain.getNextValue(ch);

                // Read delayed feedback state
                T delayed = delayLine.readSample(ch);

                // Compute output
                output[ch][i] = gainValue * input[ch][i] + delayed;

                // Compute and write new feedback state
                T newFeedback = input[ch][i] - gainValue * output[ch][i];
                delayLine.writeSample(ch, newFeedback);
			}
		}
	}

    /**
     * @brief Process a block of samples for all channels, with modulation.
     * @param input Input sample pointers (one per channel)
     * @param output Output sample pointers (one per channel)
     * @param modStruct Modulation structure containing modulation buffers
     * @param numSamples Number of samples to process
     * @note Input and output must have the same number of channels as prepared.
     */
    void processBlock(const T* const* input, T* const* output, AllpassMod::Block<T>& modStruct, size_t numSamples)
    {
        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            for (size_t i = 0; i < numSamples; ++i)
            {
			    // Apply modulation to feedback and feedforward gains
                T modulatedGain = gain.applyMultiplicativeMod(ch,modStruct.gainMod[ch][i]);

                // Read delayed feedback state with modulation
                T delayed = delayLine.readSample(ch, modStruct.delayMod[ch][i]);

                // Compute output
                output[ch][i] = modulatedGain * input[ch][i] + delayed;

                // Compute and write new feedback state
                T newFeedback = input[ch][i] - modulatedGain * output[ch][i];
                delayLine.writeSample(ch, newFeedback);
			}
		}
	}

    /**
     * @brief Set the parameter smoothing time in milliseconds.
     * @param timeMs Smoothing time in milliseconds.
     */
    void setParameterSmoothingTimeMs(T timeMs)
    {
        gain.setSmoothingTimeMs(timeMs);
        delayLine.setSmoothingTimeMs(timeMs);
    }

    /**
     * @brief Set a constant delay time in milliseconds for all channels.
     * @param newDelayMs Delay time in milliseconds
     */
    void setDelayMs(T newDelayMs, bool skipSmoothing = false)
	{
		delayLine.setDelayMs(newDelayMs, skipSmoothing);
	}

    /**
     * @brief Set a constant delay time in milliseconds for a specific channel.
     * @param ch Channel index
     * @param newDelayMs Delay time in milliseconds
     */
	void setDelayMs(size_t ch, T newDelayMs, bool skipSmoothing = false)
	{
		delayLine.setDelayMs(ch, newDelayMs, skipSmoothing);
	}

    /**
     * @brief Set a constant delay time in samples for all channels.
     * @param newDelaySamples Delay time in samples
     */
    void setDelaySamples(T newDelaySamples, bool skipSmoothing = false)
    {
        delayLine.setDelaySamples(newDelaySamples, skipSmoothing);
    }

    /**
     * @brief Set a constant delay time in samples for a specific channel.
     * @param ch Channel index
     * @param newDelaySamples Delay time in samples
     */
    void setDelaySamples(size_t ch, T newDelaySamples, bool skipSmoothing = false)
    {
        delayLine.setDelaySamples(ch, newDelaySamples, skipSmoothing);
    }

    /**
     * @brief Set a constant gain value for all channels.
     * @param newGain Gain value
     */
	void setGain(T newGain, bool skipSmoothing = false)
	{
		gain.setTarget(newGain, skipSmoothing);
	}
	/**
     * @brief Set a constant gain value for a specific channel.
     * @param ch Channel index
     * @param newGain Gain value
     */
	void setGain(size_t ch, T newGain, bool skipSmoothing = false)
	{
		gain.setTarget(ch, newGain, skipSmoothing);
	}
    
private:
	DelayLineType delayLine;
	DspParamType gain;
	size_t numChannels = 0;
};
} // namespace jonssonic::core::delays
