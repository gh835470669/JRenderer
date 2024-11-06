cmake_minimum_required(VERSION 3.11)

project(JRenderer)

set(CMAKE_CXX_STANDARD 20)


file(GLOB_RECURSE SOURCES src/*.cpp src/*.h src/*.hpp)
file(GLOB IMGUI_SOURCES imgui/*.cpp "imgui/backends/imgui_impl_win32.cpp" "imgui/backends/imgui_impl_vulkan.cpp" "imgui/misc/cpp/imgui_stdlib.cpp")
# list(APPEND IMGUI_SOURCES "imgui/backends/imgui_impl_win32.cpp")
# list(APPEND IMGUI_SOURCES "imgui/backends/imgui_impl_vulkan.cpp")
# list(APPEND IMGUI_SOURCES "imgui/misc/cpp/imgui_stdlib.cpp")
list(APPEND SOURCES ${IMGUI_SOURCES})

# tracy
list(APPEND SOURCES "tracy/public/TracyClient.cpp")

include_directories("third_party/include")
include_directories("imgui")
include_directories("tracy/public")
include_directories("gainput/lib/include")


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

# stb
include_directories("stb")

# gsl
include_directories("GSL/include")

# vulkan
add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR)
set(LIBS "vulkan-1.lib" "gainput.lib")
link_directories("third_party/lib")
# link_directories("third_party/bin")

FetchContent_Declare(
    cappuccino
    GIT_REPOSITORY https://github.com/jbaldwin/libcappuccino.git
    GIT_TAG        v1.0.0
)
FetchContent_MakeAvailable(cappuccino)

add_subdirectory(src/jrenderer)

# add_subdirectory(gainput)
# add_custom_command(TARGET gainput POST_BUILD        # Adds a post-build event to MyTest
#     COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
#         "${PROJECT_SOURCE_DIR}/libs/test.dll"      # <--this is in-file
#         $<PROJECT_BINARY_DIR/CMAKE_BUILD_TYPE>)                 # <--this is out-file path

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