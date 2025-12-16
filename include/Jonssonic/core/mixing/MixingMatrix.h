// Jonssonic - A C++ audio DSP library
// MixingMatrix class header
// SPDX-License-Identifier: MIT

#pragma once
#include <array>
#include <cstddef>
#include <random>
#include <vector>

namespace Jonssonic {

/**
 * @brief Enum for different types of compile-time mixing matrices.
 */
enum class MixingMatrixType {
	Identity,
	Hadamard,
	Householder,
    RandomOrthogonal,
   	Dense,
	DecorrelatedSum
	// Add more as needed
};

/**
 * @brief Runtime sizeable mixing matrix for FDN and other multichannel mixing tasks.
 * @tparam T Sample type (float, double, etc.)
 * @tparam N Input size
 * @tparam Type Matrix type (Identity, Hadamard, etc.)
 */
template <typename T, MixingMatrixType Type = MixingMatrixType::RandomOrthogonal>
class MixingMatrix;


// ===============================================================================
// Identity specialization
// ===============================================================================
template <typename T>
class MixingMatrix<T, MixingMatrixType::Identity> {
public:
	// Constructors and Destructor
    MixingMatrix() = default;
	MixingMatrix(size_t N) { resize(N); }
	~MixingMatrix() = default;

	// Resize function
	void resize(size_t newMatrixSize) {
		size = newMatrixSize;
	}

    // Pointer-based mixing: y = M * x
    void mix(const T* in, T* out) const {
        std::memcpy(out, in, size * sizeof(T));
    }

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
	MixingMatrix() = default;
	/**
	 * @brief Constructor that resizes the matrix to size N (must be power of 2)
	 * @param N Size of the Hadamard matrix (must be power of 2)
	 */
	MixingMatrix(size_t N) { resize(N); }
	~MixingMatrix() = default;

	/**
	 * @brief Resize the Hadamard matrix to size N (must be power of 2)
	 * @param newMatrixSize New size of the Hadamard matrix (must be power of 2)
	 */
	void resize(size_t newMatrixSize) {
		assert((newMatrixSize & (newMatrixSize - 1)) == 0 && "Hadamard matrix size must be a power of 2.");
		size = newMatrixSize; // store size
		norm = static_cast<T>(1) / static_cast<T>(std::sqrt(static_cast<T>(size))); // update normalization factor
	}

	/**
	 * @brief Pointer-based Hadamard mixing: y = M * x
	 * @param in Input array pointer
	 * @param out Output array pointer
	 * @note Input and output arrays must have size equal to matrix size
	 * @note Have to resize the matrix to desired size before use
	 */
    void mix(const T* input, T* output) const {
		// Hadamard transform implementation
        for (size_t i = 0; i < size; ++i) output[i] = input[i];
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
        // Normalization
        for (size_t i = 0; i < size; ++i) output[i] *= norm;
    }


private:
	size_t size;
	T norm;
};

// ===============================================================================
// Householder specialization (reflection across a vector, here: all-ones vector)
// ===============================================================================
template <typename T>
class MixingMatrix<T, MixingMatrixType::Householder> {
public:
	// Constructors and Destructor
	MixingMatrix() = default;

	/**
	 * @brief Constructor that sets the matrix size N
	 * @param N Size of the Householder matrix
	 */	
	MixingMatrix(size_t N)  {resize(N);}

	~MixingMatrix() = default;

	/**
	 * @brief Resize the Householder matrix to size N
	 * @param newMatrixSize New size of the Householder matrix
	 */
	void resize(size_t newMatrixSize) {
		size = newMatrixSize;
	}
	/**
	 * @brief Pointer-based Householder mixing: y = M * x
	 * @param in Input array pointer
	 * @param out Output array pointer
	 * @note Input and output arrays must have size equal to matrix size
	 * @note Have to resize the matrix to desired size before use
	 */
	void mix(const T* in, T* out) const {
		T dotProduct = T(0);
		for (size_t i = 0; i < size; ++i) {
			dotProduct += in[i];
		}
		T coeff = T(2) / static_cast<T>(size) * dotProduct;
		for (size_t i = 0; i < size; ++i) {
			out[i] = in[i] - coeff;
		}
	}

private:
	size_t size;
};

// ===============================================================================
// RandomOrthogonal specialization (using Gram-Schmidt on random vectors)
// ===============================================================================
template <typename T>
class MixingMatrix<T, MixingMatrixType::RandomOrthogonal> {
public:
	MixingMatrix() = default;
	MixingMatrix(unsigned int seed) {
		size = 0;
		generateRandomOrthogonalMatrix(seed);
	}

	/**
	 * @brief Resize the matrix to size N and regenerate random orthogonal matrix
	 * @param newMatrixSize New size of the matrix
	 * @param seed Random seed for matrix generation
	 */
	void resize(size_t newMatrixSize, unsigned int seed = 666u) {
		size = newMatrixSize;
		matrixFlat.resize(size * size, T(0));
		generateRandomOrthogonalMatrix(seed);
	}

	/**
	 * @brief Pointer-based mixing: y = M * x
	 * @param in Input array pointer
	 * @param out Output array pointer
	 * @note Input and output arrays must have size equal to matrix size
	 * @note Have to resize the matrix to desired size before use
	 */
	void mix(const T* in, T* out) const {
		for (size_t i = 0; i < size; ++i) {
			out[i] = T(0);
			for (size_t j = 0; j < size; ++j) {
				out[i] += matrixFlat[i * size + j] * in[j];
			}
		}
	}

	size_t getSize() const { return size; }

private:
	size_t size = 0;
	std::vector<T> matrixFlat;

	/**
	 * @brief Generate a random orthogonal matrix using Gram-Schmidt process (flat storage)
	 * @param seed Random seed for matrix generation
	 */
	void generateRandomOrthogonalMatrix(unsigned int seed) {
		if (size == 0) return;
		std::mt19937 rng(seed);
		std::normal_distribution<T> dist(T(0), T(1));

		// Generate random vectors
		std::vector<std::vector<T>> vectors(size, std::vector<T>(size));
		for (size_t i = 0; i < size; ++i) {
			for (size_t j = 0; j < size; ++j) {
				vectors[i][j] = dist(rng);
			}
		}
		// Gram-Schmidt orthogonalization
		for (size_t i = 0; i < size; ++i) {
			std::vector<T> vi = vectors[i];
			for (size_t j = 0; j < i; ++j) {
				T dot = T(0);
				for (size_t k = 0; k < size; ++k) {
					dot += vi[k] * matrixFlat[j * size + k];
				}
				for (size_t k = 0; k < size; ++k) {
					vi[k] -= dot * matrixFlat[j * size + k];
				}
			}
			// Normalize the vector
			T norm = T(0);
			for (size_t k = 0; k < size; ++k) {
				norm += vi[k] * vi[k];
			}
			norm = std::sqrt(norm);
			for (size_t k = 0; k < size; ++k) {
				matrixFlat[i * size + k] = vi[k] / norm;
			}
		}
	}

};


// ===============================================================================
// Dense specialization
// ===============================================================================
template <typename T>
class MixingMatrix<T, MixingMatrixType::Dense> {
public:
	// Constructors and Destructor
    MixingMatrix() = default;

	/**
	 * @brief Constructor that resizes the matrix to numInputs x numOutputs
	 * @param numInputs Number of input channels
	 * @param numOutputs Number of output channels
	 */
    MixingMatrix(size_t numInputs, size_t numOutputs) {resize(numInputs, numOutputs); }

	~MixingMatrix() = default;

	/**
	 * @brief Resize the matrix to numInputs x numOutputs
	 * @param numInputs Number of input channels
	 * @param numOutputs Number of output channels
	 */
    void resize(size_t newNumInputs, size_t newNumOutputs) {
        numInputs = newNumInputs;
        numOutputs = newNumOutputs;
        matrixFlat.assign(numInputs * numOutputs, T(0));
    }

	/**
	 * @brief Set the matrix element at (out, in) to value
	 * @param out Output channel index
	 * @param in Input channel index
	 * @param value Value to set
	 */
    void set(size_t out, size_t in, T value) {
        matrixFlat[out * numInputs + in] = value;
    }

	/**
	 * @brief Get the matrix element at (out, in)
	 * @param out Output channel index
	 * @param in Input channel index
	 * @return Value at (out, in)
	 */
    T get(size_t out, size_t in) const {
        return matrixFlat[out * numInputs + in];
    }

	/**
	 * @brief Pointer-based mixing: y = M * x
	 * @param in Input array pointer
	 * @param out Output array pointer
	 * @note Input array must have size equal to numInputs
	 * @note Output array must have size equal to numOutputs
	 * @note Have to resize the matrix to desired size before use
	 */
	void mix(const T* in, T* out) const {
		for (size_t i = 0; i < numOutputs; ++i) {
			out[i] = T(0);
			for (size_t j = 0; j < numInputs; ++j) {
				out[i] += matrixFlat[i * numInputs + j] * in[j];
			}
		}
	}
	 
    size_t getNumInputs() const { return numInputs; }
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
	// Constructors and Destructor
	MixingMatrix() = default;
	/**
	 * @brief Constructor that resizes the matrix to numInputs x numOutputs
	 * @param numInputs Number of input channels
	 * @param numOutputs Number of output channels
	 */
	MixingMatrix(size_t numInputs, size_t numOutputs) { resize(numInputs, numOutputs); }
	~MixingMatrix() = default;

	/**
	 * @brief Resize the matrix to numInputs x numOutputs
	 * @param numInputs Number of input channels
	 * @param numOutputs Number of output channels
	 * @note Must be called before calling mix()
	 */
	void resize(size_t numInputs, size_t numOutputs) {
		denseMatrix.resize(numInputs, numOutputs);
		fillDecorrelatedSum();
	}

	size_t getNumInputs() const { return denseMatrix.getNumInputs(); }
	size_t getNumOutputs() const { return denseMatrix.getNumOutputs(); }

	/**
	 * @brief Pointer-based mixing: y = M * x
	 * @param in Input array pointer
	 * @param out Output array pointer
	 * @note Input array must have size equal to numInputs
	 * @note Output array must have size equal to numOutputs
	 * @note Have to resize the matrix to desired size before use
	 */
	void mix(const T* in, T* out) const {
		denseMatrix.mix(in, out);
	}

private:
	MixingMatrix<T, MixingMatrixType::Dense> denseMatrix;

	void fillDecorrelatedSum() {
		size_t numInputs = denseMatrix.getNumInputs();
		size_t numOutputs = denseMatrix.getNumOutputs();
		T norm = static_cast<T>(1) / std::sqrt(static_cast<T>(numInputs));
		for (size_t out = 0; out < numOutputs; ++out) {
			for (size_t in = 0; in < numInputs; ++in) {
				int sign = (__builtin_parityll(static_cast<unsigned long long>(in & out)) ? -1 : 1);
				denseMatrix.set(out, in, norm * static_cast<T>(sign));
			}
		}
	}
};

} // namespace Jonssonic
