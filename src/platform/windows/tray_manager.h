/**
 * TrayManager - Windows 系统托盘管理器
 * Task 1.1: Windows 平台目录结构
 * 
 * 使用 Qt 的 QSystemTrayIcon 实现系统托盘功能
 */

#ifndef SUYAN_WINDOWS_TRAY_MANAGER_H
#define SUYAN_WINDOWS_TRAY_MANAGER_H

#ifdef _WIN32

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <string>

namespace suyan {

// 前向声明
enum class InputMode;

/**
 * TrayManager - Windows 系统托盘管理器
 * 
 * 使用 Qt 的 QSystemTrayIcon 实现：
 * - 显示当前输入模式图标
 * - 提供右键菜单
 * - 处理用户交互
 */
class TrayManager : public QObject {
    Q_OBJECT

public:
    /**
     * 获取单例实例
     */
    static TrayManager& instance();

    /**
     * 初始化托盘管理器
     * @param resourcePath 图标资源路径
     * @return 是否成功
     */
    bool initialize(const std::string& resourcePath);

    /**
     * 关闭托盘管理器
     */
    void shutdown();

    /**
     * 更新托盘图标
     * @param mode 当前输入模式
     */
    void updateIcon(InputMode mode);

    /**
     * 显示托盘图标
     */
    void show();

    /**
     * 隐藏托盘图标
     */
    void hide();

    /**
     * 检查是否已初始化
     */
    bool isInitialized() const { return initialized_; }

signals:
    /**
     * 切换模式请求
     */
    void toggleModeRequested();

    /**
     * 打开设置请求
     */
    void openSettingsRequested();

    /**
     * 显示关于对话框请求
     */
    void showAboutRequested();

    /**
     * 退出请求
     */
    void exitRequested();

private slots:
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void onToggleMode();
    void onOpenSettings();
    void onShowAbout();
    void onExit();

private:
    TrayManager();
    ~TrayManager();

    // 禁止拷贝
    TrayManager(const TrayManager&) = delete;
    TrayManager& operator=(const TrayManager&) = delete;

    // 创建托盘菜单
    void createMenu();

    // 加载图标
    QIcon loadIcon(const QString& name);

    bool initialized_ = false;
    std::string resourcePath_;
    
    QSystemTrayIcon* trayIcon_ = nullptr;
    QMenu* trayMenu_ = nullptr;
    QAction* toggleModeAction_ = nullptr;
    QAction* settingsAction_ = nullptr;
    QAction* aboutAction_ = nullptr;
    QAction* exitAction_ = nullptr;

    // 当前模式（用于图标显示）
    // InputMode currentMode_;  // 将在 Task 7 中使用
};

} // namespace suyan

#endif // _WIN32

#endif // SUYAN_WINDOWS_TRAY_MANAGER_H
