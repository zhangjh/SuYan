// src/core/input/input_session.cpp
// 输入会话管理实现

#include "input_session.h"

#include <algorithm>
#include <cctype>

#include "core/frequency/frequency_manager.h"
#include "core/storage/local_storage.h"

namespace ime {
namespace input {

// ==================== KeyEvent 实现 ====================

KeyEvent KeyEvent::FromChar(char c) {
    KeyEvent event;
    event.character = c;

    if (std::isalpha(c)) {
        event.type = KeyType::Letter;
        event.shift = std::isupper(c);
    } else if (std::isdigit(c)) {
        event.type = KeyType::Digit;
    } else if (c == ' ') {
        event.type = KeyType::Space;
    } else if (c == '-') {
        event.type = KeyType::Minus;
    } else if (c == '=') {
        event.type = KeyType::Equal;
    } else {
        event.type = KeyType::Other;
    }

    return event;
}

KeyEvent KeyEvent::FromSpecial(KeyType type, bool shift, bool ctrl, bool alt) {
    KeyEvent event;
    event.type = type;
    event.shift = shift;
    event.ctrl = ctrl;
    event.alt = alt;
    return event;
}

// ==================== InputSession 实现 ====================

InputSession::InputSession(storage::ILocalStorage* storage,
                           frequency::IFrequencyManager* frequencyManager)
    : storage_(storage),
      frequencyManager_(frequencyManager),
      merger_(std::make_unique<CandidateMerger>(storage)),
      pageSize_(9) {}

InputSession::~InputSession() = default;

InputResult InputSession::ProcessKey(const KeyEvent& event) {
    // 英文模式下，大部分按键直接透传
    if (state_.mode == InputMode::English) {
        // Shift 键切换模式
        if (event.type == KeyType::Shift && !event.ctrl && !event.alt) {
            ToggleInputMode();
            return InputResult::Consumed();
        }
        return InputResult::PassThrough();
    }

    // 临时英文模式下的特殊处理
    if (state_.mode == InputMode::TempEnglish) {
        // Shift 键切换回中文模式
        if (event.type == KeyType::Shift && !event.ctrl && !event.alt) {
            state_.mode = InputMode::Chinese;
            return InputResult::Consumed();
        }
        // 空格或回车键：透传并恢复中文模式
        if (event.type == KeyType::Space || event.type == KeyType::Enter) {
            state_.mode = InputMode::Chinese;
            return InputResult::PassThrough();
        }
        // Escape 键：恢复中文模式
        if (event.type == KeyType::Escape) {
            state_.mode = InputMode::Chinese;
            return InputResult::PassThrough();
        }
        // 其他按键直接透传
        return InputResult::PassThrough();
    }

    // 中文模式下的按键处理
    switch (event.type) {
        case KeyType::Letter:
            return ProcessChar(event.character);

        case KeyType::Digit:
            if (state_.isComposing && !state_.candidates.empty()) {
                // 数字键选择候选词
                int index = event.character - '0';
                if (index >= 1 && index <= 9) {
                    return SelectCandidate(index);
                }
            }
            // 不在输入状态时，数字键透传
            return InputResult::PassThrough();

        case KeyType::Space:
            if (state_.isComposing) {
                return SelectFirstCandidate();
            }
            return InputResult::PassThrough();

        case KeyType::Enter:
            if (state_.isComposing) {
                return CommitRawInput();
            }
            return InputResult::PassThrough();

        case KeyType::Escape:
            if (state_.isComposing) {
                return Cancel();
            }
            return InputResult::PassThrough();

        case KeyType::Backspace:
            if (state_.isComposing) {
                return Backspace();
            }
            return InputResult::PassThrough();

        case KeyType::PageUp:
        case KeyType::Minus:
            if (state_.isComposing && state_.totalPages > 1) {
                return PageUp();
            }
            return InputResult::PassThrough();

        case KeyType::PageDown:
        case KeyType::Equal:
            if (state_.isComposing && state_.totalPages > 1) {
                return PageDown();
            }
            return InputResult::PassThrough();

        case KeyType::Shift:
            if (!event.ctrl && !event.alt) {
                ToggleInputMode();
                return InputResult::Consumed();
            }
            return InputResult::PassThrough();

        default:
            return InputResult::PassThrough();
    }
}

InputResult InputSession::ProcessChar(char c) {
    // 英文模式下，直接透传
    if (state_.mode == InputMode::English) {
        return InputResult::PassThrough();
    }

    // 检查临时英文模式
    if (!state_.isComposing && IsTempEnglishTrigger(c)) {
        state_.mode = InputMode::TempEnglish;
        // 临时英文模式下，直接透传
        return InputResult::PassThrough();
    }

    // 临时英文模式下，继续透传直到提交
    if (state_.mode == InputMode::TempEnglish) {
        return InputResult::PassThrough();
    }

    // 中文模式下，处理拼音输入
    if (std::isalpha(c)) {
        // 转换为小写
        char lowerC = std::tolower(c);
        state_.preedit += lowerC;
        state_.isComposing = true;

        // 更新候选词
        UpdateCandidates();

        // 如果候选词为空，返回需要隐藏候选窗口的结果
        // 但仍然保持输入状态（用户可以继续输入或提交原文）
        if (state_.candidates.empty()) {
            InputResult result;
            result.consumed = true;
            result.needsUpdate = true;  // 仍然需要更新（显示预编辑文本）
            result.needsHide = true;    // 但需要隐藏候选窗口
            return result;
        }

        return InputResult::Update();
    }

    return InputResult::PassThrough();
}

InputResult InputSession::SelectCandidate(int index) {
    if (!state_.isComposing || state_.candidates.empty()) {
        return InputResult::PassThrough();
    }

    // 索引从 1 开始
    int arrayIndex = index - 1;
    if (arrayIndex < 0 || arrayIndex >= static_cast<int>(state_.candidates.size())) {
        return InputResult::PassThrough();
    }

    const auto& candidate = state_.candidates[arrayIndex];

    // 记录词频
    RecordWordSelection(candidate.text, state_.preedit);

    // 重置状态
    std::string commitText = candidate.text;
    state_.Reset();

    // 如果是临时英文模式，恢复中文模式
    if (state_.mode == InputMode::TempEnglish) {
        state_.mode = InputMode::Chinese;
    }

    return InputResult::Commit(commitText);
}

InputResult InputSession::SelectFirstCandidate() {
    if (!state_.isComposing) {
        return InputResult::PassThrough();
    }

    if (state_.candidates.empty()) {
        // 没有候选词时，提交原始拼音
        return CommitRawInput();
    }

    return SelectCandidate(1);
}

InputResult InputSession::CommitRawInput() {
    if (!state_.isComposing || state_.preedit.empty()) {
        return InputResult::PassThrough();
    }

    std::string commitText = state_.preedit;
    state_.Reset();

    // 如果是临时英文模式，恢复中文模式
    if (state_.mode == InputMode::TempEnglish) {
        state_.mode = InputMode::Chinese;
    }

    return InputResult::Commit(commitText);
}

InputResult InputSession::Backspace() {
    if (!state_.isComposing || state_.preedit.empty()) {
        return InputResult::PassThrough();
    }

    // 删除最后一个字符
    state_.preedit.pop_back();

    if (state_.preedit.empty()) {
        // 拼音为空，取消输入
        state_.Reset();
        return InputResult::Hide();
    }

    // 更新候选词
    UpdateCandidates();

    // 如果候选词为空，返回需要隐藏候选窗口的结果
    if (state_.candidates.empty()) {
        InputResult result;
        result.consumed = true;
        result.needsUpdate = true;  // 仍然需要更新（显示预编辑文本）
        result.needsHide = true;    // 但需要隐藏候选窗口
        return result;
    }

    return InputResult::Update();
}

InputResult InputSession::Cancel() {
    if (!state_.isComposing) {
        return InputResult::PassThrough();
    }

    state_.Reset();

    // 如果是临时英文模式，恢复中文模式
    if (state_.mode == InputMode::TempEnglish) {
        state_.mode = InputMode::Chinese;
    }

    return InputResult::Hide();
}

InputResult InputSession::PageUp() {
    if (!state_.isComposing || state_.totalPages <= 1) {
        return InputResult::PassThrough();
    }

    if (state_.currentPage > 0) {
        state_.currentPage--;
        UpdateCurrentPage();
        return InputResult::Update();
    }

    return InputResult::Consumed();
}

InputResult InputSession::PageDown() {
    if (!state_.isComposing || state_.totalPages <= 1) {
        return InputResult::PassThrough();
    }

    if (state_.currentPage < state_.totalPages - 1) {
        state_.currentPage++;
        UpdateCurrentPage();
        return InputResult::Update();
    }

    return InputResult::Consumed();
}

void InputSession::ToggleInputMode() {
    if (state_.mode == InputMode::Chinese) {
        SetInputMode(InputMode::English);
    } else {
        SetInputMode(InputMode::Chinese);
    }
}

void InputSession::SetInputMode(InputMode mode, bool persist) {
    state_.mode = mode;

    // 切换模式时，如果正在输入，取消当前输入
    if (state_.isComposing && !state_.preedit.empty()) {
        state_.Reset();
    }

    // 持久化模式状态（仅对 Chinese 和 English 模式）
    if (persist && storage_ && storage_->IsInitialized()) {
        if (mode == InputMode::Chinese || mode == InputMode::English) {
            SaveInputModeToStorage();
        }
    }
}

void InputSession::LoadInputModeFromStorage() {
    if (!storage_ || !storage_->IsInitialized()) {
        return;
    }

    std::string modeStr = storage_->GetConfig("input.default_mode", "chinese");
    state_.mode = StringToInputMode(modeStr);
}

void InputSession::SaveInputModeToStorage() {
    if (!storage_ || !storage_->IsInitialized()) {
        return;
    }

    // 只保存 Chinese 和 English 模式，TempEnglish 不持久化
    if (state_.mode == InputMode::Chinese || state_.mode == InputMode::English) {
        storage_->SetConfig("input.default_mode", InputModeToString(state_.mode));
    }
}

std::string InputSession::InputModeToString(InputMode mode) {
    switch (mode) {
        case InputMode::Chinese:
            return "chinese";
        case InputMode::English:
            return "english";
        case InputMode::TempEnglish:
            return "chinese";  // TempEnglish 保存为 chinese
        default:
            return "chinese";
    }
}

InputMode InputSession::StringToInputMode(const std::string& str) {
    if (str == "english") {
        return InputMode::English;
    }
    return InputMode::Chinese;  // 默认中文模式
}

void InputSession::UpdateCandidates() {
    state_.allCandidates.clear();
    state_.candidates.clear();
    state_.currentPage = 0;
    state_.totalPages = 0;

    if (state_.preedit.empty()) {
        return;
    }

    // 从 librime 获取候选词
    std::vector<CandidateWord> rimeCandidates;
    if (candidateQueryCallback_) {
        rimeCandidates = candidateQueryCallback_(state_.preedit);
    }

    // 合并用户高频词（使用 MergeAll 获取所有候选词用于分页）
    state_.allCandidates = merger_->MergeAll(rimeCandidates, state_.preedit);

    // 计算总页数
    state_.totalPages = CandidateUtils::GetTotalPages(
        static_cast<int>(state_.allCandidates.size()), pageSize_);

    // 更新当前页
    UpdateCurrentPage();
}

void InputSession::UpdateCurrentPage() {
    state_.candidates = CandidateUtils::GetPage(
        state_.allCandidates, state_.currentPage, pageSize_);
}

void InputSession::RecordWordSelection(const std::string& word,
                                       const std::string& pinyin) {
    if (frequencyManager_ && frequencyManager_->IsInitialized()) {
        frequencyManager_->RecordWordSelection(word, pinyin);
    }
}

bool InputSession::IsTempEnglishTrigger(char c) const {
    // 大写字母开头触发临时英文模式
    return std::isupper(c);
}

}  // namespace input
}  // namespace ime
