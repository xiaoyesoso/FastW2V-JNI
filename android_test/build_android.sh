#!/bin/bash

# Android JNI库构建脚本 (手动调用 NDK 编译器)
# 支持多架构编译 (arm64-v8a, armeabi-v7a)

set -e

# 设置 NDK 路径
ANDROID_NDK_HOME="/usr/local/share/android-commandlinetools/ndk/25.2.9519653"

if [ ! -d "$ANDROID_NDK_HOME" ]; then
    echo "错误: 未找到 NDK 目录: $ANDROID_NDK_HOME"
    exit 1
fi

# 设置工具链路径 (macOS)
TOOLCHAIN="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/darwin-x86_64"

# 默认包名和类名
PACKAGE_NAME="com.example.w2v"
CLASS_NAME="W2VNative"

# 如果提供了参数，则使用参数
if [ ! -z "$1" ]; then
    PACKAGE_NAME=$1
fi
if [ ! -z "$2" ]; then
    CLASS_NAME=$2
fi

JNI_CLASS_PATH=$(echo "${PACKAGE_NAME}.${CLASS_NAME}" | sed 's/\./\//g')

echo "使用 NDK: $ANDROID_NDK_HOME"
echo "目标 Java 类: $PACKAGE_NAME.$CLASS_NAME ($JNI_CLASS_PATH)"

# 编译函数
build_for_abi() {
    local ABI=$1
    local CXX_COMPILER=$2
    local ARCH_FLAGS=$3
    
    echo "=== 正在构建 $ABI ==="
    
    local BUILD_DIR="build_android_$ABI"
    local OUTPUT_DIR="app/src/main/jniLibs/$ABI"
    local PROJECT_ROOT=".."
    
    mkdir -p $BUILD_DIR
    mkdir -p $OUTPUT_DIR
    
    # 包含目录
    local INCLUDES="-I$PROJECT_ROOT/include -I$PROJECT_ROOT/src -I$PROJECT_ROOT/jni"
    
    # 编译参数
    # -static-libstdc++: 强制静态链接 C++ 标准库
    local CXXFLAGS="-std=c++17 -fPIC -O3 -DANDROID -DJNI_CLASS_NAME=\"$JNI_CLASS_PATH\" $INCLUDES $ARCH_FLAGS"
    local LDFLAGS="-shared -llog -landroid -lm -static-libstdc++"
    
    # 编译核心代码
    $CXX_COMPILER $CXXFLAGS -c $PROJECT_ROOT/src/TextEmbedder.cpp -o $BUILD_DIR/TextEmbedder.o
    $CXX_COMPILER $CXXFLAGS -c $PROJECT_ROOT/src/SimilaritySearch.cpp -o $BUILD_DIR/SimilaritySearch.o
    $CXX_COMPILER $CXXFLAGS -c $PROJECT_ROOT/src/main.cpp -o $BUILD_DIR/main.o
    $CXX_COMPILER $CXXFLAGS -c $PROJECT_ROOT/jni/com_example_w2v_W2VNative.cpp -o $BUILD_DIR/W2VNative.o
    
    # 链接生成 .so 文件
    $CXX_COMPILER $LDFLAGS \
        $BUILD_DIR/TextEmbedder.o \
        $BUILD_DIR/SimilaritySearch.o \
        $BUILD_DIR/main.o \
        $BUILD_DIR/W2VNative.o \
        -o $OUTPUT_DIR/libw2v_jni.so
        
    echo "$ABI 构建完成: $OUTPUT_DIR/libw2v_jni.so"
}

# 构建 arm64-v8a
build_for_abi "arm64-v8a" "$TOOLCHAIN/bin/aarch64-linux-android24-clang++" ""

# 构建 armeabi-v7a
# -march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16 为常用 v7a 参数
build_for_abi "armeabi-v7a" "$TOOLCHAIN/bin/armv7a-linux-androideabi24-clang++" "-march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16"

echo ""
echo "所有架构构建成功！"
echo "请将 android_test/app/src/main/jniLibs 目录下的文件提供给开发人员。"
