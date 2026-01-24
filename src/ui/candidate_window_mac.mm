/**
 * CandidateWindow macOS 特定实现
 *
 * 使用 Objective-C++ 设置 NSWindow 级别，确保候选词窗口
 * 在所有应用窗口之上显示，包括全屏应用。
 *
 * 关键技术点：
 * 1. 窗口级别：使用 NSPopUpMenuWindowLevel + 1 确保在全屏应用上方
 * 2. 窗口行为：设置 FullScreenAuxiliary 支持全屏场景
 * 3. 焦点控制：确保窗口不获取键盘焦点
 * 4. 事件循环：Qt 与 IMK RunLoop 的兼容处理
 */

#include "candidate_window.h"

#ifdef Q_OS_MACOS

#import <Cocoa/Cocoa.h>
#import <AppKit/AppKit.h>
#import <CoreGraphics/CoreGraphics.h>

// ========== Objective-C 类定义（必须在全局作用域）==========

/**
 * 自定义 NSWindow 子类，用于输入法候选词窗口
 *
 * 重写关键方法确保窗口不获取焦点
 * 注意：这个类目前未使用，因为 Qt 创建的 NSWindow 无法直接替换
 * 保留此类作为参考，说明输入法窗口应有的行为
 */
@interface SuYanCandidateNSWindow : NSWindow
@end

@implementation SuYanCandidateNSWindow

// 永远不成为 key window（不接收键盘输入）
- (BOOL)canBecomeKeyWindow {
    return NO;
}

// 永远不成为 main window
- (BOOL)canBecomeMainWindow {
    return NO;
}

// 不接受第一响应者
- (BOOL)acceptsFirstResponder {
    return NO;
}

// 鼠标事件不激活窗口
- (BOOL)acceptsMouseMovedEvents {
    return NO;
}

@end

// ========== C++ 命名空间实现 ==========

namespace suyan {

/**
 * 确保窗口在全屏应用中正确显示（内部辅助函数）
 *
 * 这个函数在每次显示窗口时调用，以处理某些边缘情况
 */
static void ensureWindowVisibleInFullScreenInternal(NSWindow* window) {
    if (!window) {
        return;
    }
    
    // 检查当前是否有全屏应用
    NSArray<NSWindow*>* windows = [NSApp windows];
    BOOL hasFullScreenApp = NO;
    
    for (NSWindow* w in windows) {
        if ((w.styleMask & NSWindowStyleMaskFullScreen) != 0) {
            hasFullScreenApp = YES;
            break;
        }
    }
    
    // 如果有全屏应用，确保窗口级别足够高
    if (hasFullScreenApp) {
        NSInteger currentLevel = [window level];
        if (currentLevel < NSPopUpMenuWindowLevel + 1) {
            [window setLevel:NSPopUpMenuWindowLevel + 1];
        }
    }
    
    // 强制窗口到最前
    [window orderFrontRegardless];
}

/**
 * 设置 macOS 窗口级别
 *
 * 配置窗口以满足输入法候选词窗口的特殊需求：
 * 1. 显示在所有普通窗口之上（包括全屏应用）
 * 2. 不获取键盘焦点
 * 3. 在所有桌面空间可见
 * 4. 与 Qt 事件循环兼容
 */
void CandidateWindow::setupMacOSWindowLevel() {
    // 获取 Qt 窗口的 native handle
    WId winId = this->winId();
    if (winId == 0) {
        return;
    }
    
    // 将 WId 转换为 NSView
    NSView* view = reinterpret_cast<NSView*>(winId);
    if (!view) {
        return;
    }
    
    // 获取 NSWindow
    NSWindow* window = [view window];
    if (!window) {
        return;
    }
    
    // ========== 窗口级别设置 ==========
    // 
    // macOS 窗口级别层次（从低到高）：
    // - NSNormalWindowLevel (0): 普通窗口
    // - NSFloatingWindowLevel (3): 浮动窗口
    // - NSSubmenuWindowLevel (3): 子菜单
    // - NSTornOffMenuWindowLevel (3): 撕下的菜单
    // - NSMainMenuWindowLevel (24): 主菜单栏
    // - NSStatusWindowLevel (25): 状态栏
    // - NSModalPanelWindowLevel (8): 模态面板
    // - NSPopUpMenuWindowLevel (101): 弹出菜单
    // - NSScreenSaverWindowLevel (1000): 屏幕保护
    // - CGShieldingWindowLevel(): 屏蔽窗口（最高）
    //
    // 对于输入法候选词窗口，我们需要：
    // - 高于普通应用窗口
    // - 高于全屏应用
    // - 低于屏幕保护程序
    //
    // 使用 NSPopUpMenuWindowLevel + 1 可以确保在几乎所有窗口之上
    
    NSInteger candidateWindowLevel = NSPopUpMenuWindowLevel + 1;
    [window setLevel:candidateWindowLevel];
    
    // ========== 窗口集合行为设置 ==========
    //
    // NSWindowCollectionBehavior 控制窗口在多桌面/全屏环境下的行为：
    //
    // - CanJoinAllSpaces: 窗口在所有桌面空间可见
    //   输入法窗口需要在任何桌面空间都能显示
    //
    // - FullScreenAuxiliary: 窗口可以在全屏应用上方显示
    //   这是支持全屏应用输入的关键设置
    //
    // - Stationary: 切换桌面空间时窗口不移动
    //   保持候选词窗口位置稳定
    //
    // - IgnoresCycle: 不参与 Cmd+` 窗口循环
    //   输入法窗口不应该被用户通过快捷键切换到
    //
    // - Transient: 窗口是临时的，不保存在窗口恢复中
    //   输入法窗口不需要在应用重启后恢复
    
    NSWindowCollectionBehavior behavior = 
        NSWindowCollectionBehaviorCanJoinAllSpaces |
        NSWindowCollectionBehaviorFullScreenAuxiliary |
        NSWindowCollectionBehaviorStationary |
        NSWindowCollectionBehaviorIgnoresCycle |
        NSWindowCollectionBehaviorTransient;
    
    [window setCollectionBehavior:behavior];
    
    // ========== 焦点控制设置 ==========
    //
    // 确保窗口不会获取焦点，不影响当前应用的输入
    
    // 不在应用切换时隐藏
    [window setHidesOnDeactivate:NO];
    
    // 设置窗口样式：无标题栏、不可调整大小
    // 注意：Qt 已经设置了 FramelessWindowHint，这里确保 NSWindow 也正确配置
    [window setStyleMask:NSWindowStyleMaskBorderless];
    
    // 设置窗口背景颜色为透明（配合 Qt::WA_TranslucentBackground）
    [window setBackgroundColor:[NSColor clearColor]];
    
    // ========== 阴影设置 ==========
    //
    // 输入法候选词窗口通常需要阴影来与背景区分
    [window setHasShadow:YES];
    
    // ========== 动画设置 ==========
    //
    // 禁用窗口动画，使候选词窗口显示/隐藏更快速
    [window setAnimationBehavior:NSWindowAnimationBehaviorNone];
    
    // ========== 释放策略 ==========
    //
    // 设置窗口关闭时不自动释放（由 Qt 管理生命周期）
    [window setReleasedWhenClosed:NO];
}

/**
 * CandidateWindow::ensureVisibleInFullScreen 实现
 *
 * 公开方法，供 showAt 调用
 */
void CandidateWindow::ensureVisibleInFullScreen() {
    WId winId = this->winId();
    if (winId == 0) {
        return;
    }
    
    NSView* view = reinterpret_cast<NSView*>(winId);
    if (!view) {
        return;
    }
    
    NSWindow* window = [view window];
    ensureWindowVisibleInFullScreenInternal(window);
}

} // namespace suyan

#endif // Q_OS_MACOS
