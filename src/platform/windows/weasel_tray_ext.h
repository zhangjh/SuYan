// src/platform/windows/weasel_tray_ext.h
// Weasel 托盘图标扩展 - 增强托盘图标和菜单功能

#ifndef IME_WEASEL_TRAY_EXT_H
#define IME_WEASEL_TRAY_EXT_H

#ifdef PLATFORM_WINDOWS

#include <windows.h>
#include <string>
#include <functional>
#include <vector>

namespace ime {
namespace platform {
namespace windows {

// 输入模式（与 WeaselTrayIcon 中的 WeaselTrayMode 对应）
enum class TrayInputMode {
    Initial = 0,    // 初始状态
    Chinese = 1,    // 中文模式
    English = 2,    // 英文模式
    Disabled = 3    // 禁用状态
};

// 菜单项定义
struct TrayMenuItem {
    UINT id;                    // 菜单项 ID
    std::wstring text;          // 显示文本
    bool enabled;               // 是否启用
    bool checked;               // 是否选中
    bool separator;             // 是否是分隔符

    TrayMenuItem() : id(0), enabled(true), checked(false), separator(false) {}

    // 创建普通菜单项
    static TrayMenuItem Item(UINT id, const std::wstring& text, 
                             bool enabled = true, bool checked = false) {
        TrayMenuItem item;
        item.id = id;
        item.text = text;
        item.enabled = enabled;
        item.checked = checked;
        return item;
    }

    // 创建分隔符
    static TrayMenuItem Separator() {
        TrayMenuItem item;
        item.separator = true;
        return item;
    }
};

// 托盘图标扩展配置
struct TrayExtConfig {
    bool showModeInTooltip;     // 在提示中显示当前模式
    bool showSchemaInTooltip;   // 在提示中显示当前方案
    bool enableQuickSwitch;     // 启用快速切换（双击切换模式）
    int doubleClickInterval;    // 双击间隔（毫秒）

    static TrayExtConfig Default() {
        return TrayExtConfig{
            .showModeInTooltip = true,
            .showSchemaInTooltip = true,
            .enableQuickSwitch = true,
            .doubleClickInterval = 500
        };
    }
};

// 托盘图标扩展
// 提供额外的托盘图标功能，与 WeaselTrayIcon 配合使用
class WeaselTrayExtension {
public:
    // 菜单命令回调
    using MenuCommandCallback = std::function<void(UINT menuId)>;

    // 获取单例实例
    static WeaselTrayExtension& Instance();

    // 禁止拷贝
    WeaselTrayExtension(const WeaselTrayExtension&) = delete;
    WeaselTrayExtension& operator=(const WeaselTrayExtension&) = delete;

    // ==================== 初始化 ====================

    // 初始化扩展
    // @param config 配置
    void Initialize(const TrayExtConfig& config = TrayExtConfig::Default());

    // 关闭扩展
    void Shutdown();

    // 检查是否已初始化
    bool IsInitialized() const { return initialized_; }

    // ==================== 模式管理 ====================

    // 获取当前输入模式
    TrayInputMode GetCurrentMode() const { return currentMode_; }

    // 设置当前输入模式（会触发图标更新）
    void SetCurrentMode(TrayInputMode mode);

    // 切换输入模式
    void ToggleMode();

    // ==================== 提示文本 ====================

    // 获取当前提示文本
    std::wstring GetTooltipText() const;

    // 设置方案名称（用于提示文本）
    void SetSchemaName(const std::wstring& name) { schemaName_ = name; }

    // ==================== 菜单扩展 ====================

    // 获取扩展菜单项（在原有菜单基础上添加）
    std::vector<TrayMenuItem> GetExtendedMenuItems() const;

    // 处理菜单命令
    // @param menuId 菜单项 ID
    // @return 是否处理了该命令
    bool HandleMenuCommand(UINT menuId);

    // 设置菜单命令回调
    void SetMenuCommandCallback(MenuCommandCallback callback) {
        menuCallback_ = callback;
    }

    // ==================== 双击处理 ====================

    // 处理托盘图标点击
    // @param clickTime 点击时间（GetTickCount）
    // @return 是否触发了双击
    bool HandleClick(DWORD clickTime);

    // ==================== 配置 ====================

    // 获取配置
    TrayExtConfig GetConfig() const { return config_; }

    // 设置配置
    void SetConfig(const TrayExtConfig& config) { config_ = config; }

private:
    WeaselTrayExtension();
    ~WeaselTrayExtension();

    // 获取模式显示名称
    std::wstring GetModeName(TrayInputMode mode) const;

private:
    bool initialized_;
    TrayExtConfig config_;
    TrayInputMode currentMode_;
    std::wstring schemaName_;
    MenuCommandCallback menuCallback_;
    DWORD lastClickTime_;
};

// ==================== 扩展菜单 ID ====================

// 扩展菜单项 ID（从 50000 开始，避免与 Weasel 原有 ID 冲突）
constexpr UINT ID_TRAY_EXT_BASE = 50000;
constexpr UINT ID_TRAY_EXT_TOGGLE_MODE = ID_TRAY_EXT_BASE + 1;
constexpr UINT ID_TRAY_EXT_CHINESE_MODE = ID_TRAY_EXT_BASE + 2;
constexpr UINT ID_TRAY_EXT_ENGLISH_MODE = ID_TRAY_EXT_BASE + 3;
constexpr UINT ID_TRAY_EXT_ABOUT = ID_TRAY_EXT_BASE + 10;

}  // namespace windows
}  // namespace platform
}  // namespace ime

#endif  // PLATFORM_WINDOWS

#endif  // IME_WEASEL_TRAY_EXT_H
