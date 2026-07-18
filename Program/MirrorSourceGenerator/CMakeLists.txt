file(GLOB sources Src/*.cpp)
exp_add_library(
    NAME MirrorSourceGenerator.Static
    SRC ${sources}
    PUBLIC_INC Include
    PUBLIC_LIB Mirror libclang::libclang clipp::clipp
)

file(GLOB exe_sources ExeSrc/*.cpp)
exp_add_executable(
    NAME MirrorSourceGenerator
    SRC ${exe_sources}
    INC Include
    LIB MirrorSourceGenerator.Static
)

exp_add_test(
    NAME MirrorSourceGenerator.Test
    SRC Test/Main.cpp
    INC Include
    LIB MirrorSourceGenerator.Static
    RES
        ${CMAKE_CURRENT_SOURCE_DIR}/Test/MirrorSourceGeneratorInput.h->../Test/Resource/Mirror/MirrorSourceGeneratorInput.h
        ${CMAKE_SOURCE_DIR}/Engine/Source/Mirror/Include/Mirror/Meta.h->../Test/Resource/Mirror/Mirror/Meta.h
)
