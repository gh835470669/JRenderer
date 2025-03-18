cmake_minimum_required(VERSION 3.11)

project(JRenderApp)

set(CMAKE_CXX_STANDARD 23)

# source files
file(GLOB_RECURSE SOURCES src/*.cpp src/*.h src/*.hpp)
list(FILTER SOURCES EXCLUDE REGEX src/jrenderer/.)

# all third_party
set(third_party_dir ${PROJECT_SOURCE_DIR}/third_party)
set(third_party_include_dir ${third_party_dir}/include)
include_directories(third_party/include)
link_directories(third_party/lib) # 这个需要在add_executable之前，而不是target_link_libraries之前


# tracy
list(APPEND SOURCES third_party/tracy/public/TracyClient.cpp)
include_directories(third_party/tracy/public)

# gsl
include_directories(third_party/GSL/include)

include(FetchContent)
# fmt 格式化输出
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  GIT_TAG e69e5f977d458f2650bb346dadf2ad30c5320281 # 10.2.1
  EXCLUDE_FROM_ALL # 不用它的install
)
FetchContent_MakeAvailable(fmt)

# glm 数学
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG bf71a834948186f4097caa076cd2663c69a10e1e #refs/tags/1.0.1
)
FetchContent_MakeAvailable(glm)

add_subdirectory(src/jrenderer)

add_executable(JRenderApp WIN32 ${SOURCES})
target_include_directories(JRenderApp PRIVATE src/jrenderer/include ${third_party_dir}/gainput/lib/include)
target_link_libraries(JRenderApp PRIVATE JRenderer)
target_compile_definitions(JRenderApp PRIVATE TRACY_ENABLE TRACY_HAS_CALLSTACK TRACY_CALLSTACK=20) # tracy

# precompile shaders
include(res/shaders/compile_shaders.cmake)
compile_shaders()

# copy dll when build
add_custom_command(TARGET JRenderApp
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:${PROJECT_NAME}> ${PROJECT_SOURCE_DIR}/third_party/lib/$<CONFIG>/gainput$<$<CONFIG:Debug>:-d>.dll
  VERBATIM)

# install
install(TARGETS JRenderApp DESTINATION .)
install(
  FILES ${PROJECT_SOURCE_DIR}/third_party/lib/$<CONFIG>/gainput$<$<CONFIG:Debug>:-d>.dll
  DESTINATION .
)
install(
  DIRECTORY ${PROJECT_SOURCE_DIR}/res
  DESTINATION .
)
