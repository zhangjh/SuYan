#!/bin/bash
# macOS 构建脚本
# 用法: ./scripts/build-macos.sh [Debug|Release]

set -e

BUILD_TYPE=${1:-Debug}
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build/macos-$BUILD_TYPE"

echo "=== CrossPlatformIME macOS 构建 ==="
echo "构建类型: $BUILD_TYPE"
echo "构建目录: $BUILD_DIR"
echo ""

# 检查依赖
check_dependency() {
    if ! command -v $1 &> /dev/null; then
        echo "错误: 未找到 $1"
        echo "请安装: $2"
        exit 1
    fi
}

echo "检查构建依赖..."
check_dependency "cmake" "brew install cmake"
check_dependency "git" "xcode-select --install"

# 检查 Xcode 命令行工具
if ! xcode-select -p &> /dev/null; then
    echo "错误: 未安装 Xcode 命令行工具"
    echo "请运行: xcode-select --install"
    exit 1
fi

echo "✓ 所有依赖已就绪"
echo ""

# 检查 submodules
if [ ! -f "$PROJECT_ROOT/deps/librime/CMakeLists.txt" ]; then
    echo "警告: librime submodule 未初始化"
    echo "正在初始化 submodules..."
    cd "$PROJECT_ROOT"
    git submodule update --init --recursive
fi

# 创建构建目录
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 配置 CMake
echo "配置 CMake..."
cmake "$PROJECT_ROOT" \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DBUILD_TESTS=ON \
    -DBUILD_MACOS_FRONTEND=OFF \
    -DENABLE_CLOUD_SYNC=ON \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# 构建
echo ""
echo "开始构建..."
cmake --build . --parallel $(sysctl -n hw.ncpu)

echo ""
echo "=== 构建完成 ==="
echo "输出目录: $BUILD_DIR/bin"
echo ""

# 运行测试 (可选)
if [ "$2" == "--test" ]; then
    echo "运行测试..."
    ctest --output-on-failure
fi
