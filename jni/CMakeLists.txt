cmake_minimum_required(VERSION 3.10.2)
project("w2v_jni")

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 源文件
set(SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/TextEmbedder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/SimilaritySearch.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/com_example_w2v_W2VNative.cpp
)

# 生成共享库
add_library(w2v_jni SHARED ${SOURCE_FILES})

# 链接库
# 注意：log 和 android 是 Android 系统库
target_link_libraries(w2v_jni
    log
    android
    m
)

# 默认情况下，Android Studio 的 CMake 可能会使用 c++_shared
# 如果需要静态链接，可以在 build.gradle 中设置:
# externalNativeBuild {
#     cmake {
#         arguments "-DANDROID_STL=c++_static"
#     }
# }
