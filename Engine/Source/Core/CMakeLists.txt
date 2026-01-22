set(generated_include_dir ${GENERATED_DIR}/Configure/Core/Include)

configure_file(
    Template/EngineVersion.h.in
    ${generated_include_dir}/Core/EngineVersion.h
)

file(GLOB sources Src/*.cpp)
exp_add_library(
    NAME Core
    TYPE SHARED
    SRC ${sources}
    PUBLIC_INC Include ${generated_include_dir}
    PUBLIC_LIB Common
    PUBLIC_MERGE_LIB clipp::clipp
)

file(GLOB test_sources Test/*.cpp)
exp_add_test(
    NAME Core.Test
    SRC ${test_sources}
    LIB Core
)
