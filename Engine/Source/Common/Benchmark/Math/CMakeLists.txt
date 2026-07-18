file(GLOB sources *.cpp)
exp_add_benchmark(
    NAME Common.Math.Benchmark
    SRC ${sources}
    LIB Common
)
