#pragma once

namespace jnsc::detail {

/**
 * @brief State Variable Filter TPT Engine class implementing a multi-channel, multi-section state variable filter.
 * @tparam T Sample data type (e.g., float, double)
 */
template <typename T>
class SVFTPTEngine {
  public:
    SVFTPTEngine() = default;
    ~SVFTPTEngine() = default;

    // No copy or move semantics
    SVFTPTEngine(const SVFTPTEngine&) = delete;
    SVFTPTEngine& operator=(const SVFTPTEngine&) = delete;
    SVFTPTEngine(SVFTPTEngine&&) = delete;
    SVFTPTEngine& operator=(SVFTPTEngine&&) = delete;

    // TODO: Add state and coefficient buffers
    // TODO: Add prepare, reset, processSample, setCoeffs, getCoeffs methods
};

} // namespace jnsc::detail
