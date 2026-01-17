// src/platform/windows/weasel_handler_ext.h
// Weasel 处理器扩展 - 扩展 RimeWithWeaselHandler 以集成我们的核心功能

#ifndef IME_WEASEL_HANDLER_EXT_H
#define IME_WEASEL_HANDLER_EXT_H

#ifdef PLATFORM_WINDOWS

#include <string>
#include <vector>
#include <memory>

// 前向声明
namespace weasel {
class UI;
struct CandidateInfo;
}

namespace ime {
namespace platform {
namespace windows {

class WeaselIntegration;

// Weasel 处理器扩展
// 提供钩子函数，在 RimeWithWeasel 的关键点注入我们的逻辑
class WeaselHandlerExtension {
public:
    // 获取单例实例
    static WeaselHandlerExtension& Instance();

    // 禁止拷贝
    WeaselHandlerExtension(const WeaselHandlerExtension&) = delete;
    WeaselHandlerExtension& operator=(const WeaselHandlerExtension&) = delete;

    // ==================== 生命周期钩子 ====================

    // 在 RimeWithWeaselHandler::Initialize 之后调用
    void OnInitialize(const wchar_t* userDataPath,
                      const wchar_t* sharedDataPath);

    // 在 RimeWithWeaselHandler::Finalize 之前调用
    void OnFinalize();

    // ==================== 候选词处理钩子 ====================

    // 在获取候选词之后调用，用于合并用户高频词
    // @param candidates 原始候选词列表（会被修改）
    // @param pinyin 当前输入的拼音
    void OnCandidatesReady(std::vector<std::string>& candidates,
                           const std::string& pinyin);

    // 在用户选择候选词之后调用
    // @param word 选择的词
    // @param pinyin 对应的拼音
    // @param index 选择的索引
    void OnCandidateSelected(const std::string& word,
                             const std::string& pinyin,
                             int index);

    // 在提交文本之后调用
    // @param text 提交的文本
    void OnTextCommitted(const std::string& text);

    // ==================== 输入模式钩子 ====================

    // 获取当前输入模式（用于同步状态）
    // @return 0=中文, 1=英文
    int GetInputMode() const;

    // 设置输入模式
    // @param mode 0=中文, 1=英文
    void SetInputMode(int mode);

    // 切换输入模式
    void ToggleInputMode();

    // ==================== 状态查询 ====================

    // 检查是否已初始化
    bool IsInitialized() const { return initialized_; }

private:
    WeaselHandlerExtension();
    ~WeaselHandlerExtension();

    bool initialized_;
    std::string lastPinyin_;  // 上次输入的拼音（用于自动学词）
};

// ==================== 辅助函数 ====================

// UTF-8 到 UTF-16 转换
std::wstring Utf8ToUtf16(const std::string& utf8);

// UTF-16 到 UTF-8 转换
std::string Utf16ToUtf8(const std::wstring& utf16);

}  // namespace windows
}  // namespace platform
}  // namespace ime

#endif  // PLATFORM_WINDOWS

#endif  // IME_WEASEL_HANDLER_EXT_H
