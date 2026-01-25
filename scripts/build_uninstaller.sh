#!/bin/bash
# ============================================
# SuYan - 构建卸载器应用
# ============================================
# 创建一个可双击运行的卸载器 .app

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
OUTPUT_DIR="${PROJECT_ROOT}/output"
UNINSTALLER_APP="${OUTPUT_DIR}/SuYan卸载器.app"

echo "构建 SuYan 卸载器..."

# 创建 .app 目录结构
rm -rf "${UNINSTALLER_APP}"
mkdir -p "${UNINSTALLER_APP}/Contents/MacOS"
mkdir -p "${UNINSTALLER_APP}/Contents/Resources"

# 创建 Info.plist
cat > "${UNINSTALLER_APP}/Contents/Info.plist" << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>uninstall</string>
    <key>CFBundleIdentifier</key>
    <string>cn.zhangjh.SuYanUninstaller</string>
    <key>CFBundleName</key>
    <string>SuYan卸载器</string>
    <key>CFBundleDisplayName</key>
    <string>素言输入法卸载器</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleVersion</key>
    <string>1.0</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0</string>
    <key>LSMinimumSystemVersion</key>
    <string>13.0</string>
    <key>NSHighResolutionCapable</key>
    <true/>
</dict>
</plist>
EOF

# 创建可执行脚本（使用 AppleScript 请求管理员权限）
cat > "${UNINSTALLER_APP}/Contents/MacOS/uninstall" << 'EOF'
#!/bin/bash

# 卸载脚本路径
UNINSTALL_SCRIPT="/Library/Input Methods/SuYan.app/Contents/Resources/uninstall.sh"
FALLBACK_SCRIPT="$(dirname "$0")/../Resources/uninstall.sh"

# 检查输入法是否已安装
if [ ! -d "/Library/Input Methods/SuYan.app" ]; then
    osascript -e 'display dialog "素言输入法未安装或已被卸载。" buttons {"确定"} default button 1 with title "卸载器" with icon caution'
    exit 0
fi

# 确认卸载
RESULT=$(osascript -e 'display dialog "确定要卸载素言输入法吗？\n\n卸载后需要注销或重启才能完全生效。" buttons {"取消", "卸载"} default button 1 with title "素言输入法卸载器" with icon caution')

if [[ "$RESULT" != *"卸载"* ]]; then
    exit 0
fi

# 选择卸载脚本
if [ -f "${UNINSTALL_SCRIPT}" ]; then
    SCRIPT_TO_RUN="${UNINSTALL_SCRIPT}"
elif [ -f "${FALLBACK_SCRIPT}" ]; then
    SCRIPT_TO_RUN="${FALLBACK_SCRIPT}"
else
    osascript -e 'display dialog "找不到卸载脚本。" buttons {"确定"} default button 1 with title "错误" with icon stop'
    exit 1
fi

# 使用 AppleScript 请求管理员权限执行卸载
osascript << APPLESCRIPT
do shell script "
# 停止进程
pkill -x SuYan 2>/dev/null || true
sleep 1
pkill -9 -x SuYan 2>/dev/null || true

# 删除应用
rm -rf '/Library/Input Methods/SuYan.app'

# 清理 PKG 收据
pkgutil --forget cn.zhangjh.inputmethod.SuYan 2>/dev/null || true
" with administrator privileges
APPLESCRIPT

if [ $? -eq 0 ]; then
    # 询问是否删除用户数据
    RESULT=$(osascript -e 'display dialog "是否同时删除用户数据（输入习惯、词频等）？\n\n数据位置: ~/Library/Rime" buttons {"保留数据", "删除数据"} default button 1 with title "卸载完成" with icon note')
    
    if [[ "$RESULT" == *"删除数据"* ]]; then
        rm -rf "${HOME}/Library/Rime"
    fi
    
    osascript -e 'display dialog "素言输入法已成功卸载！\n\n请注销或重启以完成清理。\n\n如果输入法仍然显示在系统设置中，请手动移除：\n系统设置 > 键盘 > 输入法" buttons {"确定"} default button 1 with title "卸载完成" with icon note'
else
    osascript -e 'display dialog "卸载失败，请重试或手动删除。" buttons {"确定"} default button 1 with title "错误" with icon stop'
fi
EOF

chmod +x "${UNINSTALLER_APP}/Contents/MacOS/uninstall"

# 复制卸载脚本作为备份
cp "${PROJECT_ROOT}/scripts/uninstall.sh" "${UNINSTALLER_APP}/Contents/Resources/"

echo "卸载器已创建: ${UNINSTALLER_APP}"
