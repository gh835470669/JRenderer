cmake_minimum_required(VERSION 3.11)

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
file(GLOB_RECURSE STAT_SOURCES "src/helper/*.cpp")
file(GLOB_RECURSE STAT_H_SOURCES "src/helper/*.h")
list(APPEND SOURCES ${IMGUI_SOURCES})
list(APPEND SOURCES ${IMGUI_HEADER_HELPER_SOURCES})
list(APPEND SOURCES ${IMGUI_HELPER_SOURCES})
list(APPEND SOURCES ${HPP_SOURCES})
list(APPEND SOURCES ${H_SOURCES})
list(APPEND SOURCES ${STAT_SOURCES})
list(APPEND SOURCES ${STAT_H_SOURCES})

# tracy
list(APPEND SOURCES "tracy/public/TracyClient.cpp")

include_directories("third_party/include")
include_directories("imgui")
include_directories("tracy/public")


include(FetchContent)
# fmt 库
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  GIT_TAG        e69e5f977d458f2650bb346dadf2ad30c5320281) # 10.2.1
FetchContent_MakeAvailable(fmt)

# glm库
FetchContent_Declare(
	glm
	GIT_REPOSITORY	https://github.com/g-truc/glm.git
	GIT_TAG 	bf71a834948186f4097caa076cd2663c69a10e1e #refs/tags/1.0.1
)
FetchContent_MakeAvailable(glm)

# vulkan
add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR)
set(LIBS "vulkan-1.lib")
link_directories("third_party/lib")
# link_directories("third_party/bin")

# set(CMAKE_CXX_STANDARD 14)

add_subdirectory(src/jrenderer)

add_executable(JRenderer WIN32 ${SOURCES})
target_include_directories(JRenderer PUBLIC
                          "${PROJECT_SOURCE_DIR}/src/jrenderer/include"
                          )
target_link_libraries(JRenderer PUBLIC ${LIBS} JRendererLib fmt::fmt)
target_compile_definitions(JRenderer PUBLIC TRACY_ENABLE TRACY_HAS_CALLSTACK TRACY_CALLSTACK=20)


if (WIN32)
    # 该文件包含不能在当前代码页(936)中表示的字符。请将该文件保存为 Unicode 格式以防止数据丢失 [E:\JRenderer\build\JRenderer.vcxproj]
    # 936 是 GB2312 字符集的代码页。
    set_target_properties(JRenderer PROPERTIES COMPILE_FLAGS "/wd4819")
endif()