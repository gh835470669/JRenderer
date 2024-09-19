cmake_minimum_required(VERSION 3.10)

project(JRenderer)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HPP_SOURCES "src/*.hpp")
file(GLOB_RECURSE H_SOURCES "src/*.h")
file(GLOB IMGUI_SOURCES "imgui/*.cpp")
list(APPEND IMGUI_SOURCES "imgui/backends/imgui_impl_win32.cpp")
list(APPEND IMGUI_SOURCES "imgui/backends/imgui_impl_vulkan.cpp")
list(APPEND IMGUI_SOURCES "imgui/misc/cpp/imgui_stdlib.cpp")
file(GLOB IMGUI_HELPER_SOURCES "src/imgui_helper/*.cpp")
file(GLOB IMGUI_HEADER_HELPER_SOURCES "src/imgui_helper/*.h")
list(APPEND SOURCES ${IMGUI_SOURCES})
list(APPEND SOURCES ${IMGUI_HEADER_HELPER_SOURCES})
list(APPEND SOURCES ${IMGUI_HELPER_SOURCES})
list(APPEND SOURCES ${HPP_SOURCES})
list(APPEND SOURCES ${H_SOURCES})

include_directories("third_party/include")
include_directories("imgui")

set(LIBS "vulkan-1.lib")
link_directories("third_party/lib")
# link_directories("third_party/bin")

add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR)

set(CMAKE_CXX_STANDARD 14)

add_executable(JRenderer WIN32 ${SOURCES})
target_link_libraries(JRenderer ${LIBS})

if (WIN32)
    # 该文件包含不能在当前代码页(936)中表示的字符。请将该文件保存为 Unicode 格式以防止数据丢失 [E:\JRenderer\build\JRenderer.vcxproj]
    # 936 是 GB2312 字符集的代码页。
    set_target_properties(JRenderer PROPERTIES COMPILE_FLAGS "/wd4819")
endif()