#!/bin/bash
# 初始化 Git submodules
# 用法: ./scripts/init-submodules.sh

set -e

echo "=== 初始化 RIME 相关 Git submodules ==="

# 检查是否在 Git 仓库中
if [ ! -d ".git" ]; then
    echo "错误: 当前目录不是 Git 仓库根目录"
    echo "请先运行: git init"
    exit 1
fi

# 初始化并更新 submodules
echo "正在初始化 submodules..."
git submodule update --init --recursive

# 验证 submodules
echo ""
echo "=== 验证 submodules ==="

check_submodule() {
    local path=$1
    local name=$2
    if [ -f "$path/CMakeLists.txt" ] || [ -f "$path/Makefile" ]; then
        echo "✓ $name 已就绪"
    else
        echo "✗ $name 未正确初始化"
        return 1
    fi
}

check_submodule "deps/librime" "librime"
check_submodule "deps/weasel" "weasel"
check_submodule "deps/squirrel" "squirrel"
check_submodule "deps/rime-ice" "rime-ice (雾凇拼音词库)"

echo ""
echo "=== 初始化完成 ==="
echo "下一步: 运行构建脚本"
echo "  - macOS: ./scripts/build-macos.sh"
echo "  - Windows: scripts\\build-windows.bat"
