file(GLOB SOURCES Src/*.cpp)
AddLibrary(
    NAME MirrorTool.Static
    SRC ${SOURCES}
    PUBLIC_INC Include
    LIB clipp llvm-clang Mirror
)

file(GLOB EXE_SOURCES ExeSrc/*.cpp)
AddExecutable(
    NAME MirrorTool
    SRC ${EXE_SOURCES}
    INC Include
    LIB MirrorTool.Static
)

AddTest(
    NAME MirrorTool.Test
    SRC Test/Main.cpp
    INC Include
    LIB MirrorTool.Static
    RES
        ${CMAKE_CURRENT_SOURCE_DIR}/Test/MirrorToolInput.h->../Test/Resource/Mirror/MirrorToolInput.h
        ${CMAKE_SOURCE_DIR}/Engine/Source/Mirror/Include/Mirror/Meta.h->../Test/Resource/Mirror/Mirror/Meta.h
)
