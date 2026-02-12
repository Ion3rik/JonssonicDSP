// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
// Damping design class for calculating coefficients of damping filters based on T60 parameters.
// SPDX-License-Identifier: MIT

#pragma once

#include "jonssonic/core/common/quantities.h"
#include "jonssonic/core/filters/detail/filter_limits.h"
#include "jonssonic/utils/detail/config_utils.h"
#include <jonssonic/utils/math_utils.h>

namespace jnsc {
/// Enumeration of supported damping filter types
enum class DampingType { OnePole, Shelf };

/**
 * @brief DampingDesign class template that calculates coefficients for damping filters based on T60 parameters.
 * @tparam T Sample data type (e.g., float, double)
 * @tparam Type Damping filter type (e.g., OnePole, Shelf)
 */
template <typename T, DampingType Type = DampingType::OnePole>
class DampingDesign;

template <typename T>
class DampingDesign<T, DampingType::OnePole> {
  public:
  private:
};

} // namespace jnsc

// THE FILTERE DESIGNS MUST STORE PARAMS PER CHANNEL AND SECTION
// Otherwise if somebody changes cutoff on channel 0 and then gain on channel 1
// the param states are mixed between channels and sections.