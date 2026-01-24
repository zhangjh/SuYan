/**
 * UI Initializer 实现
 */

#include "suyan_ui_init.h"
#include "theme_manager.h"
#include "layout_manager.h"
#include "candidate_window.h"
#include "../core/config_manager.h"
#include <QApplication>
#include <iostream>

namespace suyan {

// 全局初始化状态
static bool g_uiInitialized = false;

UIInitResult initializeUI(const UIInitConfig& config) {
    UIInitResult result;
    
    // 检查 QApplication 是否存在
    if (!QApplication::instance()) {
        result.errorMessage = "QApplication 未创建";
        std::cerr << "UIInitializer: " << result.errorMessage.toStdString() << std::endl;
        return result;
    }
    
    // 1. 初始化 ThemeManager
    auto& themeMgr = ThemeManager::instance();
    if (!themeMgr.isInitialized()) {
        if (!themeMgr.initialize(config.themesDir)) {
            result.errorMessage = "ThemeManager 初始化失败";
            std::cerr << "UIInitializer: " << result.errorMessage.toStdString() << std::endl;
            return result;
        }
    }
    
    // 从 ConfigManager 读取主题配置
    // ThemeManager::initialize() 已经调用了 syncFromConfigManager()
    // 但如果 config 中指定了特定设置，需要覆盖
    auto& configMgr = ConfigManager::instance();
    if (configMgr.isInitialized()) {
        // 从 ConfigManager 读取主题模式
        ThemeConfig themeConfig = configMgr.getThemeConfig();
        
        // 如果 ConfigManager 中设置了跟随系统，使用 ConfigManager 的设置
        // 否则使用传入的 config.followSystemTheme
        if (themeConfig.mode == ThemeMode::Auto) {
            themeMgr.setFollowSystemTheme(true);
        } else {
            themeMgr.setFollowSystemTheme(false);
            // 设置具体的主题
            if (themeConfig.mode == ThemeMode::Light) {
                themeMgr.setCurrentTheme(QString::fromUtf8("浅色"));
            } else if (themeConfig.mode == ThemeMode::Dark) {
                themeMgr.setCurrentTheme(QString::fromUtf8("深色"));
            }
        }
    } else {
        // ConfigManager 未初始化，使用传入的配置
        themeMgr.setFollowSystemTheme(config.followSystemTheme);
        
        // 如果指定了默认主题且不跟随系统，则设置
        if (!config.followSystemTheme && !config.defaultTheme.isEmpty()) {
            themeMgr.setCurrentTheme(config.defaultTheme);
        }
    }
    
    // 2. 初始化 LayoutManager
    auto& layoutMgr = LayoutManager::instance();
    if (!layoutMgr.isInitialized()) {
        // 注意：LayoutManager 依赖 ConfigManager
        // 如果 ConfigManager 未初始化，LayoutManager 会使用默认配置
        if (!layoutMgr.initialize()) {
            // LayoutManager 初始化失败不是致命错误，使用默认配置继续
            std::cerr << "UIInitializer: LayoutManager 初始化失败，使用默认配置" << std::endl;
        }
    }
    
    // 3. 创建 CandidateWindow
    // CandidateWindow 构造函数会自动连接 ThemeManager 和 LayoutManager
    CandidateWindow* window = new CandidateWindow();
    
    // 标记初始化完成
    g_uiInitialized = true;
    
    result.success = true;
    result.window = window;
    return result;
}

CandidateWindow* initializeUISimple() {
    UIInitConfig config;
    UIInitResult result = initializeUI(config);
    return result.window;
}

void cleanupUI(CandidateWindow* window) {
    if (window) {
        // 断开信号连接
        window->disconnectFromThemeManager();
        window->disconnectFromLayoutManager();
        
        // 删除窗口
        delete window;
    }
    
    g_uiInitialized = false;
}

bool isUIInitialized() {
    return g_uiInitialized;
}

} // namespace suyan
