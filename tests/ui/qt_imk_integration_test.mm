/**
 * Qt 与 IMK 集成测试
 *
 * 验证 Task 6.5 的关键功能：
 * 1. Qt 事件循环与 IMK RunLoop 兼容
 * 2. CandidateWindow 在所有应用上方显示
 * 3. CandidateWindow 不获取焦点
 * 4. 全屏应用场景支持
 */

#include <QApplication>
#include <QWidget>
#include <QTimer>
#include <QDebug>
#include <QScreen>
#include <QDir>
#include <QWindow>

#ifdef Q_OS_MACOS
#import <Cocoa/Cocoa.h>
#import <AppKit/AppKit.h>
#endif

#include "../../src/ui/candidate_window.h"
#include "../../src/ui/candidate_view.h"
#include "../../src/ui/theme_manager.h"
#include "../../src/ui/layout_manager.h"
#include "../../src/ui/suyan_ui_init.h"
#include "../../src/core/config_manager.h"
#include "../../src/core/input_engine.h"

using namespace suyan;

/**
 * 测试结果结构
 */
struct TestResult {
    QString name;
    bool passed;
    QString message;
};

/**
 * 测试类
 */
class QtIMKIntegrationTest {
public:
    QtIMKIntegrationTest(CandidateWindow* window)
        : window_(window)
    {}
    
    /**
     * 运行所有测试
     */
    bool runAllTests() {
        results_.clear();
        
        testWindowFlags();
        testWindowAttributes();
        testWindowLevel();
        testWindowCollectionBehavior();
        testFocusControl();
        testWindowVisibility();
        
        // 打印结果
        qDebug() << "\n========== Qt/IMK 集成测试结果 ==========";
        int passed = 0;
        int failed = 0;
        
        for (const auto& result : results_) {
            QString status = result.passed ? "[PASS]" : "[FAIL]";
            qDebug().noquote() << status << result.name;
            if (!result.message.isEmpty()) {
                qDebug().noquote() << "       " << result.message;
            }
            
            if (result.passed) {
                ++passed;
            } else {
                ++failed;
            }
        }
        
        qDebug() << "\n总计:" << passed << "通过," << failed << "失败";
        qDebug() << "==========================================\n";
        
        return failed == 0;
    }
    
private:
    /**
     * 测试窗口标志
     */
    void testWindowFlags() {
        Qt::WindowFlags flags = window_->windowFlags();
        
        bool hasToolFlag = flags & Qt::Tool;
        bool hasFramelessFlag = flags & Qt::FramelessWindowHint;
        bool hasStaysOnTopFlag = flags & Qt::WindowStaysOnTopHint;
        bool hasNoFocusFlag = flags & Qt::WindowDoesNotAcceptFocus;
        
        bool allFlagsSet = hasToolFlag && hasFramelessFlag && 
                          hasStaysOnTopFlag && hasNoFocusFlag;
        
        QString message;
        if (!hasToolFlag) message += "缺少 Qt::Tool; ";
        if (!hasFramelessFlag) message += "缺少 Qt::FramelessWindowHint; ";
        if (!hasStaysOnTopFlag) message += "缺少 Qt::WindowStaysOnTopHint; ";
        if (!hasNoFocusFlag) message += "缺少 Qt::WindowDoesNotAcceptFocus; ";
        
        results_.append({
            "窗口标志设置",
            allFlagsSet,
            allFlagsSet ? "所有必要标志已设置" : message
        });
    }
    
    /**
     * 测试窗口属性
     */
    void testWindowAttributes() {
        bool hasShowWithoutActivating = window_->testAttribute(Qt::WA_ShowWithoutActivating);
        bool hasTranslucentBackground = window_->testAttribute(Qt::WA_TranslucentBackground);
        bool hasMacAlwaysShowToolWindow = window_->testAttribute(Qt::WA_MacAlwaysShowToolWindow);
        
        bool allAttrsSet = hasShowWithoutActivating && hasTranslucentBackground && 
                          hasMacAlwaysShowToolWindow;
        
        QString message;
        if (!hasShowWithoutActivating) message += "缺少 WA_ShowWithoutActivating; ";
        if (!hasTranslucentBackground) message += "缺少 WA_TranslucentBackground; ";
        if (!hasMacAlwaysShowToolWindow) message += "缺少 WA_MacAlwaysShowToolWindow; ";
        
        results_.append({
            "窗口属性设置",
            allAttrsSet,
            allAttrsSet ? "所有必要属性已设置" : message
        });
    }
    
    /**
     * 测试 macOS 窗口级别
     */
    void testWindowLevel() {
#ifdef Q_OS_MACOS
        // 先显示窗口以确保 NSWindow 已创建
        QScreen* screen = QGuiApplication::primaryScreen();
        QPoint center = screen->availableGeometry().center();
        window_->showAt(center);
        QCoreApplication::processEvents();
        
        WId winId = window_->winId();
        if (winId == 0) {
            results_.append({
                "macOS 窗口级别",
                false,
                "无法获取窗口 ID"
            });
            return;
        }
        
        NSView* view = reinterpret_cast<NSView*>(winId);
        NSWindow* nsWindow = [view window];
        
        if (!nsWindow) {
            results_.append({
                "macOS 窗口级别",
                false,
                "无法获取 NSWindow"
            });
            return;
        }
        
        NSInteger level = [nsWindow level];
        bool levelOK = level >= NSPopUpMenuWindowLevel;
        
        results_.append({
            "macOS 窗口级别",
            levelOK,
            QString("窗口级别: %1 (需要 >= %2)")
                .arg(level)
                .arg(NSPopUpMenuWindowLevel)
        });
        
        window_->hideWindow();
#else
        results_.append({
            "macOS 窗口级别",
            true,
            "非 macOS 平台，跳过测试"
        });
#endif
    }
    
    /**
     * 测试 macOS 窗口集合行为
     */
    void testWindowCollectionBehavior() {
#ifdef Q_OS_MACOS
        // 先显示窗口
        QScreen* screen = QGuiApplication::primaryScreen();
        QPoint center = screen->availableGeometry().center();
        window_->showAt(center);
        QCoreApplication::processEvents();
        
        WId winId = window_->winId();
        NSView* view = reinterpret_cast<NSView*>(winId);
        NSWindow* nsWindow = [view window];
        
        if (!nsWindow) {
            results_.append({
                "macOS 窗口集合行为",
                false,
                "无法获取 NSWindow"
            });
            return;
        }
        
        NSWindowCollectionBehavior behavior = [nsWindow collectionBehavior];
        
        bool hasCanJoinAllSpaces = behavior & NSWindowCollectionBehaviorCanJoinAllSpaces;
        bool hasFullScreenAuxiliary = behavior & NSWindowCollectionBehaviorFullScreenAuxiliary;
        
        bool behaviorOK = hasCanJoinAllSpaces && hasFullScreenAuxiliary;
        
        QString message;
        if (!hasCanJoinAllSpaces) message += "缺少 CanJoinAllSpaces; ";
        if (!hasFullScreenAuxiliary) message += "缺少 FullScreenAuxiliary; ";
        
        results_.append({
            "macOS 窗口集合行为",
            behaviorOK,
            behaviorOK ? "支持全屏应用和多桌面" : message
        });
        
        window_->hideWindow();
#else
        results_.append({
            "macOS 窗口集合行为",
            true,
            "非 macOS 平台，跳过测试"
        });
#endif
    }
    
    /**
     * 测试焦点控制
     */
    void testFocusControl() {
        // 创建一个测试窗口来验证焦点不会被候选词窗口抢走
        QWidget testWidget;
        testWidget.setWindowTitle("焦点测试窗口");
        testWidget.resize(200, 100);
        testWidget.show();
        testWidget.activateWindow();
        testWidget.raise();
        QCoreApplication::processEvents();
        
        // 记录当前活动窗口
        QWidget* activeBeforeShow = QApplication::activeWindow();
        
        // 显示候选词窗口
        QScreen* screen = QGuiApplication::primaryScreen();
        QPoint center = screen->availableGeometry().center();
        window_->showAt(center);
        QCoreApplication::processEvents();
        
        // 检查活动窗口是否改变
        QWidget* activeAfterShow = QApplication::activeWindow();
        
        // 候选词窗口不应该成为活动窗口
        bool focusNotStolen = (activeAfterShow == activeBeforeShow) || 
                              (activeAfterShow != window_);
        
        results_.append({
            "焦点控制",
            focusNotStolen,
            focusNotStolen ? "候选词窗口不抢夺焦点" : "候选词窗口抢夺了焦点"
        });
        
        window_->hideWindow();
        testWidget.close();
    }
    
    /**
     * 测试窗口可见性控制
     */
    void testWindowVisibility() {
        // 测试显示
        QScreen* screen = QGuiApplication::primaryScreen();
        QPoint center = screen->availableGeometry().center();
        window_->showAt(center);
        QCoreApplication::processEvents();
        
        bool visibleAfterShow = window_->isWindowVisible();
        
        // 测试隐藏
        window_->hideWindow();
        QCoreApplication::processEvents();
        
        bool hiddenAfterHide = !window_->isWindowVisible();
        
        bool visibilityOK = visibleAfterShow && hiddenAfterHide;
        
        results_.append({
            "窗口可见性控制",
            visibilityOK,
            visibilityOK ? "显示/隐藏功能正常" : 
                QString("显示: %1, 隐藏: %2")
                    .arg(visibleAfterShow ? "成功" : "失败")
                    .arg(hiddenAfterHide ? "成功" : "失败")
        });
    }
    
    CandidateWindow* window_;
    QList<TestResult> results_;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    qDebug() << "Qt/IMK 集成测试程序启动";
    qDebug() << "Qt Version:" << QT_VERSION_STR;
    qDebug() << "Platform:" << QGuiApplication::platformName();
    
    // 初始化 ConfigManager
    QString configDir = QDir::tempPath() + "/suyan_qt_imk_test";
    QDir().mkpath(configDir);
    
    if (!ConfigManager::instance().initialize(configDir.toStdString())) {
        qDebug() << "ConfigManager 初始化失败";
        return 1;
    }
    
    // 初始化 UI 组件
    UIInitConfig config;
    config.followSystemTheme = true;
    
    UIInitResult result = initializeUI(config);
    if (!result.success) {
        qDebug() << "UI 初始化失败:" << result.errorMessage;
        return 1;
    }
    
    CandidateWindow* candidateWindow = result.window;
    candidateWindow->connectToThemeManager();
    candidateWindow->connectToLayoutManager();
    candidateWindow->syncFromManagers();
    
    // 运行测试
    QtIMKIntegrationTest test(candidateWindow);
    
    // 使用 QTimer 延迟执行测试
    bool testPassed = false;
    QTimer::singleShot(100, [&]() {
        testPassed = test.runAllTests();
        
        // 清理并退出
        cleanupUI(candidateWindow);
        QCoreApplication::exit(testPassed ? 0 : 1);
    });
    
    return app.exec();
}
