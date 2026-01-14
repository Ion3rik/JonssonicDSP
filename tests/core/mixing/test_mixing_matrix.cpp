#include <cmath>
#include <gtest/gtest.h>
#include <jonssonic/core/mixing/mixing_matrix.h>
#include <vector>

using namespace jnsc;

TEST(MixingMatrixTest, IdentityMixing) {
    MixingMatrix<float, MixingMatrixType::Identity> matrix(4);
    float in[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    float out[4] = {0};
    matrix.mix(in, out);
    for (int i = 0; i < 4; ++i) {
        EXPECT_FLOAT_EQ(out[i], in[i]);
    }
}

TEST(MixingMatrixTest, HadamardMixing) {
    MixingMatrix<float, MixingMatrixType::Hadamard> matrix(4);
    float in[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    float out[4] = {0};
    matrix.mix(in, out);
    // Hadamard(4) transform of [1,2,3,4] is [10, -2, -4, 0] / 2
    float expected[4] = {5.0f, -1.0f, -2.0f, 0.0f};
    for (int i = 0; i < 4; ++i) {
        EXPECT_NEAR(out[i], expected[i], 1e-5f);
    }
}

TEST(MixingMatrixTest, HouseholderMixing) {
    MixingMatrix<float, MixingMatrixType::Householder> matrix(4);
    float in[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    float out[4] = {0};
    matrix.mix(in, out);
    float sum = 1.0f + 2.0f + 3.0f + 4.0f;
    float coeff = 2.0f / 4.0f * sum;
    for (int i = 0; i < 4; ++i) {
        EXPECT_FLOAT_EQ(out[i], in[i] - coeff);
    }
}

TEST(MixingMatrixTest, RandomOrthogonalMixing) {
    MixingMatrix<float, MixingMatrixType::RandomOrthogonal> matrix;
    matrix.resize(3, 42u); // 3x3, seed=42
    float in[3] = {1.0f, 0.0f, 0.0f};
    float out[3] = {0};
    matrix.mix(in, out);
    // Should be orthonormal, so norm of output should be close to 1
    float norm = std::sqrt(out[0] * out[0] + out[1] * out[1] + out[2] * out[2]);
    EXPECT_NEAR(norm, 1.0f, 1e-5f);
}

TEST(MixingMatrixTest, DenseMixing) {
    MixingMatrix<float, MixingMatrixType::Dense> matrix(2, 2);
    matrix.set(0, 0, 1.0f);
    matrix.set(0, 1, 2.0f);
    matrix.set(1, 0, 3.0f);
    matrix.set(1, 1, 4.0f);
    float in[2] = {1.0f, 2.0f};
    float out[2] = {0};
    matrix.mix(in, out);
    EXPECT_FLOAT_EQ(out[0], 1.0f * 1.0f + 2.0f * 2.0f);
    EXPECT_FLOAT_EQ(out[1], 1.0f * 3.0f + 2.0f * 4.0f);
}

TEST(MixingMatrixTest, DecorrelatedSumMixing) {
    MixingMatrix<float, MixingMatrixType::DecorrelatedSum> matrix(2, 2);
    float in[2] = {1.0f, -1.0f};
    float out[2] = {0};
    matrix.mix(in, out);
    // For 2x2, output should be normalized sum and difference
    float norm = 1.0f / std::sqrt(2.0f);
    EXPECT_NEAR(out[0], norm * (1.0f + -1.0f), 1e-5f);
    EXPECT_NEAR(out[1], norm * (1.0f - -1.0f), 1e-5f);
}
