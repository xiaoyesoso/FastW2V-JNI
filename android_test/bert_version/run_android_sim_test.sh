#!/bin/bash

# Android 仿真测试自动化脚本
# 功能：自动编译、安装、启动 App，并模拟点击进行性能与正确性测试

set -e

# 1. 基础配置
PACKAGE_NAME="com.example.w2v"
ACTIVITY_NAME=".MainActivity"
APK_PATH="app/build/outputs/apk/debug/app-debug.apk"
LOG_TAG="AndroidJavaTest"

# 自动探测 ADB 路径
if ! command -v adb &> /dev/null; then
    echo "未在 PATH 中找到 adb，尝试自动探测..."
    # 常见的 Android SDK 路径
    POTENTIAL_ADB_PATHS=(
        "$HOME/Library/Android/sdk/platform-tools/adb"
        "/usr/local/share/android-commandlinetools/platform-tools/adb"
        "/Users/souljoy/Library/Android/sdk/platform-tools/adb"
    )
    for path in "${POTENTIAL_ADB_PATHS[@]}"; do
        if [ -f "$path" ]; then
            alias adb="$path"
            export PATH="$(dirname "$path"):$PATH"
            echo "找到 adb: $path"
            break
        fi
    done
fi

if ! command -v adb &> /dev/null; then
    echo "错误: 无法找到 adb 命令。请确保 Android SDK 已安装并已将 platform-tools 添加到 PATH。"
    exit 1
fi

# 获取脚本所在目录
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

echo "=== [1/5] 检查设备连接 ==="
adb wait-for-device
echo "设备已就绪"

echo "=== [2/5] 编译并安装 APK ==="
./gradlew assembleDebug
adb install -r "$APK_PATH"
echo "安装成功"

echo "=== [3/5] 启动应用 ==="
adb shell am force-stop "$PACKAGE_NAME"
adb shell am start -n "$PACKAGE_NAME/$ACTIVITY_NAME"
sleep 5 # 等待应用界面渲染完成

echo "=== [4/5] 模拟点击自动化测试 ==="

# 点击 "LOAD QA FROM ASSETS" 按钮 (坐标基于标准 1080x1920 屏幕)
echo "正在点击: LOAD QA FROM ASSETS..."
adb shell input tap 540 587 
sleep 15 # 等待模型加载和 Embedding 计算（1500+条数据需要一些时间）

# 点击 "TEST BATCH SEARCH" 按钮
echo "正在点击: TEST BATCH SEARCH..."
adb shell input tap 540 741
sleep 5 # 等待搜索完成

echo "=== [5/5] 测试结果 (Logcat) ==="
echo "------------------------------------------------------------"
adb logcat -d | grep -i "$LOG_TAG" | tail -n 50
echo "------------------------------------------------------------"

echo "提示: 如需持续查看实时日志，请运行: adb logcat -s $LOG_TAG"
