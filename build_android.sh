#!/bin/bash

# Android ARM64 JNI库构建脚本 (使用CMake)
# 针对 RK3588 (Android 系统)

set -e

# --- 配置区 ---
# 请设置您的 NDK 路径，或者确保环境变量中已定义 ANDROID_NDK_HOME
if [ -z "$ANDROID_NDK_HOME" ]; then
    # 尝试常见的 NDK 路径
    NDK_PATHS=(
        "$HOME/Library/Android/sdk/ndk-bundle"
        "$HOME/Library/Android/sdk/ndk/$(ls $HOME/Library/Android/sdk/ndk/ 2>/dev/null | sort -V | tail -n 1)"
        "/usr/local/lib/android/sdk/ndk-bundle"
    )
    for path in "${NDK_PATHS[@]}"; do
        if [ -d "$path" ]; then
            ANDROID_NDK_HOME="$path"
            break
        fi
    done
fi

if [ -z "$ANDROID_NDK_HOME" ]; then
    echo "错误: 未找到 Android NDK。请设置 ANDROID_NDK_HOME 环境变量。"
    exit 1
fi

echo "使用 NDK: $ANDROID_NDK_HOME"

# 目标 ABI
ABI="arm64-v8a"
# Android API Level
API_LEVEL="24"
# 构建类型
BUILD_TYPE="Release"
# 输出目录
OUTPUT_DIR="build/android/$ABI"
# APK 库目录
JNI_LIBS_DIR="android_test/app/src/main/jniLibs/$ABI"

# --- 执行区 ---
echo "=== 开始构建 Android JNI 库 ($ABI) ==="

mkdir -p "$OUTPUT_DIR"
cd "$OUTPUT_DIR"

cmake ../../../ \
    -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
    -DANDROID_ABI="$ABI" \
    -DANDROID_NATIVE_API_LEVEL="$API_LEVEL" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

make -j$(sysctl -n hw.ncpu || nproc)

echo "=== 构建成功 ==="
echo "库文件位置: $OUTPUT_DIR/libw2v_jni.so"

# 复制到测试项目目录
mkdir -p "../../../$JNI_LIBS_DIR"
cp "libw2v_jni.so" "../../../$JNI_LIBS_DIR/"
echo "已复制到: $JNI_LIBS_DIR/libw2v_jni.so"

echo ""
echo "提示: 最终产出的 .so 文件位于 $JNI_LIBS_DIR/libw2v_jni.so"
echo "您可以将其打包进 APK，在 com.example.w2v 包下的 Java 代码中调用。"
