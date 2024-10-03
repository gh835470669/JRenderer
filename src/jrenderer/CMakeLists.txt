
file(GLOB JRE_SOURCES "src/*.cpp")
file(GLOB JRE_H_SOURCES "src/*.h")
file(GLOB JRE_INCLUDE_SOURCES "include/*.h")
file(GLOB JRE_INCLUDE_DETAILS_SOURCES "include/details/*.h")

include_directories("include")

add_library(JRendererLib ${JRE_SOURCES} ${JRE_H_SOURCES} ${JRE_INCLUDE_SOURCES} ${JRE_INCLUDE_DETAILS_SOURCES})
target_link_libraries(JRendererLib PUBLIC fmt::fmt glm::glm)

if (WIN32)
    # 该文件包含不能在当前代码页(936)中表示的字符。请将该文件保存为 Unicode 格式以防止数据丢失 [E:\JRenderer\build\JRenderer.vcxproj]
    # 936 是 GB2312 字符集的代码页。
    set_target_properties(JRendererLib PROPERTIES COMPILE_FLAGS "/wd4819")
endif()