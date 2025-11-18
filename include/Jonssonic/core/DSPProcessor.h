#pragma once
#include <cstddef>

namespace Jonssonic::core
{
class DSPProcessor
{
public:
    DSPProcessor() = default; // default constructor
    virtual ~DSPProcessor() = default; // virtual destructor


    // Prevent copying and moving
    DSPProcessor(const DSPProcessor&) = delete; // no copy constructor
    DSPProcessor& operator=(const DSPProcessor&) = delete; // no copy assignment
    DSPProcessor(DSPProcessor&&) = delete; // no move constructor
    DSPProcessor& operator=(DSPProcessor&&) = delete; // no move assignment


    virtual void process(const float* const* input, float* const* output, size_t numOutChannels, size_t numInChannels, size_t numSamples) = 0;

};
} // namespace Jonssonic::Core