/**
 * MacOSBridge - macOS 平台桥接实现
 *
 * 实现 IPlatformBridge 接口，提供 macOS 平台特定的功能：
 * - 通过 IMK client 提交文字
 * - 获取光标位置
 * - 更新/清除 preedit
 * - 获取当前应用 Bundle Identifier
 */

#ifndef SUYAN_PLATFORM_MACOS_MACOS_BRIDGE_H
#define SUYAN_PLATFORM_MACOS_MACOS_BRIDGE_H

#include "platform_bridge.h"

// 前向声明 Objective-C 类型
#ifdef __OBJC__
@class NSObject;
typedef NSObject* ClientHandle;
#else
typedef void* ClientHandle;
#endif

namespace suyan {

/**
 * MacOSBridge - macOS 平台桥接类
 *
 * 封装 macOS IMK 相关操作，实现 IPlatformBridge 接口。
 * 通过持有 IMK client 引用来与当前输入应用通信。
 */
class MacOSBridge : public IPlatformBridge {
public:
    MacOSBridge();
    ~MacOSBridge() override;

    // ========== IPlatformBridge 接口实现 ==========

    /**
     * 提交文字到当前应用
     *
     * 通过 IMK client 的 insertText:replacementRange: 方法提交文字
     *
     * @param text 要提交的文字（UTF-8 编码）
     */
    void commitText(const std::string& text) override;

    /**
     * 获取当前光标位置（屏幕坐标）
     *
     * 通过 IMK client 的 attributesForCharacterIndex:lineHeightRectangle: 获取
     *
     * @return 光标位置信息（屏幕坐标，已转换为 Qt 坐标系）
     */
    CursorPosition getCursorPosition() override;

    /**
     * 更新 preedit（输入中的拼音显示在应用内）
     *
     * 通过 IMK client 的 setMarkedText:selectionRange:replacementRange: 更新
     *
     * @param preedit 预编辑文本
     * @param caretPos 光标位置
     */
    void updatePreedit(const std::string& preedit, int caretPos) override;

    /**
     * 清除 preedit
     *
     * 通过设置空的 marked text 来清除
     */
    void clearPreedit() override;

    /**
     * 获取当前应用的 Bundle Identifier
     *
     * 通过 IMK client 的 bundleIdentifier 方法获取
     *
     * @return Bundle Identifier（如 com.apple.Safari），获取失败返回空字符串
     */
    std::string getCurrentAppId() override;

    // ========== macOS 特定方法 ==========

    /**
     * 设置当前 IMK client
     *
     * 由 SuYanInputController 在 handleEvent 和 activateServer 时调用
     *
     * @param client IMK client 对象（id 类型）
     */
    void setClient(ClientHandle client);

    /**
     * 获取当前 IMK client
     *
     * @return 当前 client，可能为 nil
     */
    ClientHandle getClient() const;

    /**
     * 检查是否有有效的 client
     *
     * @return true 如果 client 有效
     */
    bool hasValidClient() const;

private:
    ClientHandle currentClient_;
};

} // namespace suyan

#endif // SUYAN_PLATFORM_MACOS_MACOS_BRIDGE_H
