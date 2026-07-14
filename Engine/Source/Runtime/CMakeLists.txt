file(GLOB_RECURSE sources Src/*.cpp)
exp_add_library(
    NAME Runtime
    TYPE SHARED
    SRC ${sources}
    PUBLIC_INC Include
    REFLECT Include
    PUBLIC_LIB Core Mirror Render
    PRIVATE_LIB Taskflow::Taskflow
)

file(GLOB test_sources Test/*.cpp)
exp_add_test(
    NAME Runtime.Test
    SRC ${test_sources}
    LIB Runtime
    INC Test
    REFLECT Test
    DEP_TARGET RHI-Dummy
)

file(GLOB benchmark_sources Benchmark/*.cpp)
exp_add_benchmark(
    NAME Runtime.Benchmark
    SRC ${benchmark_sources}
    INC Benchmark
    LIB Runtime EnTT::EnTT flecs::flecs_static
    REFLECT Benchmark
)
