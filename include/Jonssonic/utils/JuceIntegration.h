// Jonssonic - A C++ audio DSP library
// JUCE integration helpers
// Author: Jon Fagerström
// Update: 18.11.2025

#pragma once
#include <vector>

namespace Jonssonic::utils
{
/**
 * @brief Helper class to bridge between JUCE AudioBuffer and Jonssonic's deinterleaved API.
 * 
 * This class manages the pointer arrays needed to interface with Jonssonic processors
 * from JUCE code. Create once and reuse for the lifetime of your processor.
 * 
 * @tparam T Sample type (typically float or double)
 */
template<typename T>
class JuceBufferAdapter
{
public:
    /**
     * @brief Constructor.
     * @param maxChannels Maximum number of channels to support
     */
    explicit JuceBufferAdapter(size_t maxChannels = 8)
        : inputPtrs(maxChannels)
        , outputPtrs(maxChannels)
    {}

    /**
     * @brief Prepare the adapter for a specific channel count.
     * @param numChannels Number of channels
     */
    void prepare(size_t numChannels)
    {
        if (numChannels > inputPtrs.size())
        {
            inputPtrs.resize(numChannels);
            outputPtrs.resize(numChannels);
        }
        currentNumChannels = numChannels;
    }

    /**
     * @brief Update pointers from a JUCE AudioBuffer for the current sample position.
     * Call this before processing each sample.
     * 
     * @param buffer The JUCE AudioBuffer (can be const or non-const)
     * @param sampleIndex Current sample index in the buffer
     * 
     * @note This sets both input and output pointers to the same buffer location.
     *       Use this for in-place processing.
     */
    template<typename BufferType>
    void setPointers(BufferType& buffer, int sampleIndex = 0)
    {
        for (size_t ch = 0; ch < currentNumChannels; ++ch)
        {
            T* channelPtr = buffer.getWritePointer(static_cast<int>(ch)) + sampleIndex;
            inputPtrs[ch] = channelPtr;
            outputPtrs[ch] = channelPtr;
        }
    }

    /**
     * @brief Update pointers from separate input and output JUCE AudioBuffers.
     * Use this when you need separate input/output buffers.
     * 
     * @param inputBuffer The input JUCE AudioBuffer
     * @param outputBuffer The output JUCE AudioBuffer
     * @param sampleIndex Current sample index in the buffers
     */
    template<typename BufferType>
    void setPointers(const BufferType& inputBuffer, BufferType& outputBuffer, int sampleIndex = 0)
    {
        for (size_t ch = 0; ch < currentNumChannels; ++ch)
        {
            inputPtrs[ch] = inputBuffer.getReadPointer(static_cast<int>(ch)) + sampleIndex;
            outputPtrs[ch] = outputBuffer.getWritePointer(static_cast<int>(ch)) + sampleIndex;
        }
    }

    /**
     * @brief Set up pointers for block processing (entire buffer at once).
     * Use this when processing the whole block instead of sample-by-sample.
     * 
     * @param buffer The JUCE AudioBuffer for in-place processing
     * 
     * @note The pointers point to the start of each channel buffer.
     *       Use this with block-based process methods.
     */
    template<typename BufferType>
    void setPointersForBlock(BufferType& buffer)
    {
        for (size_t ch = 0; ch < currentNumChannels; ++ch)
        {
            T* channelPtr = buffer.getWritePointer(static_cast<int>(ch));
            inputPtrs[ch] = channelPtr;
            outputPtrs[ch] = channelPtr;
        }
    }

    /**
     * @brief Set up pointers for block processing with separate input/output buffers.
     * 
     * @param inputBuffer The input JUCE AudioBuffer
     * @param outputBuffer The output JUCE AudioBuffer
     */
    template<typename BufferType>
    void setPointersForBlock(const BufferType& inputBuffer, BufferType& outputBuffer)
    {
        for (size_t ch = 0; ch < currentNumChannels; ++ch)
        {
            inputPtrs[ch] = inputBuffer.getReadPointer(static_cast<int>(ch));
            outputPtrs[ch] = outputBuffer.getWritePointer(static_cast<int>(ch));
        }
    }

    /**
     * @brief Get input channel pointers for passing to Jonssonic processors.
     */
    const T* const* getInputPointers() const { return inputPtrs.data(); }

    /**
     * @brief Get output channel pointers for passing to Jonssonic processors.
     */
    T* const* getOutputPointers() { return outputPtrs.data(); }

    /**
     * @brief Get the current number of channels.
     */
    size_t getNumChannels() const { return currentNumChannels; }

private:
    std::vector<const T*> inputPtrs;
    std::vector<T*> outputPtrs;
    size_t currentNumChannels = 0;
};

} // namespace Jonssonic::utils
