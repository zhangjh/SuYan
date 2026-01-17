// src/core/input/input_state.cpp
// 输入状态管理实现

#include "input_state.h"

#include <algorithm>

namespace ime {
namespace input {

InputStateManager::InputStateManager()
    : currentPage_(0),
      totalPages_(0),
      mode_(InputMode::Chinese),
      pageSize_(9) {}

InputStateManager::~InputStateManager() = default;

// ==================== 状态修改 ====================

void InputStateManager::SetPreedit(const std::string& preedit) {
    if (preedit_ != preedit) {
        preedit_ = preedit;
        NotifyChange(StateChangeType::PreeditChanged);
    }
}

void InputStateManager::AppendToPreedit(char c) {
    preedit_ += c;
    NotifyChange(StateChangeType::PreeditChanged);
}

bool InputStateManager::PopFromPreedit() {
    if (preedit_.empty()) {
        return false;
    }

    preedit_.pop_back();
    NotifyChange(StateChangeType::PreeditChanged);
    return !preedit_.empty();
}

void InputStateManager::SetCandidates(const std::vector<CandidateWord>& candidates) {
    candidates_ = candidates;
    allCandidates_ = candidates;
    currentPage_ = 0;
    totalPages_ = CandidateUtils::GetTotalPages(
        static_cast<int>(candidates.size()), pageSize_);
    NotifyChange(StateChangeType::CandidatesChanged);
}

void InputStateManager::SetAllCandidates(const std::vector<CandidateWord>& candidates,
                                         int pageSize) {
    allCandidates_ = candidates;
    pageSize_ = pageSize;
    currentPage_ = 0;
    totalPages_ = CandidateUtils::GetTotalPages(
        static_cast<int>(candidates.size()), pageSize_);
    UpdateCurrentPageCandidates();
    NotifyChange(StateChangeType::CandidatesChanged);
}

void InputStateManager::SetCurrentPage(int page) {
    if (page >= 0 && page < totalPages_ && page != currentPage_) {
        currentPage_ = page;
        UpdateCurrentPageCandidates();
        NotifyChange(StateChangeType::PageChanged);
    }
}

bool InputStateManager::PreviousPage() {
    if (currentPage_ > 0) {
        currentPage_--;
        UpdateCurrentPageCandidates();
        NotifyChange(StateChangeType::PageChanged);
        return true;
    }
    return false;
}

bool InputStateManager::NextPage() {
    if (currentPage_ < totalPages_ - 1) {
        currentPage_++;
        UpdateCurrentPageCandidates();
        NotifyChange(StateChangeType::PageChanged);
        return true;
    }
    return false;
}

void InputStateManager::SetMode(InputMode mode) {
    if (mode_ != mode) {
        mode_ = mode;
        NotifyChange(StateChangeType::ModeChanged);
    }
}

void InputStateManager::ToggleMode() {
    if (mode_ == InputMode::Chinese) {
        SetMode(InputMode::English);
    } else {
        SetMode(InputMode::Chinese);
    }
}

// ==================== 状态重置 ====================

void InputStateManager::Reset() {
    preedit_.clear();
    candidates_.clear();
    allCandidates_.clear();
    currentPage_ = 0;
    totalPages_ = 0;
    // 注意：不重置 mode_，保持当前模式
    NotifyChange(StateChangeType::Reset);
}

void InputStateManager::CommitAndReset(const std::string& text) {
    preedit_.clear();
    candidates_.clear();
    allCandidates_.clear();
    currentPage_ = 0;
    totalPages_ = 0;

    // 如果是临时英文模式，恢复中文模式
    if (mode_ == InputMode::TempEnglish) {
        mode_ = InputMode::Chinese;
    }

    NotifyChange(StateChangeType::Committed, text);
}

void InputStateManager::CancelAndReset() {
    preedit_.clear();
    candidates_.clear();
    allCandidates_.clear();
    currentPage_ = 0;
    totalPages_ = 0;

    // 如果是临时英文模式，恢复中文模式
    if (mode_ == InputMode::TempEnglish) {
        mode_ = InputMode::Chinese;
    }

    NotifyChange(StateChangeType::Cancelled);
}

// ==================== 事件监听 ====================

void InputStateManager::AddListener(StateChangeListener listener) {
    if (listener) {
        listeners_.push_back(listener);
    }
}

void InputStateManager::ClearListeners() {
    listeners_.clear();
}

// ==================== 私有方法 ====================

void InputStateManager::NotifyChange(StateChangeType type,
                                     const std::string& committedText) {
    if (listeners_.empty()) {
        return;
    }

    StateChangeEvent event;
    event.type = type;
    event.preedit = preedit_;
    event.candidates = candidates_;
    event.currentPage = currentPage_;
    event.totalPages = totalPages_;
    event.mode = mode_;
    event.committedText = committedText;

    for (const auto& listener : listeners_) {
        listener(event);
    }
}

void InputStateManager::UpdateCurrentPageCandidates() {
    candidates_ = CandidateUtils::GetPage(allCandidates_, currentPage_, pageSize_);
}

}  // namespace input
}  // namespace ime
