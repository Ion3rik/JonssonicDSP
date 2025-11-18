// Jonssonic - A C++ audio DSP library
// Math utility functions
// Author: Jon Fagerström
// Update: 18.11.2025

#pragma once
#include <cstddef>

namespace Jonssonic::utils
{
/**
 * @brief Calculate the next power of two greater than or equal to n.
 * @param n Input value
 * @return The next power of two
 */
inline size_t nextPowerOfTwo(size_t n)
{
    if (n == 0) return 1;
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    return n + 1;
}

} // namespace Jonssonic::utils
