#!/bin/bash

# Android JNI库构建脚本 (手动调用 NDK 编译器)
# 适用于 RK3588 (ARM64)

set -e

# --- 配置区域 ---
ANDROID_NDK_HOME="/Users/souljoy/Documents/trae_projects/w2v/ndk/android-ndk-r26b"

if [ ! -d "$ANDROID_NDK_HOME" ]; then
    echo "错误: 未找到 NDK 目录: $ANDROID_NDK_HOME"
    exit 1
fi

# 设置工具链路径 (macOS)
TOOLCHAIN="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/darwin-x86_64"
CC="$TOOLCHAIN/bin/aarch64-linux-android24-clang"
CXX="$TOOLCHAIN/bin/aarch64-linux-android24-clang++"

# 检查编译器是否存在
if [ ! -f "$CXX" ]; then
    echo "错误: 未找到编译器: $CXX"
    exit 1
fi

echo "使用 NDK: $ANDROID_NDK_HOME"
echo "使用编译器: $CXX"

# 设置编译参数
ABI="arm64-v8a"
PROJECT_ROOT=".."
BUILD_DIR="build_android_manual"
OUTPUT_DIR="app/src/main/jniLibs/$ABI"

# 创建目录
mkdir -p $BUILD_DIR
mkdir -p $OUTPUT_DIR

# 包含目录
INCLUDES="-I$PROJECT_ROOT/include -I$PROJECT_ROOT/src -I$PROJECT_ROOT/jni"

# 编译参数
# -DANDROID: 标识安卓平台
# -fPIC: 生成位置无关代码 (共享库必须)
# -shared: 生成共享库
# -lcpp_static: 链接静态 C++ 标准库 (避免运行时找不到库)
CXXFLAGS="-std=c++17 -fPIC -O3 -DANDROID $INCLUDES"
LDFLAGS="-shared -llog -landroid -lm"

echo "=== 开始编译源码 ==="

# 编译核心代码
$CXX $CXXFLAGS -c $PROJECT_ROOT/src/TextEmbedder.cpp -o $BUILD_DIR/TextEmbedder.o
$CXX $CXXFLAGS -c $PROJECT_ROOT/src/SimilaritySearch.cpp -o $BUILD_DIR/SimilaritySearch.o
$CXX $CXXFLAGS -c $PROJECT_ROOT/src/main.cpp -o $BUILD_DIR/main.o
$CXX $CXXFLAGS -c $PROJECT_ROOT/jni/com_example_w2v_W2VNative.cpp -o $BUILD_DIR/W2VNative.o

echo "=== 链接生成 .so 文件 ==="
$CXX $LDFLAGS \
    $BUILD_DIR/TextEmbedder.o \
    $BUILD_DIR/SimilaritySearch.o \
    $BUILD_DIR/main.o \
    $BUILD_DIR/W2VNative.o \
    -o $OUTPUT_DIR/libw2v_jni.so

echo ""
echo "构建成功！"
echo "库文件位置: android_test/$OUTPUT_DIR/libw2v_jni.so"
file $OUTPUT_DIR/libw2v_jni.so
