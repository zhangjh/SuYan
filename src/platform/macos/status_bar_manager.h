/**
 * StatusBarManager - macOS 状态栏图标管理器
 *
 * 管理输入法在系统菜单栏的状态图标，根据输入模式切换显示。
 * 
 * 功能：
 * - 显示当前输入模式（中文/英文）
 * - 模式切换时更新图标
 * - 支持深色/浅色模式自适应
 */

#ifndef SUYAN_PLATFORM_MACOS_STATUS_BAR_MANAGER_H
#define SUYAN_PLATFORM_MACOS_STATUS_BAR_MANAGER_H

#include <string>
#include <functional>

namespace suyan {

// 前向声明
enum class InputMode;

/**
 * 状态栏图标类型
 */
enum class StatusIconType {
    Chinese,    // 中文模式
    English,    // 英文模式
    Disabled    // 禁用状态
};

/**
 * StatusBarManager - 状态栏管理器
 *
 * 单例模式，管理 macOS 菜单栏中的输入法状态图标。
 * 
 * 注意：macOS 输入法的状态栏图标由系统管理，
 * 通过 Info.plist 中的 tsInputMethodIconFileKey 配置。
 * 但我们可以通过 IMKInputController 的 menu 方法
 * 提供菜单，并在菜单标题中显示当前状态。
 */
class StatusBarManager {
public:
    /**
     * 获取单例实例
     */
    static StatusBarManager& instance();

    /**
     * 初始化状态栏管理器
     *
     * @param resourcePath 图标资源路径
     * @return 是否成功
     */
    bool initialize(const std::string& resourcePath);

    /**
     * 更新状态图标
     *
     * @param mode 当前输入模式
     */
    void updateIcon(InputMode mode);

    /**
     * 设置图标类型
     *
     * @param type 图标类型
     */
    void setIconType(StatusIconType type);

    /**
     * 获取当前图标类型
     */
    StatusIconType getCurrentIconType() const;

    /**
     * 获取当前模式的显示文本
     *
     * @return 模式文本（如 "中" 或 "A"）
     */
    std::string getModeText() const;

    /**
     * 检查是否已初始化
     */
    bool isInitialized() const { return initialized_; }

private:
    StatusBarManager();
    ~StatusBarManager();

    // 禁止拷贝
    StatusBarManager(const StatusBarManager&) = delete;
    StatusBarManager& operator=(const StatusBarManager&) = delete;

    bool initialized_ = false;
    StatusIconType currentIconType_ = StatusIconType::Chinese;
    std::string resourcePath_;
};

} // namespace suyan

#endif // SUYAN_PLATFORM_MACOS_STATUS_BAR_MANAGER_H
