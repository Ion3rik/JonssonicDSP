// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Global configuration header file
// SPDX-License-Identifier: MIT
#pragma once
#include <cstddef>

#define JONSSONIC_VERSION_MAJOR 0
#define JONSSONIC_VERSION_MINOR 1
#define JONSSONIC_VERSION_PATCH 0
#define JONSSONIC_VERSION (JONSSONIC_VERSION_MAJOR * 10000 + JONSSONIC_VERSION_MINOR * 100 + JONSSONIC_VERSION_PATCH)

namespace jonssonic {
    /// Maximum number of supported channels
    constexpr size_t JONSSONIC_MAX_CHANNELS = 64;
    /// Supported sample rate range
    constexpr double JONSSONIC_MIN_SAMPLE_RATE = 8000;
    constexpr double JONSSONIC_MAX_SAMPLE_RATE = 192000;
// Add other global config options here as needed
}
