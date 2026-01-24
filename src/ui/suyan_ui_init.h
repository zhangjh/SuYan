/**
 * UI Initializer - UI 组件初始化器
 *
 * 提供统一的 UI 组件初始化函数，负责初始化 ThemeManager、LayoutManager
 * 和 CandidateWindow，并建立它们之间的连接。
 */

#ifndef SUYAN_UI_INIT_H
#define SUYAN_UI_INIT_H

#include <QString>
#include <memory>

namespace suyan {

// 前向声明
class CandidateWindow;

/**
 * UI 初始化配置
 */
struct UIInitConfig {
    QString themesDir;          // 主题文件目录（可选）
    QString defaultTheme;       // 默认主题名称（可选，默认跟随系统）
    bool followSystemTheme;     // 是否跟随系统深色模式（默认 true）
    
    UIInitConfig()
        : followSystemTheme(true)
    {}
};

/**
 * UI 初始化结果
 */
struct UIInitResult {
    bool success;               // 是否成功
    QString errorMessage;       // 错误信息（如果失败）
    CandidateWindow* window;    // 候选词窗口指针（成功时有效）
    
    UIInitResult()
        : success(false)
        , window(nullptr)
    {}
};

/**
 * 初始化 UI 组件
 *
 * 按顺序初始化以下组件：
 * 1. ThemeManager - 主题管理器
 * 2. LayoutManager - 布局管理器
 * 3. CandidateWindow - 候选词窗口
 *
 * 并建立它们之间的信号连接。
 *
 * @param config 初始化配置
 * @return 初始化结果
 *
 * @note 调用此函数前，需要确保：
 *       - QApplication 已创建
 *       - ConfigManager 已初始化（LayoutManager 依赖它）
 */
UIInitResult initializeUI(const UIInitConfig& config = UIInitConfig());

/**
 * 初始化 UI 组件（简化版本）
 *
 * 使用默认配置初始化 UI 组件。
 *
 * @return 候选词窗口指针，失败返回 nullptr
 */
CandidateWindow* initializeUISimple();

/**
 * 清理 UI 组件
 *
 * 释放 UI 相关资源。通常在应用退出时调用。
 *
 * @param window 候选词窗口指针（将被删除）
 */
void cleanupUI(CandidateWindow* window);

/**
 * 检查 UI 组件是否已初始化
 *
 * @return 是否已初始化
 */
bool isUIInitialized();

} // namespace suyan

#endif // SUYAN_UI_INIT_H
