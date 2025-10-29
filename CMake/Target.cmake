option(BUILD_TEST "Build unit tests" ON)
option(BUILD_SAMPLE "Build sample" ON)

set(API_HEADER_DIR ${CMAKE_BINARY_DIR}/Generated/Api CACHE PATH "" FORCE)

if (${BUILD_TEST})
    enable_testing()
    add_definitions(-DBUILD_TEST=1)
else()
    add_definitions(-DBUILD_TEST=0)
endif()

function(exp_gather_target_runtime_dependencies_recurse)
    cmake_parse_arguments(PARAMS "" "NAME;OUTPUT" "" ${ARGN})

    get_target_property(RUNTIME_DEP ${PARAMS_NAME} RUNTIME_DEP)
    if (NOT ("${RUNTIME_DEP}" STREQUAL "RUNTIME_DEP-NOTFOUND"))
        foreach(R ${RUNTIME_DEP})
            list(APPEND RESULT ${R})
        endforeach()
    endif()

    get_target_property(LINK_LIBRARIES ${PARAMS_NAME} LINK_LIBRARIES)
    if (NOT ("${LINK_LIBRARIES}" STREQUAL "LINK_LIBRARIES-NOTFOUND"))
        foreach(L ${LINK_LIBRARIES})
            if (NOT TARGET ${L})
                continue()
            endif()

            exp_gather_target_runtime_dependencies_recurse(
                NAME ${L}
                OUTPUT TEMP
            )
            foreach(T ${TEMP})
                list(APPEND RESULT ${T})
            endforeach()
        endforeach()
    endif()

    list(REMOVE_DUPLICATES RESULT)
    set(${PARAMS_OUTPUT} ${RESULT} PARENT_SCOPE)
endfunction()

function(exp_process_runtime_dependencies)
    cmake_parse_arguments(PARAMS "NOT_INSTALL" "NAME" "" ${ARGN})

    exp_gather_target_runtime_dependencies_recurse(
        NAME ${PARAMS_NAME}
        OUTPUT RUNTIME_DEPS
    )
    foreach(R ${RUNTIME_DEPS})
        add_custom_command(
            TARGET ${PARAMS_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${R} $<TARGET_FILE_DIR:${PARAMS_NAME}>
        )
        if (NOT ${PARAMS_NOT_INSTALL})
            install(
                FILES ${R} DESTINATION ${CMAKE_INSTALL_PREFIX}/${SUB_PROJECT_NAME}/Binaries
            )
        endif ()
    endforeach()
endfunction()

function(exp_expand_resource_path_expression)
    cmake_parse_arguments(PARAMS "" "INPUT;OUTPUT_SRC;OUTPUT_DST" "" ${ARGN})

    string(REPLACE "->" ";" TEMP ${PARAMS_INPUT})
    list(GET TEMP 0 SRC)
    list(GET TEMP 1 DST)

    set(${PARAMS_OUTPUT_SRC} ${SRC} PARENT_SCOPE)
    set(${PARAMS_OUTPUT_DST} ${DST} PARENT_SCOPE)
endfunction()

function(exp_add_resources_copy_command)
    cmake_parse_arguments(PARAMS "NOT_INSTALL" "NAME" "RES" ${ARGN})

    foreach(R ${PARAMS_RES})
        exp_expand_resource_path_expression(
            INPUT ${R}
            OUTPUT_SRC SRC
            OUTPUT_DST DST
        )

        list(APPEND COPY_COMMANDS COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SRC} $<TARGET_FILE_DIR:${PARAMS_NAME}>/${DST})

        get_filename_component(ABSOLUTE_DST ${CMAKE_INSTALL_PREFIX}/${SUB_PROJECT_NAME}/Binaries/${DST} ABSOLUTE)
        get_filename_component(DST_DIR ${ABSOLUTE_DST} DIRECTORY)
        if (NOT ${PARAMS_NOT_INSTALL})
            install(FILES ${SRC} DESTINATION ${DST_DIR})
        endif ()
    endforeach()

    set(COPY_RES_TARGET_NAME ${PARAMS_NAME}.CopyRes)
    add_custom_target(
        ${COPY_RES_TARGET_NAME}
        ${COPY_COMMANDS}
    )
    add_dependencies(${PARAMS_NAME} ${COPY_RES_TARGET_NAME})
endfunction()

function(exp_gather_target_include_dirs)
    cmake_parse_arguments(PARAMS "" "NAME;OUTPUT" "" ${ARGN})

    if (NOT (TARGET ${PARAMS_NAME}))
        return()
    endif()

    get_target_property(TARGET_INCS ${PARAMS_NAME} INTERFACE_INCLUDE_DIRECTORIES)
    if (NOT ("${TARGET_INCS}" STREQUAL "TARGET_INCS-NOTFOUND"))
        foreach(TARGET_INC ${TARGET_INCS})
            list(APPEND RESULT ${TARGET_INC})
        endforeach()
    endif()

    get_target_property(TARGET_LIBS ${PARAMS_NAME} LINK_LIBRARIES)
    if (NOT ("${TARGET_LIBS}" STREQUAL "TARGET_LIBS-NOTFOUND"))
        foreach(TARGET_LIB ${TARGET_LIBS})
            exp_gather_target_include_dirs(
                NAME ${TARGET_LIB}
                OUTPUT LIB_INCS
            )
            foreach(LIB_INC ${LIB_INCS})
                list(APPEND RESULT ${LIB_INC})
            endforeach()
        endforeach()
    endif()

    list(REMOVE_DUPLICATES RESULT)
    set(${PARAMS_OUTPUT} ${RESULT} PARENT_SCOPE)
endfunction()

function(exp_add_mirror_info_source_generation_target)
    cmake_parse_arguments(PARAMS "DYNAMIC" "NAME;OUTPUT_SRC;OUTPUT_TARGET_NAME" "SEARCH_DIR;PUBLIC_INC;PRIVATE_INC;LIB;FRAMEWORK_DIR" ${ARGN})

    if (DEFINED PARAMS_PUBLIC_INC)
        foreach (I ${PARAMS_PUBLIC_INC})
            get_filename_component(ABSOLUTE_I ${I} ABSOLUTE)
            list(APPEND INC ${ABSOLUTE_I})
        endforeach ()
    endif()
    if (DEFINED PARAMS_PRIVATE_INC)
        foreach (I ${PARAMS_PRIVATE_INC})
            get_filename_component(ABSOLUTE_I ${I} ABSOLUTE)
            list(APPEND INC ${ABSOLUTE_I})
        endforeach ()
    endif()
    if (DEFINED PARAMS_LIB)
        foreach (L ${PARAMS_LIB})
            exp_gather_target_include_dirs(
                NAME ${L}
                OUTPUT TARGET_INCS
            )
            foreach (I ${TARGET_INCS})
                list(APPEND INC ${I})
            endforeach ()
        endforeach()
    endif()
    list(REMOVE_DUPLICATES INC)

    list(APPEND INC_ARGS "-I")
    foreach (I ${INC})
        list(APPEND INC_ARGS ${I})
    endforeach()

    if (DEFINED PARAMS_FRAMEWORK_DIR)
        list(APPEND FWK_DIR ${PARAMS_FRAMEWORK_DIR})
        list(APPEND FWK_DIR_ARGS "-F")
    endif ()
    list(REMOVE_DUPLICATES FWK_DIR)

    foreach (F ${FWK_DIR})
        get_filename_component(ABSOLUTE_F ${F} ABSOLUTE)
        list(APPEND FWK_DIR_ARGS ${ABSOLUTE_F})
    endforeach ()

    if (${PARAMS_DYNAMIC})
        list(APPEND DYNAMIC_ARG "-d")
    endif ()

    foreach (SEARCH_DIR ${PARAMS_SEARCH_DIR})
        file(GLOB_RECURSE INPUT_HEADER_FILES "${SEARCH_DIR}/*.h")
        foreach (INPUT_HEADER_FILE ${INPUT_HEADER_FILES})
            string(REPLACE "${CMAKE_SOURCE_DIR}/" "" TEMP ${INPUT_HEADER_FILE})
            get_filename_component(DIR ${TEMP} DIRECTORY)
            get_filename_component(FILENAME ${TEMP} NAME_WE)

            set(OUTPUT_SOURCE "${CMAKE_BINARY_DIR}/Generated/MirrorInfoSource/${DIR}/${FILENAME}.generated.cpp")
            list(APPEND OUTPUT_SOURCES ${OUTPUT_SOURCE})

            add_custom_command(
                OUTPUT ${OUTPUT_SOURCE}
                COMMAND "$<TARGET_FILE:MirrorTool>" ${DYNAMIC_ARG} "-i" ${INPUT_HEADER_FILE} "-o" ${OUTPUT_SOURCE} ${INC_ARGS} ${FWK_DIR_ARGS}
                DEPENDS MirrorTool ${INPUT_HEADER_FILE}
            )
        endforeach()
    endforeach ()

    set(CUSTOM_TARGET_NAME "${PARAMS_NAME}.Generated")
    add_custom_target(
        ${CUSTOM_TARGET_NAME}
        DEPENDS MirrorTool ${OUTPUT_SOURCES}
    )
    set(${PARAMS_OUTPUT_SRC} ${OUTPUT_SOURCES} PARENT_SCOPE)
    set(${PARAMS_OUTPUT_TARGET_NAME} ${CUSTOM_TARGET_NAME} PARENT_SCOPE)

    if (DEFINED PARAMS_LIB)
        add_dependencies(${CUSTOM_TARGET_NAME} ${PARAMS_LIB})
    endif()
endfunction()

function(exp_add_executable)
    cmake_parse_arguments(PARAMS "SAMPLE;NOT_INSTALL" "NAME" "SRC;INC;LINK;LIB;DEP_TARGET;RES;REFLECT" ${ARGN})

    if (${PARAMS_SAMPLE} AND (NOT ${BUILD_SAMPLE}))
        return()
    endif()

    if (${PARAMS_NOT_INSTALL})
        set(NOT_INSTALL_FLAG NOT_INSTALL)
    else ()
        set(NOT_INSTALL_FLAG "")
    endif ()

    if (DEFINED PARAMS_REFLECT)
        exp_add_mirror_info_source_generation_target(
            NAME ${PARAMS_NAME}
            OUTPUT_SRC GENERATED_SRC
            OUTPUT_TARGET_NAME GENERATED_TARGET
            SEARCH_DIR ${PARAMS_REFLECT}
            PRIVATE_INC ${PARAMS_INC}
            LIB ${PARAMS_LIB}
        )
    endif()

    add_executable(${PARAMS_NAME})
    target_sources(
        ${PARAMS_NAME}
        PRIVATE ${PARAMS_SRC} ${GENERATED_SRC}
    )
    target_include_directories(
        ${PARAMS_NAME}
        PRIVATE ${PARAMS_INC}
    )
    target_link_directories(
        ${PARAMS_NAME}
        PRIVATE ${PARAMS_LINK}
    )
    target_link_libraries(
        ${PARAMS_NAME}
        PUBLIC ${PARAMS_LIB}
    )
    exp_process_runtime_dependencies(
        NAME ${PARAMS_NAME}
        ${NOT_INSTALL_FLAG}
    )
    exp_add_resources_copy_command(
        NAME ${PARAMS_NAME}
        RES ${PARAMS_RES}
        ${NOT_INSTALL_FLAG}
    )
    if (DEFINED PARAMS_DEP_TARGET)
        add_dependencies(${PARAMS_NAME} ${PARAMS_DEP_TARGET})
    endif()
    if (DEFINED PARAMS_REFLECT)
        add_dependencies(${PARAMS_NAME} ${GENERATED_TARGET})
    endif()

    if (${MSVC})
        set_target_properties(${PARAMS_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    endif()

    if (NOT ${PARAMS_NOT_INSTALL})
        install(
            TARGETS ${PARAMS_NAME}
            EXPORT ${SUB_PROJECT_NAME}Targets
            RUNTIME DESTINATION ${SUB_PROJECT_NAME}/Binaries
        )

        if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
            install(CODE "execute_process(COMMAND install_name_tool -add_rpath @executable_path ${CMAKE_INSTALL_PREFIX}/${SUB_PROJECT_NAME}/Binaries/$<TARGET_FILE_NAME:${PARAMS_NAME}>)")
        endif ()
    endif ()
endfunction()

function(exp_add_library)
    cmake_parse_arguments(PARAMS "NOT_INSTALL" "NAME;TYPE" "SRC;PRIVATE_INC;PUBLIC_INC;PRIVATE_LINK;LIB;REFLECT" ${ARGN})

    if ("${PARAMS_TYPE}" STREQUAL "SHARED")
        list(APPEND PARAMS_PUBLIC_INC ${API_HEADER_DIR}/${PARAMS_NAME})
    endif ()

    if (DEFINED PARAMS_REFLECT)
        if ("${PARAMS_TYPE}" STREQUAL "SHARED")
            list(APPEND EXTRA_PARAMS DYNAMIC)
        endif ()

        exp_add_mirror_info_source_generation_target(
            NAME ${PARAMS_NAME}
            OUTPUT_SRC GENERATED_SRC
            OUTPUT_TARGET_NAME GENERATED_TARGET
            SEARCH_DIR ${PARAMS_REFLECT}
            PUBLIC_INC ${PARAMS_PUBLIC_INC}
            PRIVATE_INC ${PARAMS_PRIVATE_INC}
            LIB ${PARAMS_LIB}
            ${EXTRA_PARAMS}
        )
    endif()

    add_library(
        ${PARAMS_NAME}
        ${PARAMS_TYPE}
    )
    target_sources(
        ${PARAMS_NAME}
        PRIVATE ${PARAMS_SRC} ${GENERATED_SRC}
    )
    foreach (INC ${PARAMS_PUBLIC_INC})
        get_filename_component(ABSOLUTE_INC ${INC} ABSOLUTE)
        list(APPEND PUBLIC_INC $<BUILD_INTERFACE:${ABSOLUTE_INC}>)
    endforeach ()
    target_include_directories(
        ${PARAMS_NAME}
        PRIVATE ${PARAMS_PRIVATE_INC}
        PUBLIC ${PUBLIC_INC} $<INSTALL_INTERFACE:${SUB_PROJECT_NAME}/Target/${PARAMS_NAME}/Include>
    )
    target_link_directories(
        ${PARAMS_NAME}
        PRIVATE ${PARAMS_PRIVATE_LINK}
        PUBLIC ${PARAMS_PUBLIC_LINK}
    )
    target_link_libraries(
        ${PARAMS_NAME}
        PUBLIC ${PARAMS_LIB}
    )

    if (${MSVC})
        target_compile_options(
            ${PARAMS_NAME}
            PUBLIC /MD$<$<CONFIG:Debug>:d>
        )
    endif()

    if ("${PARAMS_TYPE}" STREQUAL "SHARED")
        string(TOUPPER ${PARAMS_NAME}_API API_NAME)
        string(REPLACE "-" "/" API_DIR ${PARAMS_NAME})

        generate_export_header(
            ${PARAMS_NAME}
            EXPORT_MACRO_NAME ${API_NAME}
            EXPORT_FILE_NAME ${API_HEADER_DIR}/${PARAMS_NAME}/${API_DIR}/Api.h
        )
    endif()

    if (DEFINED PARAMS_REFLECT)
        add_dependencies(${PARAMS_NAME} ${GENERATED_TARGET})
    endif()

    if (NOT ${PARAMS_NOT_INSTALL})
        foreach (INC ${PARAMS_PUBLIC_INC})
            list(APPEND INSTALL_INC ${INC}/)
        endforeach ()
        install(
            DIRECTORY ${INSTALL_INC}
            DESTINATION ${SUB_PROJECT_NAME}/Target/${PARAMS_NAME}/Include
        )

        if (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Darwin" OR "${PARAMS_TYPE}" STREQUAL "STATIC")
            install(
                TARGETS ${PARAMS_NAME}
                EXPORT ${SUB_PROJECT_NAME}Targets
                ARCHIVE DESTINATION ${SUB_PROJECT_NAME}/Target/${PARAMS_NAME}/Lib
                LIBRARY DESTINATION ${SUB_PROJECT_NAME}/Target/${PARAMS_NAME}/Lib
                RUNTIME DESTINATION ${SUB_PROJECT_NAME}/Binaries
            )
        endif ()

        # TODO merge with upper, use install(TARGETS xxx FRAMEWORK DESTINATION xxx)
        if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin" AND "${PARAMS_TYPE}" STREQUAL "SHARED")
            install(
                FILES $<TARGET_FILE:${PARAMS_NAME}>
                DESTINATION ${CMAKE_INSTALL_PREFIX}/${SUB_PROJECT_NAME}/Binaries
            )
        endif ()
    endif ()
endfunction()

function(exp_add_test)
    if (NOT ${BUILD_TEST})
        return()
    endif()

    cmake_parse_arguments(PARAMS "META" "NAME" "SRC;INC;LINK;LIB;DEP_TARGET;RES;REFLECT" ${ARGN})

    if (DEFINED PARAMS_REFLECT)
        exp_add_mirror_info_source_generation_target(
            NAME ${PARAMS_NAME}
            OUTPUT_SRC GENERATED_SRC
            OUTPUT_TARGET_NAME GENERATED_TARGET
            SEARCH_DIR ${PARAMS_REFLECT}
            PRIVATE_INC ${PARAMS_INC}
            LIB ${PARAMS_LIB}
        )
    endif()

    add_executable(${PARAMS_NAME})
    target_sources(
        ${PARAMS_NAME}
        PRIVATE ${PARAMS_SRC} ${GENERATED_SRC}
    )
    target_include_directories(
        ${PARAMS_NAME}
        PRIVATE ${PARAMS_INC}
    )
    target_link_directories(
        ${PARAMS_NAME}
        PRIVATE ${PARAMS_LINK}
    )
    target_link_libraries(
        ${PARAMS_NAME}
        PRIVATE Test ${PARAMS_LIB}
    )
    exp_process_runtime_dependencies(
        NAME ${PARAMS_NAME}
        NOT_INSTALL
    )
    exp_add_resources_copy_command(
        NAME ${PARAMS_NAME}
        RES ${PARAMS_RES}
        NOT_INSTALL
    )
    if (DEFINED PARAMS_DEP_TARGET)
        add_dependencies(${PARAMS_NAME} ${PARAMS_DEP_TARGET})
    endif()
    if (DEFINED PARAMS_REFLECT)
        add_dependencies(${PARAMS_NAME} ${GENERATED_TARGET})
    endif()

    add_test(
        NAME ${PARAMS_NAME}
        COMMAND ${PARAMS_NAME}
        WORKING_DIRECTORY $<TARGET_FILE_DIR:${PARAMS_NAME}>
    )
endfunction()

install(
    EXPORT ${SUB_PROJECT_NAME}Targets
    FILE ${SUB_PROJECT_NAME}Targets.cmake
    NAMESPACE ${SUB_PROJECT_NAME}::
    DESTINATION ${SUB_PROJECT_NAME}/CMake
)
