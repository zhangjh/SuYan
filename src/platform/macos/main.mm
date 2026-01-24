/**
 * SuYan Input Method - macOS 主入口
 *
 * 实现输入法应用的初始化流程：
 * 1. 创建 Qt 应用实例
 * 2. 初始化 RIME 引擎
 * 3. 初始化 InputEngine
 * 4. 初始化 UI 组件（CandidateWindow）
 * 5. 创建 IMKServer
 * 6. 建立组件间连接
 * 7. 启动事件循环
 */

// ============================================
// 头文件包含顺序（重要！）
// ============================================
// rime_api.h 定义了 #define Bool int，与 Qt 冲突
// 必须先包含 Qt 头文件，然后 #undef Bool

// Qt 头文件（必须最先）
#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QTimer>
#include <QDebug>

// 取消可能的 Bool 宏定义
#ifdef Bool
#undef Bool
#endif

// macOS 框架
#import <Cocoa/Cocoa.h>
#import <InputMethodKit/InputMethodKit.h>

// 项目头文件
#include "IMKBridge.h"
#include "macos_bridge.h"
#include "status_bar_manager.h"
#include "input_engine.h"
#include "rime_wrapper.h"
#include "candidate_window.h"
#include "theme_manager.h"
#include "layout_manager.h"
#include "config_manager.h"
#include "frequency_manager.h"
#include "suyan_ui_init.h"

// 再次取消 rime_api.h 可能定义的 Bool
#ifdef Bool
#undef Bool
#endif

using namespace suyan;

// ============================================
// 全局对象
// ============================================

static IMKServer* g_imkServer = nil;
static InputEngine* g_inputEngine = nullptr;
static CandidateWindow* g_candidateWindow = nullptr;
static MacOSBridge* g_macosBridge = nullptr;

// ============================================
// 路径获取函数
// ============================================

/**
 * 获取用户数据目录
 *
 * macOS: ~/Library/Application Support/SuYan/
 */
static QString getUserDataDir() {
    QString appSupport = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    QString userDir = appSupport + "/SuYan";
    
    // 确保目录存在
    QDir dir(userDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    return userDir;
}

/**
 * 获取共享数据目录（RIME 词库）
 *
 * 优先使用 Bundle 内的数据，否则使用开发目录
 */
static QString getSharedDataDir() {
    // 1. 尝试 Bundle 内的 SharedSupport/rime
    NSBundle* bundle = [NSBundle mainBundle];
    if (bundle) {
        NSString* sharedSupport = [bundle sharedSupportPath];
        if (sharedSupport) {
            NSString* rimePath = [sharedSupport stringByAppendingPathComponent:@"rime"];
            if ([[NSFileManager defaultManager] fileExistsAtPath:rimePath]) {
                return QString::fromNSString(rimePath);
            }
        }
    }
    
    // 2. 开发环境：使用项目目录下的 data/rime
    // 通过可执行文件路径推断项目根目录
    QString execPath = QCoreApplication::applicationDirPath();
    
    // 如果在 .app bundle 内，向上查找
    if (execPath.contains(".app/Contents/MacOS")) {
        // 从 SuYan.app/Contents/MacOS 向上到项目根目录
        QDir dir(execPath);
        dir.cdUp();  // Contents
        dir.cdUp();  // SuYan.app
        dir.cdUp();  // bin
        dir.cdUp();  // build
        dir.cdUp();  // 项目根目录
        
        QString devPath = dir.absolutePath() + "/data/rime";
        if (QDir(devPath).exists()) {
            return devPath;
        }
    }
    
    // 3. 直接检查相对路径（开发环境）
    QDir currentDir(execPath);
    QStringList searchPaths = {
        "../../../data/rime",           // build/bin/SuYan.app/Contents/MacOS -> data/rime
        "../../../../data/rime",        // 更深层级
        "../data/rime",                 // 简单相对路径
        "data/rime"                     // 当前目录
    };
    
    for (const QString& path : searchPaths) {
        QString fullPath = currentDir.absoluteFilePath(path);
        if (QDir(fullPath).exists()) {
            return QDir(fullPath).absolutePath();
        }
    }
    
    // 4. 最后尝试固定开发路径
    QString devPath = "/Users/zhangjh/Projects/SuYan/data/rime";
    if (QDir(devPath).exists()) {
        return devPath;
    }
    
    qWarning() << "SuYan: Cannot find RIME shared data directory";
    return QString();
}

/**
 * 获取主题目录
 */
static QString getThemesDir() {
    // 1. 尝试 Bundle 内的 Resources/themes
    NSBundle* bundle = [NSBundle mainBundle];
    if (bundle) {
        NSString* resourcePath = [bundle resourcePath];
        if (resourcePath) {
            NSString* themesPath = [resourcePath stringByAppendingPathComponent:@"themes"];
            if ([[NSFileManager defaultManager] fileExistsAtPath:themesPath]) {
                return QString::fromNSString(themesPath);
            }
        }
    }
    
    // 2. 开发环境
    QString execPath = QCoreApplication::applicationDirPath();
    QDir currentDir(execPath);
    QStringList searchPaths = {
        "../../../resources/themes",
        "../../../../resources/themes",
        "../resources/themes",
        "resources/themes"
    };
    
    for (const QString& path : searchPaths) {
        QString fullPath = currentDir.absoluteFilePath(path);
        if (QDir(fullPath).exists()) {
            return QDir(fullPath).absolutePath();
        }
    }
    
    return QString();
}

// ============================================
// 初始化函数
// ============================================

/**
 * 初始化 RIME 引擎
 */
static bool initializeRime() {
    QString userDir = getUserDataDir();
    QString sharedDir = getSharedDataDir();
    
    qDebug() << "SuYan: User data dir:" << userDir;
    qDebug() << "SuYan: Shared data dir:" << sharedDir;
    
    if (sharedDir.isEmpty()) {
        qCritical() << "SuYan: RIME shared data directory not found";
        return false;
    }
    
    auto& rime = RimeWrapper::instance();
    if (!rime.initialize(userDir.toStdString(), sharedDir.toStdString(), "SuYan")) {
        qCritical() << "SuYan: Failed to initialize RIME engine";
        return false;
    }
    
    qDebug() << "SuYan: RIME engine initialized, version:" 
             << QString::fromStdString(rime.getVersion());
    
    return true;
}

/**
 * 初始化 InputEngine
 */
static bool initializeInputEngine() {
    QString userDir = getUserDataDir();
    QString sharedDir = getSharedDataDir();
    
    // 初始化 FrequencyManager（词频学习）
    auto& freqMgr = FrequencyManager::instance();
    if (!freqMgr.isInitialized()) {
        if (freqMgr.initialize(userDir.toStdString())) {
            qDebug() << "SuYan: FrequencyManager initialized";
        } else {
            qWarning() << "SuYan: Failed to initialize FrequencyManager (词频学习将不可用)";
            // 不是致命错误，继续初始化
        }
    }
    
    g_inputEngine = new InputEngine();
    
    if (!g_inputEngine->initialize(userDir.toStdString(), sharedDir.toStdString())) {
        qCritical() << "SuYan: Failed to initialize InputEngine";
        delete g_inputEngine;
        g_inputEngine = nullptr;
        return false;
    }
    
    // 创建并设置平台桥接
    g_macosBridge = new MacOSBridge();
    g_inputEngine->setPlatformBridge(g_macosBridge);
    
    qDebug() << "SuYan: InputEngine initialized";
    return true;
}

/**
 * 初始化 UI 组件
 */
static bool initializeUIComponents() {
    // 初始化 ConfigManager（LayoutManager 和 ThemeManager 依赖它）
    QString userDir = getUserDataDir();
    
    auto& configMgr = ConfigManager::instance();
    if (!configMgr.isInitialized()) {
        // ConfigManager::initialize 期望的是配置目录，不是配置文件路径
        configMgr.initialize(userDir.toStdString());
    }
    
    // 使用 UI 初始化器
    UIInitConfig config;
    config.themesDir = getThemesDir();
    config.followSystemTheme = true;  // 默认值，实际会从 ConfigManager 读取
    
    UIInitResult result = suyan::initializeUI(config);
    
    if (!result.success) {
        qCritical() << "SuYan: Failed to initialize UI:" << result.errorMessage;
        return false;
    }
    
    g_candidateWindow = result.window;
    
    // 初始化状态栏管理器
    auto& statusBar = StatusBarManager::instance();
    QString statusIconPath = QCoreApplication::applicationDirPath() + "/../Resources/status";
    statusBar.initialize(statusIconPath.toStdString());
    
    // 连接 InputEngine 和 CandidateWindow
    if (g_inputEngine && g_candidateWindow) {
        // 状态变化时更新候选词窗口
        // 注意：这个回调只更新候选词内容
        // 窗口位置和显示/隐藏由 IMKBridge 中的 updateCandidateWindowPosition 处理
        // 因为只有 IMKBridge 能获取到正确的光标位置
        g_inputEngine->setStateChangedCallback([](const InputState& state) {
            if (g_candidateWindow) {
                // 更新候选词内容
                g_candidateWindow->updateCandidates(state);
                
                // 如果没有候选词且不在输入状态，隐藏窗口
                // 注意：显示窗口由 IMKBridge 处理（需要光标位置）
                if (!state.isComposing || state.candidates.empty()) {
                    g_candidateWindow->hideWindow();
                }
            }
            
            // 更新状态栏图标
            auto& statusBar = StatusBarManager::instance();
            statusBar.updateIcon(state.mode);
        });
        
        // 文字提交回调
        // InputEngine 处理按键后，如果有文字需要提交，会调用这个回调
        // MacOSBridge 会将文字发送到当前应用
        g_inputEngine->setCommitTextCallback([](const std::string& text) {
            if (g_macosBridge) {
                g_macosBridge->commitText(text);
            }
        });
    }
    
    qDebug() << "SuYan: UI initialized";
    return true;
}

/**
 * 创建 IMKServer
 */
static bool createIMKServer() {
    // 从 Info.plist 获取连接名称
    NSString* connectionName = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"InputMethodConnectionName"];
    
    if (!connectionName) {
        connectionName = @"SuYan_Connection";
    }
    
    qDebug() << "SuYan: Creating IMKServer with connection:" 
             << QString::fromNSString(connectionName);
    
    g_imkServer = [[IMKServer alloc] initWithName:connectionName
                                 bundleIdentifier:[[NSBundle mainBundle] bundleIdentifier]];
    
    if (!g_imkServer) {
        qCritical() << "SuYan: Failed to create IMKServer";
        return false;
    }
    
    // 设置全局 InputEngine 和 CandidateWindow 供 IMKBridge 使用
    SuYanIMK_SetInputEngine(g_inputEngine);
    SuYanIMK_SetCandidateWindow(g_candidateWindow);
    
    qDebug() << "SuYan: IMKServer created successfully";
    return true;
}

// ============================================
// 清理函数
// ============================================

static void cleanup() {
    qDebug() << "SuYan: Cleaning up...";
    
    // 清理 UI
    if (g_candidateWindow) {
        cleanupUI(g_candidateWindow);
        g_candidateWindow = nullptr;
    }
    
    // 清理 InputEngine
    if (g_inputEngine) {
        g_inputEngine->shutdown();
        delete g_inputEngine;
        g_inputEngine = nullptr;
    }
    
    // 清理 MacOSBridge
    if (g_macosBridge) {
        delete g_macosBridge;
        g_macosBridge = nullptr;
    }
    
    // 清理 FrequencyManager
    FrequencyManager::instance().shutdown();
    
    // 清理 RIME
    RimeWrapper::instance().finalize();
    
    // 清理 IMKServer
    if (g_imkServer) {
        g_imkServer = nil;
    }
    
    qDebug() << "SuYan: Cleanup complete";
}

// ============================================
// 主函数
// ============================================

int main(int argc, char* argv[]) {
    @autoreleasepool {
        // ============================================
        // 关键：IMKServer 必须在 NSApplication 初始化之前创建
        // ============================================
        // 
        // IMK 框架要求 IMKServer 在应用启动的早期阶段创建，
        // 这样系统才能正确注册输入法 Controller。
        //
        // 初始化顺序：
        // 1. 创建 IMKServer（最先！）
        // 2. 创建 Qt 应用
        // 3. 初始化其他组件
        // 4. 启动事件循环
        
        NSLog(@"SuYan: Starting input method (early init)...");
        
        // 1. 首先创建 IMKServer
        NSString* connectionName = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"InputMethodConnectionName"];
        if (!connectionName) {
            connectionName = @"SuYan_Connection";
        }
        
        NSLog(@"SuYan: Creating IMKServer with connection: %@", connectionName);
        
        g_imkServer = [[IMKServer alloc] initWithName:connectionName
                                     bundleIdentifier:[[NSBundle mainBundle] bundleIdentifier]];
        
        if (!g_imkServer) {
            NSLog(@"SuYan: FATAL - Failed to create IMKServer");
            return 1;
        }
        
        NSLog(@"SuYan: IMKServer created successfully");
        
        // 2. 创建 Qt 应用
        // 注意：输入法不应该显示 Dock 图标，Info.plist 中已配置 LSUIElement
        QApplication app(argc, argv);
        app.setApplicationName("SuYan");
        app.setApplicationVersion("1.0.0");
        app.setOrganizationName("zhangjh");
        app.setOrganizationDomain("cn.zhangjh");
        
        // 设置 Qt 不退出当最后一个窗口关闭时
        // 输入法需要持续运行
        app.setQuitOnLastWindowClosed(false);
        
        qDebug() << "SuYan: Qt application created";
        
        // 3. 初始化 RIME 引擎
        if (!initializeRime()) {
            qCritical() << "SuYan: RIME initialization failed, exiting";
            return 1;
        }
        
        // 4. 初始化 InputEngine
        if (!initializeInputEngine()) {
            qCritical() << "SuYan: InputEngine initialization failed, exiting";
            RimeWrapper::instance().finalize();
            return 1;
        }
        
        // 5. 初始化 UI
        if (!initializeUIComponents()) {
            qCritical() << "SuYan: UI initialization failed, exiting";
            cleanup();
            return 1;
        }
        
        // 6. 设置全局 InputEngine 和 CandidateWindow 供 IMKBridge 使用
        SuYanIMK_SetInputEngine(g_inputEngine);
        SuYanIMK_SetCandidateWindow(g_candidateWindow);
        
        qDebug() << "SuYan: Input method started successfully";
        
        // 7. 注册清理函数
        QObject::connect(&app, &QCoreApplication::aboutToQuit, []() {
            cleanup();
        });
        
        // 8. 启动事件循环
        //
        // Qt 事件循环与 macOS RunLoop 兼容性说明：
        //
        // QApplication::exec() 在 macOS 上的实现：
        // - 使用 CFRunLoopRun() 作为底层事件循环
        // - CFRunLoop 与 NSRunLoop 共享同一个运行循环
        // - IMK 框架的回调通过 NSRunLoop 调度
        // - 因此 Qt 事件循环和 IMK 回调可以正确交互
        //
        int result = app.exec();
        
        return result;
    }
}
