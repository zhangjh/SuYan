// src/platform/windows/weasel_tray_ext.cpp
// Weasel 托盘图标扩展实现

#ifdef PLATFORM_WINDOWS

#include "weasel_tray_ext.h"
#include "weasel_integration.h"

namespace ime {
namespace platform {
namespace windows {

// ==================== WeaselTrayExtension ====================

WeaselTrayExtension& WeaselTrayExtension::Instance() {
    static WeaselTrayExtension instance;
    return instance;
}

WeaselTrayExtension::WeaselTrayExtension()
    : initialized_(false),
      currentMode_(TrayInputMode::Chinese),
      lastClickTime_(0) {
}

WeaselTrayExtension::~WeaselTrayExtension() {
    Shutdown();
}

void WeaselTrayExtension::Initialize(const TrayExtConfig& config) {
    if (initialized_) {
        return;
    }

    config_ = config;

    // 从集成层获取当前输入模式
    if (WeaselIntegration::Instance().IsInitialized()) {
        auto mode = WeaselIntegration::Instance().GetInputMode();
        switch (mode) {
            case input::InputMode::English:
            case input::InputMode::TempEnglish:
                currentMode_ = TrayInputMode::English;
                break;
            default:
                currentMode_ = TrayInputMode::Chinese;
                break;
        }
    }

    initialized_ = true;
}

void WeaselTrayExtension::Shutdown() {
    if (!initialized_) {
        return;
    }

    initialized_ = false;
}

void WeaselTrayExtension::SetCurrentMode(TrayInputMode mode) {
    if (currentMode_ == mode) {
        return;
    }

    currentMode_ = mode;

    // 同步到集成层
    if (WeaselIntegration::Instance().IsInitialized()) {
        input::InputMode inputMode;
        switch (mode) {
            case TrayInputMode::English:
                inputMode = input::InputMode::English;
                break;
            default:
                inputMode = input::InputMode::Chinese;
                break;
        }
        WeaselIntegration::Instance().SetInputMode(inputMode);
    }
}

void WeaselTrayExtension::ToggleMode() {
    if (currentMode_ == TrayInputMode::Chinese) {
        SetCurrentMode(TrayInputMode::English);
    } else if (currentMode_ == TrayInputMode::English) {
        SetCurrentMode(TrayInputMode::Chinese);
    }
    // Disabled 和 Initial 状态不切换
}

std::wstring WeaselTrayExtension::GetTooltipText() const {
    std::wstring tooltip = L"跨平台输入法";

    if (config_.showModeInTooltip) {
        tooltip += L" - ";
        tooltip += GetModeName(currentMode_);
    }

    if (config_.showSchemaInTooltip && !schemaName_.empty()) {
        tooltip += L" [";
        tooltip += schemaName_;
        tooltip += L"]";
    }

    return tooltip;
}

std::wstring WeaselTrayExtension::GetModeName(TrayInputMode mode) const {
    switch (mode) {
        case TrayInputMode::Chinese:
            return L"中文";
        case TrayInputMode::English:
            return L"英文";
        case TrayInputMode::Disabled:
            return L"禁用";
        default:
            return L"初始化";
    }
}

std::vector<TrayMenuItem> WeaselTrayExtension::GetExtendedMenuItems() const {
    std::vector<TrayMenuItem> items;

    // 模式切换菜单项
    items.push_back(TrayMenuItem::Item(
        ID_TRAY_EXT_CHINESE_MODE,
        L"中文模式(&C)",
        true,
        currentMode_ == TrayInputMode::Chinese
    ));

    items.push_back(TrayMenuItem::Item(
        ID_TRAY_EXT_ENGLISH_MODE,
        L"英文模式(&E)",
        true,
        currentMode_ == TrayInputMode::English
    ));

    items.push_back(TrayMenuItem::Separator());

    // 关于菜单项
    items.push_back(TrayMenuItem::Item(
        ID_TRAY_EXT_ABOUT,
        L"关于跨平台输入法(&A)..."
    ));

    return items;
}

bool WeaselTrayExtension::HandleMenuCommand(UINT menuId) {
    switch (menuId) {
        case ID_TRAY_EXT_TOGGLE_MODE:
            ToggleMode();
            return true;

        case ID_TRAY_EXT_CHINESE_MODE:
            SetCurrentMode(TrayInputMode::Chinese);
            return true;

        case ID_TRAY_EXT_ENGLISH_MODE:
            SetCurrentMode(TrayInputMode::English);
            return true;

        case ID_TRAY_EXT_ABOUT:
            // 显示关于对话框
            MessageBoxW(
                NULL,
                L"跨平台输入法 v1.0.0\n\n"
                L"基于 RIME 开源引擎开发\n"
                L"支持 Windows 和 macOS 双平台\n\n"
                L"功能特性：\n"
                L"• 简体拼音输入\n"
                L"• 智能词频学习\n"
                L"• 自动学词\n"
                L"• 云端词库同步",
                L"关于跨平台输入法",
                MB_OK | MB_ICONINFORMATION
            );
            return true;

        default:
            // 调用外部回调
            if (menuCallback_) {
                menuCallback_(menuId);
            }
            return false;
    }
}

bool WeaselTrayExtension::HandleClick(DWORD clickTime) {
    if (!config_.enableQuickSwitch) {
        return false;
    }

    // 检测双击
    if (lastClickTime_ > 0 && 
        (clickTime - lastClickTime_) < static_cast<DWORD>(config_.doubleClickInterval)) {
        // 双击：切换模式
        ToggleMode();
        lastClickTime_ = 0;
        return true;
    }

    lastClickTime_ = clickTime;
    return false;
}

}  // namespace windows
}  // namespace platform
}  // namespace ime

#endif  // PLATFORM_WINDOWS
