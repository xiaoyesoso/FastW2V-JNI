#!/bin/bash

# 本地 JNI 库构建脚本 (macOS)
# 用于验证代码正确性

set -e

# 设置 JAVA_HOME
if [ -z "$JAVA_HOME" ]; then
    export JAVA_HOME=$(/usr/libexec/java_home)
fi

echo "使用 JAVA_HOME: $JAVA_HOME"

# 创建构建目录
mkdir -p build/local
cd build/local

# 编译 C++ 代码
echo "1. 编译核心代码..."
g++ -std=c++17 -fPIC -O2 \
    -I../../include \
    -I../../src \
    -I../../jni \
    -I"$JAVA_HOME/include" \
    -I"$JAVA_HOME/include/darwin" \
    -c ../../src/TextEmbedder.cpp -o TextEmbedder.o

g++ -std=c++17 -fPIC -O2 \
    -I../../include \
    -I../../src \
    -I../../jni \
    -I"$JAVA_HOME/include" \
    -I"$JAVA_HOME/include/darwin" \
    -c ../../src/SimilaritySearch.cpp -o SimilaritySearch.o

g++ -std=c++17 -fPIC -O2 \
    -I../../include \
    -I../../src \
    -I../../jni \
    -I"$JAVA_HOME/include" \
    -I"$JAVA_HOME/include/darwin" \
    -c ../../src/main.cpp -o main.o

echo "2. 编译 JNI 代码..."
g++ -std=c++17 -fPIC -O2 \
    -I../../include \
    -I../../src \
    -I../../jni \
    -I"$JAVA_HOME/include" \
    -I"$JAVA_HOME/include/darwin" \
    -c ../../jni/com_example_w2v_W2VNative.cpp -o W2VNative.o

echo "3. 链接动态库..."
# macOS 使用 .dylib, Android 使用 .so
g++ -dynamiclib \
    TextEmbedder.o SimilaritySearch.o main.o W2VNative.o \
    -o libw2v_jni.dylib

echo "=== 构建成功 ==="
echo "本地库: build/local/libw2v_jni.dylib"
