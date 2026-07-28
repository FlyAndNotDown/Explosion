exp_add_benchmark(
    NAME Common.Math.Benchmark.Throughput
    SRC Throughput.cpp
    LIB Common
)

exp_add_benchmark(
    NAME Common.Math.Benchmark.PrimitiveLatency
    SRC PrimitiveLatency.cpp
    LIB Common
)

set(explicit_simd_benchmark_supported ON)
if (MSVC)
    # MSVC has no documented compiler-wide switch for disabling its auto-vectorizer. The definition enables
    # #pragma loop(no_vector) on each measured loop while leaving handwritten intrinsics available.
    set(no_auto_vectorization_definitions MATH_BENCHMARK_DISABLE_AUTO_VECTORIZATION=1)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(no_auto_vectorization_options -fno-vectorize -fno-slp-vectorize)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(no_auto_vectorization_options -fno-tree-loop-vectorize -fno-tree-slp-vectorize)
else ()
    set(explicit_simd_benchmark_supported OFF)
    message(WARNING "Explicit SIMD benchmark is not supported with ${CMAKE_CXX_COMPILER_ID}")
endif ()

if (explicit_simd_benchmark_supported)
    # This target deliberately compiles the exact throughput workload with automatic vectorization disabled for both
    # backends. Explicit SSE/NEON intrinsics remain intact, isolating the value of the handwritten SIMD kernels.
    exp_add_benchmark(
        NAME Common.Math.Benchmark.ExplicitSimd
        SRC Throughput.cpp
        LIB Common
        PRIVATE_COMPILE_DEF ${no_auto_vectorization_definitions}
        PRIVATE_COMPILE_OPT ${no_auto_vectorization_options}
    )
endif ()
