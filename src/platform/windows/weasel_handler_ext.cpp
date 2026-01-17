// src/platform/windows/weasel_handler_ext.cpp
// Weasel 处理器扩展实现

#ifdef PLATFORM_WINDOWS

#include "weasel_handler_ext.h"
#include "weasel_integration.h"

#include <windows.h>
#include <codecvt>
#include <locale>

namespace ime {
namespace platform {
namespace windows {

// ==================== 辅助函数实现 ====================

std::wstring Utf8ToUtf16(const std::string& utf8) {
    if (utf8.empty()) {
        return std::wstring();
    }

    int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(),
                                   static_cast<int>(utf8.size()),
                                   nullptr, 0);
    if (size <= 0) {
        return std::wstring();
    }

    std::wstring result(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(),
                        static_cast<int>(utf8.size()),
                        &result[0], size);
    return result;
}

std::string Utf16ToUtf8(const std::wstring& utf16) {
    if (utf16.empty()) {
        return std::string();
    }

    int size = WideCharToMultiByte(CP_UTF8, 0, utf16.c_str(),
                                   static_cast<int>(utf16.size()),
                                   nullptr, 0, nullptr, nullptr);
    if (size <= 0) {
        return std::string();
    }

    std::string result(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, utf16.c_str(),
                        static_cast<int>(utf16.size()),
                        &result[0], size, nullptr, nullptr);
    return result;
}

// ==================== WeaselHandlerExtension ====================

WeaselHandlerExtension& WeaselHandlerExtension::Instance() {
    static WeaselHandlerExtension instance;
    return instance;
}

WeaselHandlerExtension::WeaselHandlerExtension()
    : initialized_(false) {
}

WeaselHandlerExtension::~WeaselHandlerExtension() {
    OnFinalize();
}

void WeaselHandlerExtension::OnInitialize(const wchar_t* userDataPath,
                                          const wchar_t* sharedDataPath) {
    if (initialized_) {
        return;
    }

    WeaselIntegrationConfig config = WeaselIntegrationConfig::Default();
    if (userDataPath) {
        config.userDataPath = userDataPath;
    }
    if (sharedDataPath) {
        config.sharedDataPath = sharedDataPath;
    }

    initialized_ = WeaselIntegration::Instance().Initialize(config);
}

void WeaselHandlerExtension::OnFinalize() {
    if (!initialized_) {
        return;
    }

    WeaselIntegration::Instance().Shutdown();
    initialized_ = false;
}

void WeaselHandlerExtension::OnCandidatesReady(
    std::vector<std::string>& candidates,
    const std::string& pinyin) {
    
    if (!initialized_) {
        return;
    }

    // 保存拼音用于后续自动学词
    lastPinyin_ = pinyin;

    // 合并用户高频词
    candidates = WeaselIntegration::Instance().MergeCandidates(candidates, pinyin);
}

void WeaselHandlerExtension::OnCandidateSelected(const std::string& word,
                                                  const std::string& pinyin,
                                                  int index) {
    if (!initialized_) {
        return;
    }

    // 使用保存的拼音（如果传入的为空）
    std::string actualPinyin = pinyin.empty() ? lastPinyin_ : pinyin;

    // 记录连续选择（用于自动学词）
    WeaselIntegration::Instance().RecordConsecutiveSelection(word, actualPinyin);
}

void WeaselHandlerExtension::OnTextCommitted(const std::string& text) {
    if (!initialized_) {
        return;
    }

    // 触发自动学词检查
    WeaselIntegration::Instance().OnCommitComplete();

    // 清空上次拼音
    lastPinyin_.clear();
}

int WeaselHandlerExtension::GetInputMode() const {
    if (!initialized_) {
        return 0;  // 默认中文模式
    }

    auto mode = WeaselIntegration::Instance().GetInputMode();
    switch (mode) {
        case input::InputMode::English:
        case input::InputMode::TempEnglish:
            return 1;
        default:
            return 0;
    }
}

void WeaselHandlerExtension::SetInputMode(int mode) {
    if (!initialized_) {
        return;
    }

    input::InputMode inputMode = (mode == 0) 
        ? input::InputMode::Chinese 
        : input::InputMode::English;
    WeaselIntegration::Instance().SetInputMode(inputMode);
}

void WeaselHandlerExtension::ToggleInputMode() {
    if (!initialized_) {
        return;
    }

    WeaselIntegration::Instance().ToggleInputMode();
}

}  // namespace windows
}  // namespace platform
}  // namespace ime

#endif  // PLATFORM_WINDOWS
