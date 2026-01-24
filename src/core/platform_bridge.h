/**
 * IPlatformBridge - 平台桥接接口
 *
 * 定义平台层需要实现的统一接口，供核心层调用。
 * macOS 和 Windows 分别实现此接口。
 */

#ifndef SUYAN_CORE_PLATFORM_BRIDGE_H
#define SUYAN_CORE_PLATFORM_BRIDGE_H

#include <string>

namespace suyan {

/**
 * 光标位置结构
 */
struct CursorPosition {
    int x = 0;      // 屏幕 X 坐标
    int y = 0;      // 屏幕 Y 坐标
    int height = 0; // 光标高度（用于候选词窗口定位）
};

/**
 * IPlatformBridge - 平台桥接接口
 *
 * 核心层通过此接口与平台层通信，实现：
 * - 文字提交到当前应用
 * - 获取光标位置
 * - 更新/清除 preedit
 * - 获取当前应用信息
 */
class IPlatformBridge {
public:
    virtual ~IPlatformBridge() = default;

    /**
     * 提交文字到当前应用
     *
     * @param text 要提交的文字（UTF-8 编码）
     */
    virtual void commitText(const std::string& text) = 0;

    /**
     * 获取当前光标位置（屏幕坐标）
     *
     * @return 光标位置信息
     */
    virtual CursorPosition getCursorPosition() = 0;

    /**
     * 更新 preedit（输入中的拼音显示在应用内）
     *
     * @param preedit 预编辑文本
     * @param caretPos 光标位置
     */
    virtual void updatePreedit(const std::string& preedit, int caretPos) = 0;

    /**
     * 清除 preedit
     */
    virtual void clearPreedit() = 0;

    /**
     * 获取当前应用的标识符
     *
     * macOS: Bundle Identifier (如 com.apple.Safari)
     * Windows: 进程名 (如 notepad.exe)
     *
     * @return 应用标识符
     */
    virtual std::string getCurrentAppId() = 0;
};

} // namespace suyan

#endif // SUYAN_CORE_PLATFORM_BRIDGE_H
