file(GLOB_RECURSE sources Src/*.cpp)
exp_add_library(
    NAME Common
    TYPE STATIC
    SRC ${sources}
    PUBLIC_INC Include
    PUBLIC_LIB rapidjson cityhash::cityhash
    PRIVATE_LIB debugbreak::debugbreak
)

file(GLOB test_sources Test/*.cpp)
exp_add_test(
    NAME Common.Test
    INC Test
    SRC ${test_sources}
)

# exp_add_benchmark early-returns when BUILD_BENCHMARK is OFF, so this is safe to declare unconditionally.
file(GLOB benchmark_sources Benchmark/*.cpp)
exp_add_benchmark(
    NAME Common.Benchmark
    SRC ${benchmark_sources}
    LIB Common
)
