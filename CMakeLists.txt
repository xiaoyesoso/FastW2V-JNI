cmake_minimum_required(VERSION 3.10.2)

# 项目名称
project(w2v_jni)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含目录
include_directories(
    include
    src
    jni
)

# 核心源码
set(CORE_SOURCES
    src/TextEmbedder.cpp
    src/SimilaritySearch.cpp
    src/main.cpp
)

# JNI源码
set(JNI_SOURCES
    jni/com_example_w2v_W2VNative.cpp
)

# 构建共享库 (JNI库)
add_library(w2v_jni SHARED
    ${CORE_SOURCES}
    ${JNI_SOURCES}
)

# 链接Android日志库和其他必要库
find_library(log-lib log)
target_link_libraries(w2v_jni
    ${log-lib}
    android
    m
)
