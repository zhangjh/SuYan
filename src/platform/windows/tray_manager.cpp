/**
 * TrayManager 实现
 * Task 1.1: Windows 平台目录结构
 * 
 * 占位符实现，将在 Task 7 中完成
 */

#ifdef _WIN32

#include "tray_manager.h"
#include <QApplication>

namespace suyan {

TrayManager& TrayManager::instance() {
    static TrayManager instance;
    return instance;
}

TrayManager::TrayManager() = default;

TrayManager::~TrayManager() {
    shutdown();
}

bool TrayManager::initialize(const std::string& resourcePath) {
    // TODO: Task 7.1 实现
    if (initialized_) {
        return true;
    }
    
    resourcePath_ = resourcePath;
    
    // 检查系统托盘是否可用
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        return false;
    }
    
    // 创建托盘图标
    trayIcon_ = new QSystemTrayIcon(this);
    
    // 创建菜单
    createMenu();
    
    // 连接信号
    connect(trayIcon_, &QSystemTrayIcon::activated,
            this, &TrayManager::onTrayActivated);
    
    initialized_ = true;
    return true;
}

void TrayManager::shutdown() {
    // TODO: Task 7.1 实现
    if (!initialized_) {
        return;
    }
    
    if (trayIcon_) {
        trayIcon_->hide();
        delete trayIcon_;
        trayIcon_ = nullptr;
    }
    
    if (trayMenu_) {
        delete trayMenu_;
        trayMenu_ = nullptr;
    }
    
    initialized_ = false;
}

void TrayManager::updateIcon(InputMode mode) {
    // TODO: Task 7.3 实现
}

void TrayManager::show() {
    if (trayIcon_) {
        trayIcon_->show();
    }
}

void TrayManager::hide() {
    if (trayIcon_) {
        trayIcon_->hide();
    }
}

void TrayManager::createMenu() {
    // TODO: Task 7.2 实现
    trayMenu_ = new QMenu();
    
    // 切换中/英文
    toggleModeAction_ = trayMenu_->addAction(QString::fromUtf8("切换中/英文"));
    connect(toggleModeAction_, &QAction::triggered, this, &TrayManager::onToggleMode);
    
    // 设置（暂时禁用）
    settingsAction_ = trayMenu_->addAction(QString::fromUtf8("设置..."));
    settingsAction_->setEnabled(false);
    connect(settingsAction_, &QAction::triggered, this, &TrayManager::onOpenSettings);
    
    trayMenu_->addSeparator();
    
    // 关于
    aboutAction_ = trayMenu_->addAction(QString::fromUtf8("关于素言"));
    connect(aboutAction_, &QAction::triggered, this, &TrayManager::onShowAbout);
    
    // 退出
    exitAction_ = trayMenu_->addAction(QString::fromUtf8("退出"));
    connect(exitAction_, &QAction::triggered, this, &TrayManager::onExit);
    
    if (trayIcon_) {
        trayIcon_->setContextMenu(trayMenu_);
    }
}

QIcon TrayManager::loadIcon(const QString& name) {
    // TODO: Task 7.3 实现
    QString iconPath = QString::fromStdString(resourcePath_) + "/" + name;
    return QIcon(iconPath);
}

void TrayManager::onTrayActivated(QSystemTrayIcon::ActivationReason reason) {
    // TODO: Task 7.2 实现
    if (reason == QSystemTrayIcon::Trigger) {
        // 单击切换模式
        emit toggleModeRequested();
    }
}

void TrayManager::onToggleMode() {
    emit toggleModeRequested();
}

void TrayManager::onOpenSettings() {
    emit openSettingsRequested();
}

void TrayManager::onShowAbout() {
    emit showAboutRequested();
}

void TrayManager::onExit() {
    emit exitRequested();
}

} // namespace suyan

#endif // _WIN32
