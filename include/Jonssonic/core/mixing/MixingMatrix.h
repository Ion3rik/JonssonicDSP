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
    RandomOrthogonal, // BROKEN!!! DO NOT USE
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
		// BROKEN
		assert(true && "RandomOrthogonal MixingMatrix is broken atm. Do not use.");
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
		0.080867542781482893,
-0.052632053340117668,
-0.29262019295744679,
0.29043533485586936,
0.23075838254909337,
0.29724300903585521,
0.29182900027460518,
0.50455715857107808,
0.0047125921780683264,
-0.36000180798423331,
-0.2863126618964838,
0.21687117061059843,
0.24491882674665155,
0.083380167085796125,
-0.084375659617646787,
-0.08171796648898301,
0.27582450919222135,
0.31333265731561466,
-0.12787042130282811,
0.21412355364906352,
0.068906928695033648,
0.11084638045565559,
-0.27155042214368996,
-0.28466165419139033,
-0.38113546119345326,
-0.055191313781076744,
-0.32892169665066173,
0.074078363503325514,
-0.21485317409149937,
0.40269684655461646,
0.22103777806777461,
0.26937896681621454,
-0.33974067175323236,
0.44974558970764345,
-0.5006805934798384,
-0.17282177681193875,
-0.34076597685593552,
0.1331433641981721,
-0.24386456090000586,
0.15523309036765079,
0.2722926013516595,
-0.049298563964964112,
-0.063676700957388019,
-0.12328126741893632,
0.007318384891026904,
-0.12575162444460702,
-0.072444628020564727,
0.25961328588650961,
0.12967472389847284,
0.33197558085994033,
0.45034334843556856,
-0.029117065027514776,
0.081386410988822683,
0.48038244306140204,
-0.25060667036909057,
-0.056482321520533188,
0.23277341343701827,
-0.17740996481628979,
0.2871445398994743,
0.27867751219956322,
0.28056678828145076,
-0.11439290580761327,
0.16804166404868104,
0.01106294284924798,
0.047943717916827597,
0.1607445408721784,
0.12989660092326014,
-0.37775970526124297,
-0.070796681564988639,
-0.13784257864487151,
0.072700034699126823,
0.080692608686552272,
-0.51074583134061535,
-0.49278159011531103,
0.10084525406118018,
-0.38391469305930553,
0.32525424220771787,
0.027853646092315939,
-0.079883956011922871,
0.047695714554942099,
-0.19668216907506245,
-0.26084107267395673,
-0.24721048100037402,
-0.25192861217560603,
0.51407982863290236,
0.090462675629597186,
-0.27923347029026713,
-0.35829805127674974,
0.015524152011677842,
0.049112023837955307,
-0.27754030663744722,
-0.08806483156623289,
0.39674613452244722,
-0.17072783859381921,
0.11664509899817055,
-0.047833309091576508,
-0.065214179618853563,
0.20142464376088476,
0.41754699367305143,
0.017516843825621835,
0.026412598863752454,
0.26677097218862061,
0.22082508829834951,
-0.073361412922278754,
0.080518657233375202,
0.057864738714106266,
-0.5949095577133886,
-0.36415182505908261,
-0.20165825837203405,
-0.26906409629232997,
-0.20792249999257889,
-0.043174942063277717,
0.051532252332153923,
0.40681332948154303,
-0.27812234369452993,
0.40540959089648809,
0.16915996513521253,
-0.029846847980990718,
0.31035927338883146,
-0.22868662567529952,
-0.19010609063534528,
0.14925866054557554,
0.30270835068874041,
-0.18264141970686334,
0.098814167951940896,
-0.40714365518172957,
0.13807728880369657,
-0.18441189590878851,
0.5382069059617306,
-0.011492172595382565,
-0.11503655028990879,
-0.40725273594289901,
0.066458406431516293,
0.064409462232147899,
-0.039875703293769986,
0.39492046106998946,
-0.13798218924551453,
0.31391077538004042,
-0.069197635208623001,
0.039497708014870028,
-0.14270158902742586,
-0.38733873855168161,
0.24516210649257877,
0.10280357558950427,
0.41653571703099412,
0.16018420158823365,
-0.067331373782618928,
-0.040076051830932713,
0.32436833980094099,
-0.27798506409912843,
0.008035272659254647,
0.00071004944125274347,
0.59212011767093153,
-0.16954230536483117,
0.059218166249798546,
-0.3992211444870376,
-0.045856818465246425,
0.26000921429115914,
0.0015721227539755519,
-0.014091745962447368,
-0.20302903386550172,
0.23910261959618556,
0.19691192171308855,
-0.0068177180310162134,
-0.065460961363561221,
-0.22499356658799619,
0.1662980295873514,
0.24261809266004414,
0.041635540573334649,
0.42451362152833361,
-0.21047421367853111,
-0.03231632058854176,
0.44045403905070796,
0.31695026539572874,
0.44586911045930455,
-0.028582849832360299,
0.45646606459771316,
-0.19473570615640259,
-0.067178944141729424,
0.18373059093363334,
-0.49409309752169434,
0.0075028121146003832,
0.028945497127288243,
-0.26170486059500164,
0.11602523317889751,
0.11609637379394279,
-0.15316283739804878,
-0.038410052116451272,
0.51007015674997791,
-0.087408138994057194,
-0.21868477613009382,
0.18554258400773765,
0.1091040401766416,
0.047844447096867408,
-0.21097277861254918,
-0.36237206062954663,
-0.31164206628307106,
0.213936996799556,
0.2437530567594659,
-0.28019655934148718,
0.077124340910731587,
-0.11071547539274039,
-0.095717235915935353,
0.10836442224270953,
-0.096544045309883156,
0.18378368690542712,
0.22056241780243729,
-0.63112044723617944,
-0.0094837349731846567,
-0.20039392710136025,
-0.078441536904777218,
0.021605991380427819,
0.021493139400719397,
0.59677578678954335,
-0.003025413361924534,
0.1089343786520437,
-0.099393609457313112,
0.32906123217925193,
0.30786767570832391,
-0.48655712605827678,
0.0095351480858291184,
0.35944096080101712,
-0.073556052746493017,
0.016533282305880692,
0.10750052979193156,
0.20141216862302133,
0.015627712053748438,
0.11584638188663485,
0.01149655223998936,
-0.13246761034130441,
-0.53458980340303885,
0.21258342729401031,
-0.14288344421593924,
0.1970805191418403,
-0.073544153535043982,
-0.0039420850953249291,
0.097938233159634222,
0.038519093085331836,
-0.41625472503614425,
-0.58011209994920998,
-0.030827811984756395,
-0.28762997248904881,
0.053736955330491575,
0.34897571217977746,
-0.2547443351873464,
-0.0079278251036647967,
-0.3392946711684397,
0.16015560901206227,
0.039715514468194886,
-0.28674224978266244,
-0.071933043368837546,
-0.34864885764844922,
-0.076236502225573113,
-0.20906174547266415,
0.54239902318368804,
-0.1779729489622296
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
