cmake_minimum_required(VERSION 3.11)

project(JRenderer)

set(CMAKE_CXX_STANDARD 20)


file(GLOB_RECURSE SOURCES src/*.cpp src/*.h src/*.hpp)

# imgui
file(GLOB IMGUI_SOURCES third_party/include/imgui/*.cpp
  third_party/include/imgui/backends/imgui_impl_win32.cpp
  third_party/include/imgui/backends/imgui_impl_vulkan.cpp
  third_party/include/imgui/misc/cpp/imgui_stdlib.cpp)
list(APPEND SOURCES ${IMGUI_SOURCES})

# tracy
list(APPEND SOURCES "third_party/tracy/public/TracyClient.cpp")

# mmd
# file(GLOB MMD_SOURCES third_party/include/MMDLoader/pmd_reader.h
#   third_party/include/MMDLoader/pmd_reader.cc
#   third_party/include/MMDLoader/pmd_model.h
#   third_party/include/MMDLoader/mmd_math.h)
# list(APPEND SOURCES ${MMD_SOURCES})
set(MMD_DIR third_party/MMDFormats/MikuMikuFormats)
file(GLOB MMD_SOURCES ${MMD_DIR}/Pmx.h ${MMD_DIR}/Pmx.cpp ${MMD_DIR}/EncodingHelper.h
)
list(APPEND SOURCES ${MMD_SOURCES})

include_directories("third_party/include")
include_directories(${MMD_DIR})
include_directories("third_party/include/imgui")
include_directories("third_party/tracy/public")
include_directories("third_party/gainput/lib/include")

# gsl
include_directories("third_party/GSL/include")


include(FetchContent)
# fmt 库
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  GIT_TAG e69e5f977d458f2650bb346dadf2ad30c5320281) # 10.2.1
FetchContent_MakeAvailable(fmt)

# glm库
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG bf71a834948186f4097caa076cd2663c69a10e1e #refs/tags/1.0.1
)
FetchContent_MakeAvailable(glm)

# vulkan
add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR)
set(LIBS
  "vulkan-1.lib"
  "gainput.lib" # build by myself
)
link_directories("third_party/lib")
# link_directories("third_party/bin")


option(CAPPUCCINO_BUILD_EXAMPLES "Build the examples. Default=OFF" OFF)
option(CAPPUCCINO_BUILD_TESTS "Build the tests. Default=OFF" OFF)
FetchContent_Declare(
  cappuccino
  GIT_REPOSITORY https://github.com/jbaldwin/libcappuccino.git
  GIT_TAG v1.0.0
)
FetchContent_MakeAvailable(cappuccino)

add_subdirectory(src/jrenderer)

add_executable(JRenderer WIN32 ${SOURCES})
target_include_directories(JRenderer PUBLIC
  "${PROJECT_SOURCE_DIR}/src/jrenderer/include"
)
target_link_libraries(JRenderer PUBLIC ${LIBS} JRendererLib fmt::fmt)
target_compile_definitions(JRenderer PUBLIC TRACY_ENABLE TRACY_HAS_CALLSTACK TRACY_CALLSTACK=20) # tracy
target_compile_definitions(JRenderer PUBLIC NOMINMAX) # can use std::max
if(WIN32)
  # 该文件包含不能在当前代码页(936)中表示的字符。请将该文件保存为 Unicode 格式以防止数据丢失 [E:\JRenderer\build\JRenderer.vcxproj]
  # 936 是 GB2312 字符集的代码页。
  set_target_properties(JRenderer PROPERTIES COMPILE_FLAGS "/wd4819")
  # set_target_properties(JRenderer PROPERTIES COMPILE_FLAGS "/utf-8") # mmd 的 pmd_reader.cc 用的是日文
endif()
