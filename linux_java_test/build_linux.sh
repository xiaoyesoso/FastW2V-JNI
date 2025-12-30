#!/bin/bash

# Linux JNI 库构建脚本
# 用于在 Linux 环境下编译 .so 库并运行 Java 测试

set -e

# 设置 JAVA_HOME (如果未设置)
if [ -z "$JAVA_HOME" ]; then
    # 尝试自动检测 Linux 常见的 JAVA_HOME
    if [ -f "/usr/lib/jvm/java-8-openjdk-amd64/include/jni.h" ]; then
        export JAVA_HOME="/usr/lib/jvm/java-8-openjdk-amd64"
    elif [ -f "/usr/lib/jvm/java-11-openjdk-amd64/include/jni.h" ]; then
        export JAVA_HOME="/usr/lib/jvm/java-11-openjdk-amd64"
    else
        # macOS 兼容处理 (用于当前开发环境测试)
        export JAVA_HOME=$(/usr/libexec/java_home)
    fi
fi

echo "使用 JAVA_HOME: $JAVA_HOME"

# 设置编译参数
INCLUDES="-I../include -I../src -I../jni -I$JAVA_HOME/include"
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    INCLUDES="$INCLUDES -I$JAVA_HOME/include/linux"
    LIB_EXT="so"
    CXX_FLAGS="-shared -fPIC"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    INCLUDES="$INCLUDES -I$JAVA_HOME/include/darwin"
    LIB_EXT="dylib"
    CXX_FLAGS="-dynamiclib -fPIC"
fi

# 1. 编译 C++ 源码
echo "1. 编译 C++ 核心库与 JNI 代码..."
mkdir -p build
# 针对 macOS 的架构匹配处理
ARCH_FLAGS=""
if [[ "$OSTYPE" == "darwin"* ]]; then
    # 强制编译为 x86_64 以匹配当前的 Java 环境
    ARCH_FLAGS="-arch x86_64"
fi

g++ -std=c++17 $CXX_FLAGS $INCLUDES $ARCH_FLAGS \
    ../src/TextEmbedder.cpp \
    ../src/SimilaritySearch.cpp \
    ../src/main.cpp \
    ../jni/com_example_w2v_W2VNative.cpp \
    -o libs/libw2v_jni.$LIB_EXT

# 2. 编译 Java 代码
echo "2. 编译 Java 测试代码..."
javac -d . src/com/example/w2v/W2VNative.java
javac -cp . src/LinuxJavaTest.java

echo "=== 构建完成 ==="
echo "库文件: libs/libw2v_jni.$LIB_EXT"
echo ""
echo "运行测试命令:"
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo "java -Djava.library.path=./libs -cp . LinuxJavaTest"
else
    echo "java -Djava.library.path=./libs -cp . LinuxJavaTest"
fi
