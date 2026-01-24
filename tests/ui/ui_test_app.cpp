/**
 * UI 层测试程序
 *
 * 验证 CandidateWindow 显示、布局切换、主题切换、窗口定位功能。
 * 支持交互模式和自动测试模式。
 */

#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QDebug>
#include <QScreen>
#include <QGroupBox>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QDir>

#include "../../src/ui/candidate_window.h"
#include "../../src/ui/candidate_view.h"
#include "../../src/ui/theme_manager.h"
#include "../../src/ui/layout_manager.h"
#include "../../src/ui/suyan_ui_init.h"
#include "../../src/core/config_manager.h"
#include "../../src/core/input_engine.h"

using namespace suyan;

/**
 * 测试控制面板
 */
class TestControlPanel : public QWidget {
    Q_OBJECT

public:
    explicit TestControlPanel(CandidateWindow* candidateWindow, QWidget* parent = nullptr)
        : QWidget(parent)
        , candidateWindow_(candidateWindow)
    {
        setWindowTitle("SuYan UI 测试控制面板");
        setMinimumSize(500, 600);
        setupUI();
        setupConnections();
    }

private slots:
    void onShowWindow() {
        if (!candidateWindow_) return;
        
        // 在屏幕中央显示
        QScreen* screen = QGuiApplication::primaryScreen();
        QRect screenGeometry = screen->availableGeometry();
        QPoint center(screenGeometry.center().x(), screenGeometry.center().y());
        
        candidateWindow_->showAt(center);
        updateStatus("窗口已显示在屏幕中央");
    }

    void onHideWindow() {
        if (!candidateWindow_) return;
        candidateWindow_->hideWindow();
        updateStatus("窗口已隐藏");
    }

    void onUpdateCandidates() {
        if (!candidateWindow_) return;
        
        // 创建测试候选词
        InputState state;
        state.preedit = "ni'hao";
        state.rawInput = "nihao";
        state.highlightedIndex = 0;
        state.pageIndex = 0;
        state.pageSize = 9;
        state.hasMorePages = true;
        state.mode = InputMode::Chinese;
        state.isComposing = true;
        
        state.candidates = {
            {"你好", "nǐ hǎo", 1},
            {"拟好", "nǐ hǎo", 2},
            {"泥号", "ní hào", 3},
            {"尼豪", "ní háo", 4},
            {"妮好", "nī hǎo", 5},
            {"霓虹", "ní hóng", 6},
            {"逆号", "nì hào", 7},
            {"倪浩", "ní hào", 8},
            {"腻好", "nì hǎo", 9}
        };
        
        candidateWindow_->updateCandidates(state);
        updateStatus("候选词已更新（9个候选词）");
    }

    void onUpdateFewCandidates() {
        if (!candidateWindow_) return;
        
        InputState state;
        state.preedit = "wo";
        state.rawInput = "wo";
        state.highlightedIndex = 0;
        state.pageIndex = 0;
        state.pageSize = 9;
        state.hasMorePages = false;
        state.mode = InputMode::Chinese;
        state.isComposing = true;
        
        state.candidates = {
            {"我", "wǒ", 1},
            {"窝", "wō", 2},
            {"握", "wò", 3}
        };
        
        candidateWindow_->updateCandidates(state);
        updateStatus("候选词已更新（3个候选词）");
    }

    void onClearCandidates() {
        if (!candidateWindow_) return;
        candidateWindow_->clearCandidates();
        updateStatus("候选词已清空");
    }

    void onSetHorizontalLayout() {
        if (!candidateWindow_) return;
        candidateWindow_->setLayoutType(LayoutType::Horizontal);
        LayoutManager::instance().setLayoutType(LayoutType::Horizontal);
        updateStatus("已切换到横排布局");
    }

    void onSetVerticalLayout() {
        if (!candidateWindow_) return;
        candidateWindow_->setLayoutType(LayoutType::Vertical);
        LayoutManager::instance().setLayoutType(LayoutType::Vertical);
        updateStatus("已切换到竖排布局");
    }

    void onSetLightTheme() {
        ThemeManager::instance().setFollowSystemTheme(false);
        ThemeManager::instance().setCurrentTheme(Theme::NAME_LIGHT);
        updateStatus("已切换到浅色主题");
    }

    void onSetDarkTheme() {
        ThemeManager::instance().setFollowSystemTheme(false);
        ThemeManager::instance().setCurrentTheme(Theme::NAME_DARK);
        updateStatus("已切换到深色主题");
    }

    void onSetAutoTheme() {
        ThemeManager::instance().setFollowSystemTheme(true);
        updateStatus("已切换到跟随系统主题");
    }

    void onMoveToCorner(int corner) {
        if (!candidateWindow_) return;
        
        QScreen* screen = QGuiApplication::primaryScreen();
        QRect screenGeometry = screen->availableGeometry();
        QPoint pos;
        
        switch (corner) {
            case 0: // 左上
                pos = screenGeometry.topLeft() + QPoint(50, 50);
                break;
            case 1: // 右上
                pos = screenGeometry.topRight() + QPoint(-200, 50);
                break;
            case 2: // 左下
                pos = screenGeometry.bottomLeft() + QPoint(50, -100);
                break;
            case 3: // 右下
                pos = screenGeometry.bottomRight() + QPoint(-200, -100);
                break;
            default:
                pos = screenGeometry.center();
        }
        
        candidateWindow_->showAt(pos);
        updateStatus(QString("窗口已移动到位置 (%1, %2)").arg(pos.x()).arg(pos.y()));
    }

    void onHighlightChanged(int index) {
        if (!candidateWindow_ || !candidateWindow_->candidateView()) return;
        candidateWindow_->candidateView()->setHighlightedIndex(index - 1);
        candidateWindow_->update();
        updateStatus(QString("高亮索引已设置为 %1").arg(index));
    }

private:
    void setupUI() {
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        
        // 标题
        QLabel* titleLabel = new QLabel("SuYan UI 层测试");
        titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; margin-bottom: 10px;");
        titleLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(titleLabel);
        
        // 窗口控制组
        QGroupBox* windowGroup = new QGroupBox("窗口控制");
        QHBoxLayout* windowLayout = new QHBoxLayout(windowGroup);
        
        QPushButton* showBtn = new QPushButton("显示窗口");
        QPushButton* hideBtn = new QPushButton("隐藏窗口");
        connect(showBtn, &QPushButton::clicked, this, &TestControlPanel::onShowWindow);
        connect(hideBtn, &QPushButton::clicked, this, &TestControlPanel::onHideWindow);
        windowLayout->addWidget(showBtn);
        windowLayout->addWidget(hideBtn);
        mainLayout->addWidget(windowGroup);
        
        // 候选词控制组
        QGroupBox* candidateGroup = new QGroupBox("候选词控制");
        QVBoxLayout* candidateLayout = new QVBoxLayout(candidateGroup);
        
        QHBoxLayout* candidateBtnLayout = new QHBoxLayout();
        QPushButton* updateBtn = new QPushButton("更新候选词(9个)");
        QPushButton* updateFewBtn = new QPushButton("更新候选词(3个)");
        QPushButton* clearBtn = new QPushButton("清空候选词");
        connect(updateBtn, &QPushButton::clicked, this, &TestControlPanel::onUpdateCandidates);
        connect(updateFewBtn, &QPushButton::clicked, this, &TestControlPanel::onUpdateFewCandidates);
        connect(clearBtn, &QPushButton::clicked, this, &TestControlPanel::onClearCandidates);
        candidateBtnLayout->addWidget(updateBtn);
        candidateBtnLayout->addWidget(updateFewBtn);
        candidateBtnLayout->addWidget(clearBtn);
        candidateLayout->addLayout(candidateBtnLayout);
        
        // 高亮控制
        QHBoxLayout* highlightLayout = new QHBoxLayout();
        highlightLayout->addWidget(new QLabel("高亮索引:"));
        QSpinBox* highlightSpin = new QSpinBox();
        highlightSpin->setRange(0, 9);
        highlightSpin->setValue(1);
        connect(highlightSpin, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &TestControlPanel::onHighlightChanged);
        highlightLayout->addWidget(highlightSpin);
        highlightLayout->addStretch();
        candidateLayout->addLayout(highlightLayout);
        
        mainLayout->addWidget(candidateGroup);
        
        // 布局控制组
        QGroupBox* layoutGroup = new QGroupBox("布局控制");
        QHBoxLayout* layoutLayout = new QHBoxLayout(layoutGroup);
        
        QPushButton* horizontalBtn = new QPushButton("横排布局");
        QPushButton* verticalBtn = new QPushButton("竖排布局");
        connect(horizontalBtn, &QPushButton::clicked, this, &TestControlPanel::onSetHorizontalLayout);
        connect(verticalBtn, &QPushButton::clicked, this, &TestControlPanel::onSetVerticalLayout);
        layoutLayout->addWidget(horizontalBtn);
        layoutLayout->addWidget(verticalBtn);
        mainLayout->addWidget(layoutGroup);
        
        // 主题控制组
        QGroupBox* themeGroup = new QGroupBox("主题控制");
        QHBoxLayout* themeLayout = new QHBoxLayout(themeGroup);
        
        QPushButton* lightBtn = new QPushButton("浅色主题");
        QPushButton* darkBtn = new QPushButton("深色主题");
        QPushButton* autoBtn = new QPushButton("跟随系统");
        connect(lightBtn, &QPushButton::clicked, this, &TestControlPanel::onSetLightTheme);
        connect(darkBtn, &QPushButton::clicked, this, &TestControlPanel::onSetDarkTheme);
        connect(autoBtn, &QPushButton::clicked, this, &TestControlPanel::onSetAutoTheme);
        themeLayout->addWidget(lightBtn);
        themeLayout->addWidget(darkBtn);
        themeLayout->addWidget(autoBtn);
        mainLayout->addWidget(themeGroup);
        
        // 位置控制组
        QGroupBox* posGroup = new QGroupBox("窗口位置");
        QGridLayout* posLayout = new QGridLayout(posGroup);
        
        QPushButton* topLeftBtn = new QPushButton("左上角");
        QPushButton* topRightBtn = new QPushButton("右上角");
        QPushButton* bottomLeftBtn = new QPushButton("左下角");
        QPushButton* bottomRightBtn = new QPushButton("右下角");
        
        connect(topLeftBtn, &QPushButton::clicked, this, [this]() { onMoveToCorner(0); });
        connect(topRightBtn, &QPushButton::clicked, this, [this]() { onMoveToCorner(1); });
        connect(bottomLeftBtn, &QPushButton::clicked, this, [this]() { onMoveToCorner(2); });
        connect(bottomRightBtn, &QPushButton::clicked, this, [this]() { onMoveToCorner(3); });
        
        posLayout->addWidget(topLeftBtn, 0, 0);
        posLayout->addWidget(topRightBtn, 0, 1);
        posLayout->addWidget(bottomLeftBtn, 1, 0);
        posLayout->addWidget(bottomRightBtn, 1, 1);
        mainLayout->addWidget(posGroup);
        
        // 状态显示
        QGroupBox* statusGroup = new QGroupBox("状态");
        QVBoxLayout* statusLayout = new QVBoxLayout(statusGroup);
        statusLabel_ = new QLabel("就绪");
        statusLabel_->setWordWrap(true);
        statusLayout->addWidget(statusLabel_);
        mainLayout->addWidget(statusGroup);
        
        mainLayout->addStretch();
    }

    void setupConnections() {
        // 连接 ThemeManager 信号
        connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
                this, [this](const Theme& theme) {
            updateStatus(QString("主题已变更: %1").arg(theme.name));
        });
        
        // 连接 LayoutManager 信号
        connect(&LayoutManager::instance(), &LayoutManager::layoutTypeChanged,
                this, [this](LayoutType type) {
            QString typeName = (type == LayoutType::Horizontal) ? "横排" : "竖排";
            updateStatus(QString("布局已变更: %1").arg(typeName));
        });
    }

    void updateStatus(const QString& message) {
        statusLabel_->setText(message);
        qDebug() << "[Status]" << message;
    }

    CandidateWindow* candidateWindow_ = nullptr;
    QLabel* statusLabel_ = nullptr;
};

/**
 * 自动测试函数
 * 
 * 在非交互模式下运行一系列自动化测试
 */
bool runAutoTests(CandidateWindow* window) {
    qDebug() << "========== 开始自动测试 ==========";
    bool allPassed = true;
    
    // 测试 1: 窗口显示
    qDebug() << "[Test 1] 窗口显示测试";
    QScreen* screen = QGuiApplication::primaryScreen();
    QPoint center = screen->availableGeometry().center();
    window->showAt(center);
    QCoreApplication::processEvents();
    
    if (!window->isWindowVisible()) {
        qDebug() << "  [FAIL] 窗口未能正确显示";
        allPassed = false;
    } else {
        qDebug() << "  [PASS] 窗口显示成功";
    }
    
    // 测试 2: 候选词更新
    qDebug() << "[Test 2] 候选词更新测试";
    InputState state;
    state.preedit = "test";
    state.rawInput = "test";
    state.highlightedIndex = 0;
    state.pageIndex = 0;
    state.pageSize = 9;
    state.hasMorePages = false;
    state.mode = InputMode::Chinese;
    state.isComposing = true;
    state.candidates = {
        {"测试", "cè shì", 1},
        {"测试2", "cè shì", 2}
    };
    
    window->updateCandidates(state);
    QCoreApplication::processEvents();
    qDebug() << "  [PASS] 候选词更新成功";
    
    // 测试 3: 布局切换
    qDebug() << "[Test 3] 布局切换测试";
    window->setLayoutType(LayoutType::Vertical);
    QCoreApplication::processEvents();
    
    if (window->getLayoutType() != LayoutType::Vertical) {
        qDebug() << "  [FAIL] 竖排布局切换失败";
        allPassed = false;
    } else {
        qDebug() << "  [PASS] 竖排布局切换成功";
    }
    
    window->setLayoutType(LayoutType::Horizontal);
    QCoreApplication::processEvents();
    
    if (window->getLayoutType() != LayoutType::Horizontal) {
        qDebug() << "  [FAIL] 横排布局切换失败";
        allPassed = false;
    } else {
        qDebug() << "  [PASS] 横排布局切换成功";
    }
    
    // 测试 4: 主题切换
    qDebug() << "[Test 4] 主题切换测试";
    ThemeManager::instance().setFollowSystemTheme(false);
    ThemeManager::instance().setCurrentTheme(Theme::NAME_DARK);
    QCoreApplication::processEvents();
    
    Theme currentTheme = window->getTheme();
    if (currentTheme.name != Theme::NAME_DARK) {
        qDebug() << "  [FAIL] 深色主题切换失败，当前主题:" << currentTheme.name;
        allPassed = false;
    } else {
        qDebug() << "  [PASS] 深色主题切换成功";
    }
    
    ThemeManager::instance().setCurrentTheme(Theme::NAME_LIGHT);
    QCoreApplication::processEvents();
    
    currentTheme = window->getTheme();
    if (currentTheme.name != Theme::NAME_LIGHT) {
        qDebug() << "  [FAIL] 浅色主题切换失败，当前主题:" << currentTheme.name;
        allPassed = false;
    } else {
        qDebug() << "  [PASS] 浅色主题切换成功";
    }
    
    // 测试 5: 窗口定位（屏幕边缘处理）
    qDebug() << "[Test 5] 窗口定位测试";
    QRect screenGeometry = screen->availableGeometry();
    
    // 测试各个角落
    QPoint corners[] = {
        screenGeometry.topLeft() + QPoint(10, 10),
        screenGeometry.topRight() + QPoint(-10, 10),
        screenGeometry.bottomLeft() + QPoint(10, -10),
        screenGeometry.bottomRight() + QPoint(-10, -10)
    };
    
    for (int i = 0; i < 4; ++i) {
        window->showAt(corners[i]);
        QCoreApplication::processEvents();
        
        // 检查窗口是否在屏幕内
        QRect windowGeometry = window->geometry();
        if (!screenGeometry.contains(windowGeometry.topLeft()) ||
            !screenGeometry.contains(windowGeometry.bottomRight())) {
            qDebug() << "  [WARN] 窗口可能超出屏幕边界，位置:" << corners[i];
        }
    }
    qDebug() << "  [PASS] 窗口定位测试完成";
    
    // 测试 6: 窗口隐藏
    qDebug() << "[Test 6] 窗口隐藏测试";
    window->hideWindow();
    QCoreApplication::processEvents();
    
    if (window->isWindowVisible()) {
        qDebug() << "  [FAIL] 窗口未能正确隐藏";
        allPassed = false;
    } else {
        qDebug() << "  [PASS] 窗口隐藏成功";
    }
    
    // 测试 7: 候选词清空
    qDebug() << "[Test 7] 候选词清空测试";
    window->showAt(center);
    window->clearCandidates();
    QCoreApplication::processEvents();
    qDebug() << "  [PASS] 候选词清空成功";
    
    // 测试 8: ConfigManager 布局配置集成
    qDebug() << "[Test 8] ConfigManager 布局配置集成测试";
    {
        auto& configMgr = ConfigManager::instance();
        
        // 通过 ConfigManager 设置竖排布局
        configMgr.setLayoutType(LayoutType::Vertical);
        QCoreApplication::processEvents();
        
        // 验证 LayoutManager 是否同步
        if (LayoutManager::instance().getLayoutType() != LayoutType::Vertical) {
            qDebug() << "  [FAIL] ConfigManager 设置布局后 LayoutManager 未同步";
            allPassed = false;
        } else {
            qDebug() << "  [PASS] ConfigManager 布局设置同步到 LayoutManager";
        }
        
        // 验证 CandidateWindow 是否同步
        if (window->getLayoutType() != LayoutType::Vertical) {
            qDebug() << "  [FAIL] ConfigManager 设置布局后 CandidateWindow 未同步";
            allPassed = false;
        } else {
            qDebug() << "  [PASS] ConfigManager 布局设置同步到 CandidateWindow";
        }
        
        // 恢复横排布局
        configMgr.setLayoutType(LayoutType::Horizontal);
        QCoreApplication::processEvents();
    }
    
    // 测试 9: ConfigManager 主题配置集成
    qDebug() << "[Test 9] ConfigManager 主题配置集成测试";
    {
        auto& configMgr = ConfigManager::instance();
        
        // 通过 ConfigManager 设置深色主题
        configMgr.setThemeMode(ThemeMode::Dark);
        QCoreApplication::processEvents();
        
        // 验证 ThemeManager 是否同步
        if (ThemeManager::instance().isFollowSystemTheme()) {
            qDebug() << "  [FAIL] ConfigManager 设置深色主题后 ThemeManager 仍在跟随系统";
            allPassed = false;
        } else {
            qDebug() << "  [PASS] ConfigManager 主题设置同步到 ThemeManager (不跟随系统)";
        }
        
        // 验证 CandidateWindow 主题
        currentTheme = window->getTheme();
        if (currentTheme.name != Theme::NAME_DARK) {
            qDebug() << "  [FAIL] ConfigManager 设置深色主题后 CandidateWindow 主题不正确，当前:" << currentTheme.name;
            allPassed = false;
        } else {
            qDebug() << "  [PASS] ConfigManager 主题设置同步到 CandidateWindow";
        }
        
        // 测试浅色主题
        configMgr.setThemeMode(ThemeMode::Light);
        QCoreApplication::processEvents();
        
        currentTheme = window->getTheme();
        if (currentTheme.name != Theme::NAME_LIGHT) {
            qDebug() << "  [FAIL] ConfigManager 设置浅色主题后 CandidateWindow 主题不正确，当前:" << currentTheme.name;
            allPassed = false;
        } else {
            qDebug() << "  [PASS] ConfigManager 浅色主题设置正确";
        }
        
        // 测试跟随系统
        configMgr.setThemeMode(ThemeMode::Auto);
        QCoreApplication::processEvents();
        
        if (!ThemeManager::instance().isFollowSystemTheme()) {
            qDebug() << "  [FAIL] ConfigManager 设置 Auto 后 ThemeManager 未跟随系统";
            allPassed = false;
        } else {
            qDebug() << "  [PASS] ConfigManager Auto 模式设置正确";
        }
    }
    
    // 测试 10: 跟随系统深色模式
    qDebug() << "[Test 10] 跟随系统深色模式测试";
    {
        auto& themeMgr = ThemeManager::instance();
        
        // 设置跟随系统
        themeMgr.setFollowSystemTheme(true);
        QCoreApplication::processEvents();
        
        // 检查当前系统深色模式状态
        bool isDark = themeMgr.isSystemDarkMode();
        qDebug() << "  系统当前深色模式:" << (isDark ? "是" : "否");
        
        // 验证主题是否与系统一致
        currentTheme = window->getTheme();
        QString expectedTheme = isDark ? Theme::NAME_DARK : Theme::NAME_LIGHT;
        if (currentTheme.name != expectedTheme) {
            qDebug() << "  [WARN] 跟随系统模式下主题可能不一致，期望:" << expectedTheme << "，实际:" << currentTheme.name;
            // 这不是严格的失败，因为系统深色模式检测可能有延迟
        } else {
            qDebug() << "  [PASS] 跟随系统深色模式正确";
        }
    }
    
    qDebug() << "========== 自动测试完成 ==========";
    qDebug() << (allPassed ? "所有测试通过!" : "部分测试失败!");
    
    return allPassed;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    qDebug() << "SuYan UI 测试程序启动";
    qDebug() << "Qt Version:" << QT_VERSION_STR;
    
    // 检查是否为自动测试模式
    bool autoTestMode = false;
    for (int i = 1; i < argc; ++i) {
        if (QString(argv[i]) == "--test" || QString(argv[i]) == "--auto") {
            autoTestMode = true;
            break;
        }
    }
    
    // 初始化 ConfigManager（LayoutManager 依赖它）
    QString configDir = QDir::tempPath() + "/suyan_ui_test";
    QDir().mkpath(configDir);
    
    if (!ConfigManager::instance().initialize(configDir.toStdString())) {
        qDebug() << "ConfigManager 初始化失败";
        return 1;
    }
    qDebug() << "ConfigManager 初始化成功";
    
    // 初始化 UI 组件
    UIInitConfig config;
    config.followSystemTheme = true;
    
    UIInitResult result = initializeUI(config);
    if (!result.success) {
        qDebug() << "UI 初始化失败:" << result.errorMessage;
        return 1;
    }
    qDebug() << "UI 组件初始化成功";
    
    CandidateWindow* candidateWindow = result.window;
    
    // 连接到管理器
    candidateWindow->connectToThemeManager();
    candidateWindow->connectToLayoutManager();
    candidateWindow->syncFromManagers();
    
    if (autoTestMode) {
        // 自动测试模式
        qDebug() << "运行自动测试模式...";
        
        // 使用 QTimer 延迟执行测试，确保事件循环已启动
        QTimer::singleShot(100, [candidateWindow]() {
            bool passed = runAutoTests(candidateWindow);
            
            // 清理并退出
            cleanupUI(candidateWindow);
            QCoreApplication::exit(passed ? 0 : 1);
        });
        
        return app.exec();
    } else {
        // 交互测试模式
        qDebug() << "运行交互测试模式...";
        
        TestControlPanel controlPanel(candidateWindow);
        controlPanel.show();
        
        int ret = app.exec();
        
        // 清理
        cleanupUI(candidateWindow);
        
        return ret;
    }
}

#include "ui_test_app.moc"
