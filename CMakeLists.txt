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
    src/W2VEmbedder.cpp
    src/BertTokenizer.cpp
    src/BertEmbedder.cpp
    src/SimilaritySearch.cpp
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

# ONNX Runtime 配置
# 优先从环境变量或命令行参数获取 ONNXRUNTIME_DIR
if(NOT ONNXRUNTIME_DIR)
    if(DEFINED ENV{ONNXRUNTIME_DIR})
        set(ONNXRUNTIME_DIR "$ENV{ONNXRUNTIME_DIR}")
    else()
        # 默认搜索路径 (例如 Android 测试工程中的 SDK)
        set(ONNXRUNTIME_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android_test/bert_version/ort_android_sdk")
    endif()
endif()

message(STATUS "ONNX Runtime Dir: ${ONNXRUNTIME_DIR}")

if(EXISTS "${ONNXRUNTIME_DIR}/headers")
    include_directories(${ONNXRUNTIME_DIR}/headers)
elseif(EXISTS "${ONNXRUNTIME_DIR}/include")
    include_directories(${ONNXRUNTIME_DIR}/include)
endif()

# 根据 Android 平台设置库文件名称
if(ANDROID)
    # Android 平台通常在 jni/<abi>/ 目录下
    set(ORT_LIB_PATH "${ONNXRUNTIME_DIR}/jni/${ANDROID_ABI}/libonnxruntime.so")
    if(NOT EXISTS "${ORT_LIB_PATH}")
        # 尝试备选路径
        set(ORT_LIB_PATH "${ONNXRUNTIME_DIR}/lib/libonnxruntime.so")
    endif()
else()
    message(FATAL_ERROR "This project is now configured for Android only. Please use an Android toolchain.")
endif()

if(EXISTS "${ORT_LIB_PATH}")
    message(STATUS "Found ONNX Runtime lib: ${ORT_LIB_PATH}")
    target_link_libraries(w2v_jni ${ORT_LIB_PATH})
else()
    message(WARNING "ONNX Runtime library not found at ${ORT_LIB_PATH}. BERT inference may fail at runtime.")
endif()

target_link_libraries(w2v_jni
    ${log-lib}
    android
    m
)
