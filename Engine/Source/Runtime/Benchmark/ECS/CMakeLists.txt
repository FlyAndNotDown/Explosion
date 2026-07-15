file(GLOB sources *.cpp)
exp_add_benchmark(
    NAME Runtime.ECS.Benchmark
    SRC ${sources}
    INC .
    LIB Runtime EnTT::EnTT flecs::flecs_static
    REFLECT .
)
