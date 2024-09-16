cmake_minimum_required(VERSION 3.10)

project(JRenderer)

file(GLOB SOURCES "src/*.cpp")

include_directories("third_party/include")

set(LIBS "vulkan-1.lib")
link_directories("third_party/lib")
# link_directories("third_party/bin")

add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR)

add_executable(JRenderer WIN32 ${SOURCES})
target_link_libraries(JRenderer ${LIBS})

if (WIN32)
    # 该文件包含不能在当前代码页(936)中表示的字符。请将该文件保存为 Unicode 格式以防止数据丢失 [E:\JRenderer\build\JRenderer.vcxproj]
    # 936 是 GB2312 字符集的代码页。
    set_target_properties(JRenderer PROPERTIES COMPILE_FLAGS "/wd4819")
endif()