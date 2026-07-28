#include "Common.h"

#include <benchmark/benchmark.h>

#if defined(_MSC_VER) && defined(MATH_BENCHMARK_DISABLE_AUTO_VECTORIZATION)
#define MATH_BENCHMARK_NO_AUTO_VECTORIZE __pragma(loop(no_vector))
#else
#define MATH_BENCHMARK_NO_AUTO_VECTORIZE
#endif

using namespace Common;
using namespace Common::MathBenchmark;

// These workloads intentionally expose a contiguous batch to the optimizer. They measure the code shipped by each
// backend after all Release optimizations, including inlining, loop vectorization and SLP vectorization.
template <MathBackend B>
static void VecAddThroughput(benchmark::State& state)
{
    const auto a = MakeRandomVecs<B>(batchSize);
    const auto b = MakeRandomVecs<B>(batchSize);
    std::vector<Vec<float, 4, B>> output(batchSize);
    for (auto _ : state) {
        MATH_BENCHMARK_NO_AUTO_VECTORIZE
        for (int i = 0; i < batchSize; i++) {
            output[i] = a[i] + b[i];
        }
        benchmark::DoNotOptimize(output.data());
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * batchSize);
}
BENCHMARK(VecAddThroughput<MathBackend::scalar>);
BENCHMARK(VecAddThroughput<MathBackend::simd>);

template <MathBackend B>
static void VecDotThroughput(benchmark::State& state)
{
    const auto a = MakeRandomVecs<B>(batchSize);
    const auto b = MakeRandomVecs<B>(batchSize);
    for (auto _ : state) {
        float sum = 0.0f;
        MATH_BENCHMARK_NO_AUTO_VECTORIZE
        for (int i = 0; i < batchSize; i++) {
            sum += a[i].Dot(b[i]);
        }
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * batchSize);
}
BENCHMARK(VecDotThroughput<MathBackend::scalar>);
BENCHMARK(VecDotThroughput<MathBackend::simd>);

template <MathBackend B>
static void MatMulThroughput(benchmark::State& state)
{
    const auto a = MakeRandomMats<B>(batchSize);
    const auto b = MakeRandomMats<B>(batchSize);
    std::vector<Mat<float, 4, 4, B>> output(batchSize);
    for (auto _ : state) {
        MATH_BENCHMARK_NO_AUTO_VECTORIZE
        for (int i = 0; i < batchSize; i++) {
            output[i] = a[i] * b[i];
        }
        benchmark::DoNotOptimize(output.data());
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * batchSize);
}
BENCHMARK(MatMulThroughput<MathBackend::scalar>);
BENCHMARK(MatMulThroughput<MathBackend::simd>);

template <MathBackend B>
static void QuatMulThroughput(benchmark::State& state)
{
    const auto a = MakeRandomQuats<B>(batchSize);
    const auto b = MakeRandomQuats<B>(batchSize);
    std::vector<Quaternion<float, B>> output(batchSize);
    for (auto _ : state) {
        MATH_BENCHMARK_NO_AUTO_VECTORIZE
        for (int i = 0; i < batchSize; i++) {
            output[i] = a[i] * b[i];
        }
        benchmark::DoNotOptimize(output.data());
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * batchSize);
}
BENCHMARK(QuatMulThroughput<MathBackend::scalar>);
BENCHMARK(QuatMulThroughput<MathBackend::simd>);

template <MathBackend B>
static void Mat4InverseThroughput(benchmark::State& state)
{
    const auto input = MakeRandomMats<B>(batchSize);
    std::vector<Mat<float, 4, 4, B>> output(batchSize);
    for (auto _ : state) {
        MATH_BENCHMARK_NO_AUTO_VECTORIZE
        for (int i = 0; i < batchSize; i++) {
            output[i] = input[i].Inverse();
        }
        benchmark::DoNotOptimize(output.data());
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * batchSize);
}
BENCHMARK(Mat4InverseThroughput<MathBackend::scalar>);
BENCHMARK(Mat4InverseThroughput<MathBackend::simd>);

template <MathBackend B>
static void Mat4InverseUncheckedThroughput(benchmark::State& state)
{
    const auto input = MakeRandomMats<B>(batchSize);
    std::vector<Mat<float, 4, 4, B>> output(batchSize);
    for (auto _ : state) {
        MATH_BENCHMARK_NO_AUTO_VECTORIZE
        for (int i = 0; i < batchSize; i++) {
            output[i] = input[i].InverseUnchecked();
        }
        benchmark::DoNotOptimize(output.data());
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * batchSize);
}
BENCHMARK(Mat4InverseUncheckedThroughput<MathBackend::scalar>);
BENCHMARK(Mat4InverseUncheckedThroughput<MathBackend::simd>);

template <MathBackend B>
static void Mat3MulThroughput(benchmark::State& state)
{
    const auto a = MakeRandomMat3s<B>(batchSize);
    const auto b = MakeRandomMat3s<B>(batchSize);
    std::vector<Mat<float, 3, 3, B>> output(batchSize);
    for (auto _ : state) {
        MATH_BENCHMARK_NO_AUTO_VECTORIZE
        for (int i = 0; i < batchSize; i++) {
            output[i] = a[i] * b[i];
        }
        benchmark::DoNotOptimize(output.data());
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * batchSize);
}
BENCHMARK(Mat3MulThroughput<MathBackend::scalar>);
BENCHMARK(Mat3MulThroughput<MathBackend::simd>);

template <MathBackend B>
static void Mat3MulVecThroughput(benchmark::State& state)
{
    const auto matrices = MakeRandomMat3s<B>(batchSize);
    const auto vectors = MakeRandomVec3s<B>(batchSize);
    std::vector<Vec<float, 3, B>> output(batchSize);
    for (auto _ : state) {
        MATH_BENCHMARK_NO_AUTO_VECTORIZE
        for (int i = 0; i < batchSize; i++) {
            output[i] = matrices[i] * vectors[i];
        }
        benchmark::DoNotOptimize(output.data());
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * batchSize);
}
BENCHMARK(Mat3MulVecThroughput<MathBackend::scalar>);
BENCHMARK(Mat3MulVecThroughput<MathBackend::simd>);

template <MathBackend B>
static void VecNormalizeThroughput(benchmark::State& state)
{
    const auto input = MakeRandomVecs<B>(batchSize);
    std::vector<Vec<float, 4, B>> output(batchSize);
    for (auto _ : state) {
        MATH_BENCHMARK_NO_AUTO_VECTORIZE
        for (int i = 0; i < batchSize; i++) {
            output[i] = input[i].Normalized();
        }
        benchmark::DoNotOptimize(output.data());
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * batchSize);
}
BENCHMARK(VecNormalizeThroughput<MathBackend::scalar>);
BENCHMARK(VecNormalizeThroughput<MathBackend::simd>);
