// Jonssonic - A C++ audio DSP library
// MixingMatrix class header
// SPDX-License-Identifier: MIT

#pragma once
#include "../../utils/MathUtils.h"
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
	MixingMatrix(size_t newMatrixSize, unsigned int seed = 666u) {
		resize(newMatrixSize, seed);
	}

	/**
	 * @brief Resize the matrix to size N and regenerate random orthogonal matrix
	 * @param newMatrixSize New size of the matrix
	 * @param seed Random seed for matrix generation
	 */
	void resize(size_t newMatrixSize, unsigned int seed = 666u) {
		size = newMatrixSize;
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

	// Fixed pre generated orthogonal matrix for sizes 4, 8, 16, 32
	std::vector<T> matrixFlat{
		0.40599383360672014,
		0.37154874370525376,
		0.21472970333933944,
		0.18950257276531443,
		-0.26730549715060753,
		0.51548487959734823,
		-0.089031912577836797,
		-0.27054018574625283,
		0.055906937799022303,
		-0.10155627785557314,
		-0.19270108709191086,
		-0.32175047301205945,
		0.19854718123321652,
		0.053590768328794558,
		0.019756270625500578,
		0.0050300210335165064,
		-0.45307205373730186,
		-0.26209084136943789,
		-0.26978692998711012,
		-0.049800171656634785,
		-0.25956525237916306,
		0.11387938423940139,
		-0.15890021912462912,
		-0.19218497561188877,
		0.34530648943859477,
		-0.05258273790935783,
		0.14775921891662791,
		-0.31200379781309423,
		0.3449379413110959,
		-0.11849475405478617,
		-0.12665815729095409,
		-0.34350535645830138,
		0.44727245876271154,
		0.13409855246334165,
		-0.069875874732039434,
		0.17331796599233115,
		0.42508723772269003,
		-0.35819404691834916,
		0.049842227643152756,
		-0.074370800875839638,
		0.33704645556514501,
		-0.14100278469493821,
		0.33917683482892852,
		-0.16019062578261295,
		0.16501825878779278,
		-0.25519077598510775,
		-0.23462602997582158,
		-0.091863920312112074,
		0.067722191074564217,
		-0.29759369878546937,
		-0.44272663060296852,
		0.26628979215631671,
		0.26867492794545783,
		0.3420669352737748,
		0.15097684607711673,
		0.26690465238139871,
		0.061996945225829783,
		-0.17104268774264886,
		-0.19213157981079407,
		-0.35113758313602256,
		-0.34704702208209415,
		0.15528419525109452,
		-0.097403777095230251,
		0.096709796180139862,
		0.20063267995756304,
		-0.40445652116144898,
		0.094547993994618149,
		0.092335615056066661,
		-0.090202430001520492,
		-0.28290011010475824,
		-0.01866222871768233,
		-0.047050066582267722,
		-0.56220653726416214,
		-0.32882416411072912,
		-0.069376088281892623,
		-0.14848468602095785,
		0.32172950854499727,
		0.28617730128679236,
		-0.16020654125676781,
		-0.16071589115483725,
		-0.33386547294593721,
		0.26092993489001443,
		0.27594558074414599,
		0.12861943599282513,
		-0.023865421777860423,
		-0.32258716667761367,
		0.15746368732946739,
		-0.19510789784921295,
		0.22386897094655581,
		-0.135044255845022,
		0.028845784236950219,
		-0.27679600657168429,
		-0.30720347445451557,
		0.55346224520264242,
		-0.11715687190580511,
		0.0066339419031027714,
		-0.11837301648586024,
		-0.013479459900340166,
		0.021044178740585253,
		0.33506138113447448,
		-0.37924671050626035,
		-0.20581982428665138,
		0.52042501924685969,
		-0.018065679402548999,
		-0.13910642351907795,
		0.14930096107334459,
		-0.099059124325550307,
		-0.19981272198731928,
		-0.0103655532032676,
		-0.48328570730750658,
		-0.13257253191240595,
		0.27039139370963083,
		-0.055785365533381787,
		-0.024115154728682035,
		-0.0069361533941033147,
		-0.040372449176940113,
		-0.1204457711943229,
		0.14129520271098828,
		-0.22117268217898794,
		-0.35694772747383663,
		-0.16728479085601947,
		-0.50855887832000768,
		0.23965244440788411,
		0.20047732137257693,
		-0.4609145260213322,
		-0.28508016390084706,
		-0.3021498848535214,
		0.12074608192444712,
		0.084805424158625381,
		0.30423339831163321,
		-0.45297128314851232,
		0.34130104662215249,
		-0.18476387366819161,
		-0.1533689026908249,
		-0.099224473473959754,
		-0.093385620520863771,
		-0.27120316654693755,
		0.18036269921725223,
		0.11088159964998714,
		0.05466197421803394,
		-0.25691804166326598,
		0.039885005710093936,
		0.23104414139409532,
		-0.50991273915763402,
		-0.33507646286393511,
		0.15402550553439232,
		0.20389595372182195,
		0.32707627901932601,
		0.39749132987095176,
		0.3480797932139591,
		0.26080731199074203,
		-0.001123362233907239,
		-0.19530493971998727,
		0.014710560514961134,
		0.0032211091119679312,
		0.2932417882937583,
		0.19901272428428479,
		-0.029228683719465364,
		-0.31037608093223823,
		-0.3272790945538821,
		0.094619504149517308,
		0.12400649740189142,
		-0.42682160172107975,
		-0.46059385120946428,
		0.098398823657411205,
		0.051675158847042005,
		0.37434511411875149,
		-0.48024494319813421,
		-0.14394232375902796,
		0.20940087520537906,
		-0.057797126683866898,
		0.027572588856906,
		0.093242348030329089,
		0.19823734788261088,
		-0.26108766830559782,
		0.11574202080405274,
		-0.24332198530654101,
		0.37646175969249734,
		-0.34474208687037022,
		0.208576271477657,
		0.0097039435447998142,
		-0.056140463323179637,
		-0.30987482750041689,
		0.19373911407661989,
		-0.16016361570265777,
		-0.12197384232005895,
		0.13866417219905186,
		0.022517785606969442,
		0.36271862942632843,
		0.13822378567125643,
		-0.090302862489617677,
		0.52993073597480722,
		0.013279079173811355,
		0.30211219259350064,
		0.06231798479204434,
		-0.43695798156567256,
		-0.15336235662720166,
		0.11583323382772312,
		0.16392066952444373,
		0.5391776411815653,
		-0.214219927273851,
		-0.093339949936584066,
		0.33542514373827648,
		-0.31523162862051934,
		-0.075323464307916643,
		-0.052262832170235723,
		-0.19897657757582674,
		-0.21150881439765964,
		0.130875950406699,
		0.092435793432464247,
		-0.19581099896942294,
		0.001894024059076181,
		-0.35127684816367677,
		-0.05667540014147078,
		0.3165920426494141,
		0.22325537174717536,
		0.35332251975661205,
		-0.44819036675697888,
		-0.20648468149663451,
		0.48865770836511641,
		0.12494019998923285,
		0.13465411331175725,
		-0.056940987346504768,
		-0.12321858719269331,
		0.065614927048668115,
		-0.21571107553529009,
		0.032513568264779219,
		0.14679713222402452,
		-0.093268504025938101,
		0.24262456527551407,
		0.3392603897139107,
		-0.092430260341054699,
		0.0051123257221629519,
		-0.024617109599529445,
		0.70277141329336557,
		0.093431460092685506,
		0.081116404710883869,
		0.22554737339177375,
		0.38379002119537126,
		0.16539262484270545,
		0.21719845144629252,
		-0.1858828657823143,
		0.059811914279863751,
		0.17347374915090141,
		-0.28934234865250413,
		0.059776411566256973,
		-0.20105320909273519,
		0.13818087937915016,
		0.13358717658911373,
		0.47836429459679308,
		0.17966617550570338,
		0.18509150003044131,
		-0.098768632179518451,
		0.24127595785598299,
		-0.59548619997589969,
		0.04475878400326292
		};


	

		

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
				   int sign = Jonssonic::parity_sign(static_cast<uint64_t>(in & out));
				denseMatrix.set(out, in, norm * static_cast<T>(sign));
			}
		}
	}
};

} // namespace Jonssonic
