// Jonssonic - A C++ audio DSP library
// MixingMatrix class header
// SPDX-License-Identifier: MIT

#pragma once
#include <cstddef>
#include <jonssonic/utils/math_utils.h>
#include <random>
#include <vector>

namespace jnsc {

/**
 * @brief Enum for different types of compile-time mixing matrices.
 */
enum class MixingMatrixType { Identity, Hadamard, Householder, Dense, DecorrelatedSum };

/**
 * @brief Runtime sizeable mixing matrix for FDN and other multichannel mixing tasks.
 * @tparam T Sample type (float, double, etc.)
 * @tparam N Input size
 * @tparam Type Matrix type (Identity, Hadamard, etc.)
 */
template <typename T, MixingMatrixType Type = MixingMatrixType::Identity>
class MixingMatrix;

// ===============================================================================
// Identity specialization
// ===============================================================================
template <typename T>
class MixingMatrix<T, MixingMatrixType::Identity> {
  public:
    /// Default constructor
    MixingMatrix() = default;

    /// Constructor that resizes the matrix to size N
    MixingMatrix(size_t N) { resize(N); }

    /// Default destructor
    ~MixingMatrix() = default;

    /**
     * @brief Resize the identity matrix.
     * @param N New size of the identity matrix.
     */
    void resize(size_t N) { size = N; }

    /**
     * @brief Apply identity mixing: y = M * x.
     * @param in Input array pointer.
     * @param out Output array pointer.
     * @note Input and output arrays must have size equal to matrix size.
     * @note Have to call @ref resize before mixing.
     */
    void mix(const T* in, T* out) const { std::memcpy(out, in, size * sizeof(T)); }

    /// Get the size of the identity matrix.
    size_t getSize() const { return size; }

  private:
    size_t size;
};

// ===============================================================================
// Hadamard specialization (Sylvester's construction, N must be power of 2)
// ===============================================================================
template <typename T>
class MixingMatrix<T, MixingMatrixType::Hadamard> {
  public:
    /// Default constructor
    MixingMatrix() = default;
    /**
     * @brief Constructor that resizes the matrix.
     * @param N Size of the Hadamard matrix.
     * @note N must be a power of 2.
     */
    MixingMatrix(size_t N) { resize(N); }

    /// Default destructor
    ~MixingMatrix() = default;

    /**
     * @brief Resize the Hadamard matrix.
     * @param N New size of the Hadamard matrix.
     * @note N must be a power of 2.
     */
    void resize(size_t N) {
        assert((N & (N - 1)) == 0 && "Hadamard matrix size must be a power of 2.");
        size = N;
        norm = static_cast<T>(1) / static_cast<T>(std::sqrt(static_cast<T>(size)));
    }

    /**
     * @brief Apply Hadamard mixing: y = M * x.
     * @param in Input array pointer.
     * @param out Output array pointer.
     * @note Input and output arrays must have size equal to matrix size.
     * @note Have to call @ref resize before mixing.
     */
    void mix(const T* input, T* output) const {
        // Hadamard transform using the Fast Walsh-Hadamard Transform (FWHT) algorithm
        for (size_t i = 0; i < size; ++i)
            output[i] = input[i];
        for (size_t len = 1; len < size; len <<= 1) {
            for (size_t i = 0; i < size; i += (len << 1)) {
                for (size_t j = 0; j < len; ++j) {
                    T u = output[i + j];
                    T v = output[i + j + len];
                    output[i + j] = u + v;
                    output[i + j + len] = u - v;
                }
            }
        }
        // Apply normalization
        for (size_t i = 0; i < size; ++i)
            output[i] *= norm;
    }

  private:
    size_t size;
    T norm; // Normalization factor 1/sqrt(N)
};

// ===============================================================================
// Householder specialization (reflection across a vector, here: all-ones vector)
// ===============================================================================
template <typename T>
class MixingMatrix<T, MixingMatrixType::Householder> {
  public:
    /// Default constructor
    MixingMatrix() = default;

    /**
     * @brief Constructor that sets the matrix size.
     * @param N Size of the Householder matrix.
     */
    MixingMatrix(size_t N) { resize(N); }

    /// Default destructor
    ~MixingMatrix() = default;

    /**
     * @brief Resize the Householder matrix.
     * @param N New size of the Householder matrix.
     */
    void resize(size_t N) { size = N; }

    /**
     * @brief Apply Householder mixing: y = M * x.
     * @param in Input array pointer
     * @param out Output array pointer
     * @note Input and output arrays must have size equal to matrix size.
     * @note Have to call @ref resize before mixing.
     */
    void mix(const T* in, T* out) const {
        T inputSum = T(0);
        for (size_t i = 0; i < size; ++i) {
            inputSum += in[i];
        }
        T coeff = T(2) / static_cast<T>(size) * inputSum;
        for (size_t i = 0; i < size; ++i) {
            out[i] = in[i] - coeff;
        }
    }

  private:
    size_t size;
};

// ===============================================================================
// Dense specialization
// ===============================================================================
template <typename T>
class MixingMatrix<T, MixingMatrixType::Dense> {
  public:
    /// Default constructor.
    MixingMatrix() = default;

    /**
     * @brief Constructor that resizes the matrix to numInputs x numOutputs.
     * @param numInputs Number of input channels.
     * @param numOutputs Number of output channels.
     */
    MixingMatrix(size_t numInputs, size_t numOutputs) { resize(numInputs, numOutputs); }

    /// Default destructor.
    ~MixingMatrix() = default;

    /**
     * @brief Resize the matrix to numInputs x numOutputs.
     * @param numInputs Number of input channels.
     * @param numOutputs Number of output channels.
     */
    void resize(size_t newNumInputs, size_t newNumOutputs) {
        numInputs = newNumInputs;
        numOutputs = newNumOutputs;
        matrixFlat.assign(numInputs * numOutputs, T(0));
    }

    /**
     * @brief Set the matrix element at (out, in) to value
     * @param out Output channel index.
     * @param in Input channel index.
     * @param value Value to set.
     */
    void set(size_t out, size_t in, T value) { matrixFlat[out * numInputs + in] = value; }

    /**
     * @brief Get the matrix element at (out, in).
     * @param out Output channel index.
     * @param in Input channel index.
     * @return Value at (out, in).
     */
    T get(size_t out, size_t in) const { return matrixFlat[out * numInputs + in]; }

    /**
     * @brief Apply dense mixing: y = M * x.
     * @param in Input array pointer.
     * @param out Output array pointer.
     * @note Input array must have size equal to numInputs.
     * @note Output array must have size equal to numOutputs.
     * @note Have to call @ref resize before mixing.
     */
    void mix(const T* in, T* out) const {
        for (size_t i = 0; i < numOutputs; ++i) {
            out[i] = T(0);
            for (size_t j = 0; j < numInputs; ++j) {
                out[i] += matrixFlat[i * numInputs + j] * in[j];
            }
        }
    }

    /// Get number of input channels.
    size_t getNumInputs() const { return numInputs; }

    /// Get number of output channels.
    size_t getNumOutputs() const { return numOutputs; }

  private:
    size_t numInputs = 0;
    size_t numOutputs = 0;
    std::vector<T> matrixFlat;
};

// ===============================================================================
// DecorrelatedSum specialization
// ===============================================================================
/**
 * @brief Mixing matrix that implements a decorrelated sum mixing strategy.
 * Each output channel is a normalized sum of all input channels with alternating signs.
 * @note Uses the Dense mixing matrix internally.
 */
template <typename T>
class MixingMatrix<T, MixingMatrixType::DecorrelatedSum> {
  public:
    /// Default constructor.
    MixingMatrix() = default;
    /**
     * @brief Constructor that resizes the matrix to numInputs x numOutputs.
     * @param numInputs Number of input channels.
     * @param numOutputs Number of output channels.
     */
    MixingMatrix(size_t numInputs, size_t numOutputs) { resize(numInputs, numOutputs); }

    /// Default destructor.
    ~MixingMatrix() = default;

    /**
     * @brief Resize the matrix to numInputs x numOutputs.
     * @param numInputs Number of input channels.
     * @param numOutputs Number of output channels.
     * @note Must be called before calling @ref mix.
     */
    void resize(size_t numInputs, size_t numOutputs) {
        denseMatrix.resize(numInputs, numOutputs);
        fillDecorrelatedSum();
    }

    /**
     * @brief Apply decorrelated sum mixing: y = M * x.
     * @param in Input array pointer.
     * @param out Output array pointer.
     * @note Input array must have size equal to numInputs.
     * @note Output array must have size equal to numOutputs.
     * @note Have to resize the matrix to desired size before use.
     */
    void mix(const T* in, T* out) const { denseMatrix.mix(in, out); }

    /// Get number of input channels.
    size_t getNumInputs() const { return denseMatrix.getNumInputs(); }

    /// Get number of output channels.
    size_t getNumOutputs() const { return denseMatrix.getNumOutputs(); }

  private:
    MixingMatrix<T, MixingMatrixType::Dense> denseMatrix;

    // Help function to fill the dense matrix with random +/- 1 values normalized by sqrt(numInputs).
    void fillDecorrelatedSum() {
        size_t numInputs = denseMatrix.getNumInputs();
        size_t numOutputs = denseMatrix.getNumOutputs();
        T norm = static_cast<T>(1) / std::sqrt(static_cast<T>(numInputs));
        for (size_t out = 0; out < numOutputs; ++out) {
            for (size_t in = 0; in < numInputs; ++in) {
                int sign = utils::parity_sign(static_cast<uint64_t>(in & out));
                denseMatrix.set(out, in, norm * static_cast<T>(sign));
            }
        }
    }
};

} // namespace jnsc
