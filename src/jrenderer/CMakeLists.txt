# imgui
file(GLOB IMGUI_SOURCES ${third_party_include_dir}/imgui/*.cpp
    ${third_party_include_dir}/imgui/backends/imgui_impl_win32.cpp
    ${third_party_include_dir}/imgui/backends/imgui_impl_vulkan.cpp
    ${third_party_include_dir}/imgui/misc/cpp/imgui_stdlib.cpp)

# mmd
set(MMD_DIR ${third_party_dir}/MMDFormats/MikuMikuFormats)
file(GLOB MMD_SOURCES ${MMD_DIR}/Pmx.h ${MMD_DIR}/Pmx.cpp ${MMD_DIR}/EncodingHelper.h
)

# cappuccino for cache
option(CAPPUCCINO_BUILD_EXAMPLES "Build the examples. Default=OFF" OFF)
option(CAPPUCCINO_BUILD_TESTS "Build the tests. Default=OFF" OFF)
FetchContent_Declare(
    cappuccino
    GIT_REPOSITORY https://github.com/jbaldwin/libcappuccino.git
    GIT_TAG v1.0.0
)
FetchContent_MakeAvailable(cappuccino)

# vulkan utils
file(GLOB VK_UTILS_SOURCES ${third_party_include_dir}/vulkan_utils/sample_utils.cpp ${third_party_include_dir}/vulkan_utils/raii_utils.cpp)


file(GLOB_RECURSE JRE_SOURCES src/*.cpp src/*.h src/*.hpp)

add_library(JRenderer ${JRE_SOURCES} ${IMGUI_SOURCES} ${MMD_SOURCES} ${VK_UTILS_SOURCES})
target_include_directories(JRenderer PRIVATE include
    ${third_party_include_dir}/imgui
    ${third_party_dir}/gainput/lib/include
    ${MMD_DIR})
target_link_libraries(JRenderer PUBLIC fmt::fmt glm::glm cappuccino vulkan-1)
target_link_libraries(JRenderer PUBLIC debug Debug/gainput-d)
target_link_libraries(JRenderer PUBLIC optimized Release/gainput)
target_compile_definitions(JRenderer PUBLIC NOMINMAX) # can use std::max
# vulkan
target_compile_definitions(JRenderer PUBLIC GLM_FORCE_DEFAULT_ALIGNED_GENTYPES) # glm alignment 为了Uniform Buffer与Vulkan对齐
# ASan
# target_compile_options(JRenderer PRIVATE -fsanitize=address)
# target_link_options(JRenderer PRIVATE -fsanitize=address)

# 该文件包含不能在当前代码页(936)中表示的字符。请将该文件保存为 Unicode 格式以防止数据丢失 [E:\JRenderer\build\JRenderer.vcxproj]
# 936 是 GB2312 字符集的代码页。
target_compile_options(JRenderer PUBLIC /wd4819)
if(WIN32)
    target_compile_definitions(JRenderer PRIVATE VK_USE_PLATFORM_WIN32_KHR)
endif()
