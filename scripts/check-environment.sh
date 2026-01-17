#!/bin/bash
# 检查构建环境
# 用法: ./scripts/check-environment.sh

set -e

echo "=== CrossPlatformIME 构建环境检查 ==="
echo ""

# 检测操作系统
OS="$(uname -s)"
echo "操作系统: $OS"

check_tool() {
    local tool=$1
    local install_hint=$2
    if command -v $tool &> /dev/null; then
        local version=$($tool --version 2>&1 | head -n 1)
        echo "✓ $tool: $version"
        return 0
    else
        echo "✗ $tool: 未安装"
        echo "  安装方法: $install_hint"
        return 1
    fi
}

echo ""
echo "=== 必需工具 ==="

MISSING=0

check_tool "git" "brew install git (macOS) / apt install git (Linux)" || MISSING=1
check_tool "cmake" "brew install cmake (macOS) / apt install cmake (Linux)" || MISSING=1

if [ "$OS" == "Darwin" ]; then
    echo ""
    echo "=== macOS 特定检查 ==="
    
    # 检查 Xcode 命令行工具
    if xcode-select -p &> /dev/null; then
        echo "✓ Xcode 命令行工具: $(xcode-select -p)"
    else
        echo "✗ Xcode 命令行工具: 未安装"
        echo "  安装方法: xcode-select --install"
        MISSING=1
    fi
    
    # 检查编译器
    if command -v clang++ &> /dev/null; then
        echo "✓ clang++: $(clang++ --version | head -n 1)"
    else
        echo "✗ clang++: 未安装"
        MISSING=1
    fi
fi

echo ""
echo "=== 可选工具 ==="

check_tool "ninja" "brew install ninja (macOS) / apt install ninja-build (Linux)" || true
check_tool "ccache" "brew install ccache (macOS) / apt install ccache (Linux)" || true

echo ""
echo "=== Git Submodules 状态 ==="

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

check_submodule() {
    local path=$1
    local name=$2
    if [ -f "$PROJECT_ROOT/$path/CMakeLists.txt" ] || [ -f "$PROJECT_ROOT/$path/Makefile" ]; then
        echo "✓ $name: 已初始化"
    else
        echo "○ $name: 未初始化 (运行 git submodule update --init --recursive)"
    fi
}

check_submodule "deps/librime" "librime"
check_submodule "deps/weasel" "weasel"
check_submodule "deps/squirrel" "squirrel"
check_submodule "deps/rime-ice" "rime-ice"

echo ""
if [ $MISSING -eq 0 ]; then
    echo "=== 环境检查通过 ==="
    echo "可以开始构建项目"
else
    echo "=== 环境检查失败 ==="
    echo "请安装缺失的工具后重试"
    exit 1
fi
