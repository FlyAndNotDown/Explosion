if (${BUILD_TEST})
    add_subdirectory(Test)
endif()

add_subdirectory(Common)
add_subdirectory(Core)
add_subdirectory(Mirror)

add_subdirectory(RHI)
add_subdirectory(RHI-Dummy)
add_subdirectory(RHI-Vulkan)
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    add_subdirectory(RHI-DirectX12)
endif()

add_subdirectory(Render)
add_subdirectory(Runtime)
add_subdirectory(Launch)
