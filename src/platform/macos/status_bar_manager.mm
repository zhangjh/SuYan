/**
 * StatusBarManager - macOS 状态栏图标管理器实现
 */

#import <Cocoa/Cocoa.h>

#include "status_bar_manager.h"
#include "input_engine.h"

namespace suyan {

// ========== 单例实现 ==========

StatusBarManager& StatusBarManager::instance() {
    static StatusBarManager instance;
    return instance;
}

StatusBarManager::StatusBarManager() = default;

StatusBarManager::~StatusBarManager() = default;

// ========== 公共方法 ==========

bool StatusBarManager::initialize(const std::string& resourcePath) {
    if (initialized_) {
        return true;
    }

    resourcePath_ = resourcePath;
    initialized_ = true;
    
    NSLog(@"StatusBarManager: Initialized with resource path: %s", resourcePath.c_str());
    return true;
}

void StatusBarManager::updateIcon(InputMode mode) {
    switch (mode) {
        case InputMode::Chinese:
            setIconType(StatusIconType::Chinese);
            break;
        case InputMode::English:
        case InputMode::TempEnglish:
            setIconType(StatusIconType::English);
            break;
    }
}

void StatusBarManager::setIconType(StatusIconType type) {
    if (currentIconType_ == type) {
        return;
    }

    currentIconType_ = type;
    
    // macOS 输入法的状态栏图标由系统管理
    // 我们无法直接更改系统菜单栏中的输入法图标
    // 但可以通过以下方式提供视觉反馈：
    // 1. 在候选词窗口中显示当前模式
    // 2. 在输入法菜单中显示当前模式
    // 3. 使用 Dock 图标（但输入法通常隐藏 Dock 图标）
    
    NSLog(@"StatusBarManager: Icon type changed to %s", 
          type == StatusIconType::Chinese ? "Chinese" : "English");
}

StatusIconType StatusBarManager::getCurrentIconType() const {
    return currentIconType_;
}

std::string StatusBarManager::getModeText() const {
    switch (currentIconType_) {
        case StatusIconType::Chinese:
            return "中";
        case StatusIconType::English:
            return "A";
        case StatusIconType::Disabled:
            return "-";
    }
    return "?";
}

} // namespace suyan
