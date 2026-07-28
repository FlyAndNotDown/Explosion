#include "Common.h"

#include <benchmark/benchmark.h>

using namespace Common;
using namespace Common::MathBenchmark;

namespace {
    template <MathBackend B>
    Mat<float, 4, 4, B> MakeStableMat4()
    {
        return Mat<float, 4, 4, B>(
            0.0f, -1.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            2.0f, 3.0f, 4.0f, 1.0f);
    }

    template <MathBackend B>
    Mat<float, 3, 3, B> MakeStableMat3()
    {
        return Mat<float, 3, 3, B>(
            0.0f, -1.0f, 0.0f,
            1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f);
    }
}

// Each iteration consumes the previous iteration's result. This prevents cross-object vectorization and measures the
// latency of the backend operation rather than bulk throughput. The final value is observed once to keep barriers out
// of the timed dependency chain.
template <MathBackend B>
static void VecAddLatency(benchmark::State& state)
{
    auto value = MakeRandomVecs<B>(1)[0];
    const auto rhs = MakeRandomVecs<B>(1)[0];
    for (auto _ : state) {
        value = value + rhs;
    }
    benchmark::DoNotOptimize(value);
}
BENCHMARK(VecAddLatency<MathBackend::scalar>);
BENCHMARK(VecAddLatency<MathBackend::simd>);

template <MathBackend B>
static void VecDotLatency(benchmark::State& state)
{
    auto value = MakeRandomVecs<B>(1)[0];
    const auto rhs = MakeRandomVecs<B>(1)[0];
    for (auto _ : state) {
        value.x = value.Dot(rhs) * 0.125f;
    }
    benchmark::DoNotOptimize(value);
}
BENCHMARK(VecDotLatency<MathBackend::scalar>);
BENCHMARK(VecDotLatency<MathBackend::simd>);

template <MathBackend B>
static void MatMulLatency(benchmark::State& state)
{
    auto value = MakeStableMat4<B>();
    auto rhs = MakeStableMat4<B>();
    benchmark::DoNotOptimize(value);
    benchmark::DoNotOptimize(rhs);
    for (auto _ : state) {
        value = value * rhs;
    }
    benchmark::DoNotOptimize(value);
}
BENCHMARK(MatMulLatency<MathBackend::scalar>);
BENCHMARK(MatMulLatency<MathBackend::simd>);

template <MathBackend B>
static void QuatMulLatency(benchmark::State& state)
{
    Quaternion<float, B> value(1.0f, 0.0f, 0.0f, 0.0f);
    Quaternion<float, B> rhs(0.0f, 0.0f, 0.0f, 1.0f);
    benchmark::DoNotOptimize(value);
    benchmark::DoNotOptimize(rhs);
    for (auto _ : state) {
        value = value * rhs;
    }
    benchmark::DoNotOptimize(value);
}
BENCHMARK(QuatMulLatency<MathBackend::scalar>);
BENCHMARK(QuatMulLatency<MathBackend::simd>);

template <MathBackend B>
static void Mat4InverseLatency(benchmark::State& state)
{
    auto value = MakeStableMat4<B>();
    benchmark::DoNotOptimize(value);
    for (auto _ : state) {
        value = value.Inverse();
    }
    benchmark::DoNotOptimize(value);
}
BENCHMARK(Mat4InverseLatency<MathBackend::scalar>);
BENCHMARK(Mat4InverseLatency<MathBackend::simd>);

template <MathBackend B>
static void Mat4InverseUncheckedLatency(benchmark::State& state)
{
    auto value = MakeStableMat4<B>();
    benchmark::DoNotOptimize(value);
    for (auto _ : state) {
        value = value.InverseUnchecked();
    }
    benchmark::DoNotOptimize(value);
}
BENCHMARK(Mat4InverseUncheckedLatency<MathBackend::scalar>);
BENCHMARK(Mat4InverseUncheckedLatency<MathBackend::simd>);

template <MathBackend B>
static void Mat3MulLatency(benchmark::State& state)
{
    auto value = MakeStableMat3<B>();
    auto rhs = MakeStableMat3<B>();
    benchmark::DoNotOptimize(value);
    benchmark::DoNotOptimize(rhs);
    for (auto _ : state) {
        value = value * rhs;
    }
    benchmark::DoNotOptimize(value);
}
BENCHMARK(Mat3MulLatency<MathBackend::scalar>);
BENCHMARK(Mat3MulLatency<MathBackend::simd>);

template <MathBackend B>
static void Mat3MulVecLatency(benchmark::State& state)
{
    auto matrix = MakeStableMat3<B>();
    auto value = MakeRandomVec3s<B>(1)[0];
    benchmark::DoNotOptimize(matrix);
    benchmark::DoNotOptimize(value);
    for (auto _ : state) {
        value = matrix * value;
    }
    benchmark::DoNotOptimize(value);
}
BENCHMARK(Mat3MulVecLatency<MathBackend::scalar>);
BENCHMARK(Mat3MulVecLatency<MathBackend::simd>);

template <MathBackend B>
static void VecNormalizeLatency(benchmark::State& state)
{
    auto value = MakeRandomVecs<B>(1)[0];
    for (auto _ : state) {
        value = value.Normalized();
    }
    benchmark::DoNotOptimize(value);
}
BENCHMARK(VecNormalizeLatency<MathBackend::scalar>);
BENCHMARK(VecNormalizeLatency<MathBackend::simd>);
