#!/bin/bash

# Android磁盘带宽测试工具构建脚本

set -e

# 默认值
BUILD_TYPE="Release"
ANDROID_BUILD=false
ANDROID_NDK_PATH=""
ANDROID_ABI="arm64-v8a"
ANDROID_PLATFORM="android-21"

# 函数：显示帮助信息
show_help() {
    echo "Android Disk Bandwidth Test - 构建脚本"
    echo ""
    echo "用法: $0 [选项]"
    echo ""
    echo "选项:"
    echo "  -h, --help              显示此帮助信息"
    echo "  -d, --debug             调试构建 (默认: Release)"
    echo "  -a, --android           为Android构建"
    echo "  -n, --ndk-path PATH     Android NDK路径"
    echo "  -b, --abi ABI           Android ABI (默认: arm64-v8a)"
    echo "                          可选: arm64-v8a, armeabi-v7a, x86, x86_64"
    echo "  -p, --platform LEVEL    Android平台级别 (默认: android-21)"
    echo ""
    echo "示例:"
    echo "  $0                      # 本地Linux构建"
    echo "  $0 -d                   # 调试构建"
    echo "  $0 -a -n /path/to/ndk   # Android构建"
    echo ""
}

# 解析命令行参数
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -a|--android)
            ANDROID_BUILD=true
            shift
            ;;
        -n|--ndk-path)
            ANDROID_NDK_PATH="$2"
            shift 2
            ;;
        -b|--abi)
            ANDROID_ABI="$2"
            shift 2
            ;;
        -p|--platform)
            ANDROID_PLATFORM="$2"
            shift 2
            ;;
        *)
            echo "未知选项: $1"
            show_help
            exit 1
            ;;
    esac
done

# 项目根目录
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"

echo "=========================================="
echo "     Android磁盘带宽测试工具构建"
echo "=========================================="
echo "项目目录: $PROJECT_DIR"
echo "构建类型: $BUILD_TYPE"

# 清理并创建构建目录
if [ -d "$BUILD_DIR" ]; then
    echo "清理旧的构建目录..."
    rm -rf "$BUILD_DIR"
fi
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 构建配置
CMAKE_ARGS="-DCMAKE_BUILD_TYPE=$BUILD_TYPE"

if [ "$ANDROID_BUILD" = true ]; then
    echo "Android构建: 是"
    echo "Android ABI: $ANDROID_ABI"
    echo "Android平台: $ANDROID_PLATFORM"
    
    # 检查NDK路径
    if [ -z "$ANDROID_NDK_PATH" ]; then
        # 尝试从环境变量获取
        if [ -n "$ANDROID_NDK_ROOT" ]; then
            ANDROID_NDK_PATH="$ANDROID_NDK_ROOT"
        elif [ -n "$NDK_ROOT" ]; then
            ANDROID_NDK_PATH="$NDK_ROOT"
        else
            echo "错误: 未指定Android NDK路径"
            echo "请使用 -n 选项指定NDK路径，或设置 ANDROID_NDK_ROOT 环境变量"
            exit 1
        fi
    fi
    
    if [ ! -d "$ANDROID_NDK_PATH" ]; then
        echo "错误: Android NDK路径不存在: $ANDROID_NDK_PATH"
        exit 1
    fi
    
    echo "Android NDK路径: $ANDROID_NDK_PATH"
    
    # Android CMake参数
    CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_PATH/build/cmake/android.toolchain.cmake"
    CMAKE_ARGS="$CMAKE_ARGS -DANDROID_ABI=$ANDROID_ABI"
    CMAKE_ARGS="$CMAKE_ARGS -DANDROID_PLATFORM=$ANDROID_PLATFORM"
    CMAKE_ARGS="$CMAKE_ARGS -DANDROID_NDK=$ANDROID_NDK_PATH"
    
    # 设置编译器
    CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_ANDROID_ARCH_ABI=$ANDROID_ABI"
    CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_SYSTEM_NAME=Android"
else
    echo "Android构建: 否 (本地Linux构建)"
fi

echo ""
echo "开始配置..."
echo "CMake参数: $CMAKE_ARGS"

# 运行CMake配置
cmake $CMAKE_ARGS "$PROJECT_DIR"

echo ""
echo "开始编译..."

# 编译项目
make -j$(nproc)

echo ""
echo "=========================================="
echo "           构建完成!"
echo "=========================================="

# 显示构建结果
if [ "$ANDROID_BUILD" = true ]; then
    EXECUTABLE="$BUILD_DIR/bin/disk_bandwidth_test"
    echo "Android可执行文件: $EXECUTABLE"
    echo ""
    echo "要将文件推送到Android设备，请运行:"
    echo "  adb push $EXECUTABLE /data/local/tmp/"
    echo "  adb shell chmod 755 /data/local/tmp/disk_bandwidth_test"
    echo ""
    echo "在Android设备上运行:"
    echo "  adb shell /data/local/tmp/disk_bandwidth_test [目录] [文件大小MB]"
    echo "  例如: adb shell /data/local/tmp/disk_bandwidth_test /sdcard 1024"
else
    EXECUTABLE="$BUILD_DIR/bin/disk_bandwidth_test"
    echo "本地可执行文件: $EXECUTABLE"
    echo ""
    echo "运行测试:"
    echo "  $EXECUTABLE [目录] [文件大小MB]"
    echo "  例如: $EXECUTABLE /tmp 100"
fi

echo ""
