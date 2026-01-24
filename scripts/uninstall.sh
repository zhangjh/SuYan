#!/bin/bash
# ============================================
# SuYan Input Method - 卸载脚本
# ============================================
#
# 用法:
#   sudo ./scripts/uninstall.sh
#
# 此脚本将：
#   1. 停止正在运行的 SuYan 进程
#   2. 从系统中移除输入法
#   3. 可选：删除用户数据

set -e

# ============================================
# 颜色定义
# ============================================
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# ============================================
# 配置
# ============================================
APP_PATH="/Library/Input Methods/SuYan.app"
BUNDLE_ID="cn.zhangjh.inputmethod.SuYan"
USER_DATA_DIR="${HOME}/Library/Application Support/SuYan"

# ============================================
# 日志函数
# ============================================
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# ============================================
# 检查 root 权限
# ============================================
check_root() {
    if [ "$EUID" -ne 0 ]; then
        log_error "此脚本需要 root 权限运行"
        echo "请使用: sudo $0"
        exit 1
    fi
}

# ============================================
# 停止正在运行的进程
# ============================================
stop_process() {
    log_info "停止 SuYan 进程..."
    
    local pids=$(pgrep -x "SuYan" 2>/dev/null || true)
    
    if [ -n "${pids}" ]; then
        for pid in ${pids}; do
            log_info "终止进程 ${pid}..."
            kill -TERM "${pid}" 2>/dev/null || true
        done
        
        sleep 2
        
        # 强制终止残留进程
        pids=$(pgrep -x "SuYan" 2>/dev/null || true)
        if [ -n "${pids}" ]; then
            for pid in ${pids}; do
                kill -9 "${pid}" 2>/dev/null || true
            done
        fi
        
        log_success "进程已停止"
    else
        log_info "没有发现正在运行的进程"
    fi
}

# ============================================
# 移除输入法
# ============================================
remove_app() {
    log_info "移除输入法..."
    
    if [ -d "${APP_PATH}" ]; then
        rm -rf "${APP_PATH}"
        log_success "已移除: ${APP_PATH}"
    else
        log_warning "输入法未安装: ${APP_PATH}"
    fi
}

# ============================================
# 清理 PKG 收据
# ============================================
remove_receipts() {
    log_info "清理安装收据..."
    
    # 移除 pkgutil 收据
    if pkgutil --pkgs | grep -q "${BUNDLE_ID}"; then
        pkgutil --forget "${BUNDLE_ID}" 2>/dev/null || true
        log_success "已清理安装收据"
    else
        log_info "没有发现安装收据"
    fi
}

# ============================================
# 询问是否删除用户数据
# ============================================
ask_remove_user_data() {
    if [ -d "${USER_DATA_DIR}" ]; then
        echo ""
        echo -e "${YELLOW}发现用户数据目录: ${USER_DATA_DIR}${NC}"
        echo "此目录包含您的输入习惯和词频数据。"
        echo ""
        read -p "是否删除用户数据？[y/N] " -n 1 -r
        echo ""
        
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            rm -rf "${USER_DATA_DIR}"
            log_success "已删除用户数据"
            
            # 同时删除备份
            rm -rf "${HOME}/Library/Application Support/SuYan.backup."* 2>/dev/null || true
            log_info "已删除备份数据"
        else
            log_info "保留用户数据"
        fi
    fi
}

# ============================================
# 主函数
# ============================================
main() {
    echo ""
    echo "============================================"
    echo "  SuYan Input Method - 卸载程序"
    echo "============================================"
    echo ""
    
    # 检查 root 权限
    check_root
    
    # 确认卸载
    echo -e "${YELLOW}警告: 此操作将从系统中移除素言输入法${NC}"
    echo ""
    read -p "确定要继续吗？[y/N] " -n 1 -r
    echo ""
    
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        log_info "卸载已取消"
        exit 0
    fi
    
    echo ""
    
    # 停止进程
    stop_process
    
    # 移除应用
    remove_app
    
    # 清理收据
    remove_receipts
    
    # 询问是否删除用户数据
    ask_remove_user_data
    
    echo ""
    echo "============================================"
    log_success "素言输入法已成功卸载"
    echo "============================================"
    echo ""
    echo "请在系统设置中移除输入法（如果仍然显示）："
    echo "  系统设置 > 键盘 > 输入法 > 选择素言 > 点击 - 按钮"
    echo ""
    echo "建议注销并重新登录以完成清理。"
    echo ""
}

# 执行主函数
main
