/**
 * MacOSBridge - macOS 平台桥接实现
 *
 * 实现 IPlatformBridge 接口，通过 Objective-C 调用 IMK client API。
 */

#import <Cocoa/Cocoa.h>
#import <InputMethodKit/InputMethodKit.h>

#include "macos_bridge.h"

namespace suyan {

// ========== 构造与析构 ==========

MacOSBridge::MacOSBridge()
    : currentClient_(nil) {
}

MacOSBridge::~MacOSBridge() {
    currentClient_ = nil;
}

// ========== IPlatformBridge 接口实现 ==========

void MacOSBridge::commitText(const std::string& text) {
    if (!currentClient_ || text.empty()) {
        return;
    }

    @autoreleasepool {
        id client = (__bridge id)currentClient_;
        NSString* nsText = [NSString stringWithUTF8String:text.c_str()];
        
        if (!nsText) {
            NSLog(@"MacOSBridge: Failed to convert text to NSString");
            return;
        }

        @try {
            // 使用 insertText:replacementRange: 提交文字
            // NSNotFound 表示在当前位置插入
            [client insertText:nsText
              replacementRange:NSMakeRange(NSNotFound, NSNotFound)];
        } @catch (NSException* exception) {
            NSLog(@"MacOSBridge: Failed to commit text: %@", exception);
        }
    }
}

CursorPosition MacOSBridge::getCursorPosition() {
    CursorPosition pos;
    pos.x = 100;      // 默认位置
    pos.y = 100;
    pos.height = 20;

    if (!currentClient_) {
        return pos;
    }

    @autoreleasepool {
        id client = (__bridge id)currentClient_;
        NSRect cursorRect = NSZeroRect;

        @try {
            // 获取光标位置矩形
            // attributesForCharacterIndex:lineHeightRectangle: 返回字符属性
            // 并通过 lineHeightRectangle 参数返回光标位置
            [client attributesForCharacterIndex:0 
                            lineHeightRectangle:&cursorRect];
        } @catch (NSException* exception) {
            NSLog(@"MacOSBridge: Failed to get cursor position: %@", exception);
            return pos;
        }

        // macOS 坐标系：原点在左下角
        // Qt 坐标系：原点在左上角
        // 需要进行坐标转换
        NSScreen* screen = [NSScreen mainScreen];
        if (screen) {
            CGFloat screenHeight = screen.frame.size.height;
            
            // 转换 Y 坐标：screenHeight - y - height
            // 候选词窗口应该显示在光标下方
            pos.x = static_cast<int>(cursorRect.origin.x);
            pos.y = static_cast<int>(screenHeight - cursorRect.origin.y - cursorRect.size.height);
            pos.height = static_cast<int>(cursorRect.size.height);
        } else {
            pos.x = static_cast<int>(cursorRect.origin.x);
            pos.y = static_cast<int>(cursorRect.origin.y);
            pos.height = static_cast<int>(cursorRect.size.height);
        }
    }

    return pos;
}

void MacOSBridge::updatePreedit(const std::string& preedit, int caretPos) {
    if (!currentClient_) {
        return;
    }

    @autoreleasepool {
        id client = (__bridge id)currentClient_;
        NSString* nsPreedit = [NSString stringWithUTF8String:preedit.c_str()];
        
        if (!nsPreedit) {
            nsPreedit = @"";
        }

        @try {
            // 创建带下划线的属性字符串
            NSDictionary* attrs = @{
                NSUnderlineStyleAttributeName: @(NSUnderlineStyleSingle),
                NSForegroundColorAttributeName: [NSColor textColor]
            };
            NSAttributedString* attrString = [[NSAttributedString alloc]
                initWithString:nsPreedit
                attributes:attrs];

            // 设置 marked text（预编辑文本）
            // selectionRange: 光标位置，长度为 0 表示插入点
            // replacementRange: NSNotFound 表示替换当前 marked text
            [client setMarkedText:attrString
                   selectionRange:NSMakeRange(static_cast<NSUInteger>(caretPos), 0)
                 replacementRange:NSMakeRange(NSNotFound, NSNotFound)];
        } @catch (NSException* exception) {
            NSLog(@"MacOSBridge: Failed to update preedit: %@", exception);
        }
    }
}

void MacOSBridge::clearPreedit() {
    if (!currentClient_) {
        return;
    }

    @autoreleasepool {
        id client = (__bridge id)currentClient_;

        @try {
            // 设置空的 marked text 来清除 preedit
            [client setMarkedText:@""
                   selectionRange:NSMakeRange(0, 0)
                 replacementRange:NSMakeRange(NSNotFound, NSNotFound)];
        } @catch (NSException* exception) {
            NSLog(@"MacOSBridge: Failed to clear preedit: %@", exception);
        }
    }
}

std::string MacOSBridge::getCurrentAppId() {
    if (!currentClient_) {
        return "";
    }

    @autoreleasepool {
        id client = (__bridge id)currentClient_;

        @try {
            // IMK client 提供 bundleIdentifier 方法获取当前应用的 Bundle ID
            if ([client respondsToSelector:@selector(bundleIdentifier)]) {
                NSString* bundleId = [client bundleIdentifier];
                if (bundleId) {
                    return std::string([bundleId UTF8String]);
                }
            }
        } @catch (NSException* exception) {
            NSLog(@"MacOSBridge: Failed to get bundle identifier: %@", exception);
        }
    }

    return "";
}

// ========== macOS 特定方法 ==========

void MacOSBridge::setClient(ClientHandle client) {
    currentClient_ = client;
}

ClientHandle MacOSBridge::getClient() const {
    return currentClient_;
}

bool MacOSBridge::hasValidClient() const {
    return currentClient_ != nil;
}

} // namespace suyan
