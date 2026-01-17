// src/core/input/input_state.h
// 输入状态管理

#ifndef IME_INPUT_STATE_H
#define IME_INPUT_STATE_H

#include <functional>
#include <string>
#include <vector>

#include "candidate_merger.h"

namespace ime {
namespace input {

// 输入状态变化类型
enum class StateChangeType {
    None,           // 无变化
    PreeditChanged, // 预编辑文本变化
    CandidatesChanged, // 候选词变化
    PageChanged,    // 翻页
    ModeChanged,    // 模式变化
    Committed,      // 提交
    Cancelled,      // 取消
    Reset           // 重置
};

// 输入状态变化事件
struct StateChangeEvent {
    StateChangeType type;
    std::string preedit;                    // 当前预编辑文本
    std::vector<CandidateWord> candidates;  // 当前候选词
    int currentPage;                        // 当前页码
    int totalPages;                         // 总页数
    InputMode mode;                         // 当前模式
    std::string committedText;              // 提交的文本（仅 Committed 时有效）

    StateChangeEvent() : type(StateChangeType::None), 
                         currentPage(0), totalPages(0),
                         mode(InputMode::Chinese) {}
};

// 状态变化监听器
using StateChangeListener = std::function<void(const StateChangeEvent&)>;

// 输入状态管理器
// 提供输入状态的统一管理和事件通知
class InputStateManager {
public:
    InputStateManager();
    ~InputStateManager();

    // 禁止拷贝
    InputStateManager(const InputStateManager&) = delete;
    InputStateManager& operator=(const InputStateManager&) = delete;

    // ==================== 状态查询 ====================

    // 获取预编辑文本
    const std::string& GetPreedit() const { return preedit_; }

    // 获取候选词列表
    const std::vector<CandidateWord>& GetCandidates() const { return candidates_; }

    // 获取所有候选词（用于分页）
    const std::vector<CandidateWord>& GetAllCandidates() const { return allCandidates_; }

    // 获取当前页码
    int GetCurrentPage() const { return currentPage_; }

    // 获取总页数
    int GetTotalPages() const { return totalPages_; }

    // 获取输入模式
    InputMode GetMode() const { return mode_; }

    // 是否正在输入
    bool IsComposing() const { return !preedit_.empty(); }

    // 是否有候选词
    bool HasCandidates() const { return !candidates_.empty(); }

    // ==================== 状态修改 ====================

    // 设置预编辑文本
    void SetPreedit(const std::string& preedit);

    // 追加字符到预编辑文本
    void AppendToPreedit(char c);

    // 从预编辑文本删除最后一个字符
    // @return 是否还有剩余字符
    bool PopFromPreedit();

    // 设置候选词列表
    void SetCandidates(const std::vector<CandidateWord>& candidates);

    // 设置所有候选词（用于分页）
    void SetAllCandidates(const std::vector<CandidateWord>& candidates, int pageSize = 9);

    // 设置当前页
    void SetCurrentPage(int page);

    // 上一页
    bool PreviousPage();

    // 下一页
    bool NextPage();

    // 设置输入模式
    void SetMode(InputMode mode);

    // 切换输入模式
    void ToggleMode();

    // ==================== 状态重置 ====================

    // 重置所有状态
    void Reset();

    // 提交并重置
    void CommitAndReset(const std::string& text);

    // 取消并重置
    void CancelAndReset();

    // ==================== 事件监听 ====================

    // 添加状态变化监听器
    void AddListener(StateChangeListener listener);

    // 清除所有监听器
    void ClearListeners();

    // ==================== 配置 ====================

    // 设置每页候选词数量
    void SetPageSize(int size) { pageSize_ = size; }

    // 获取每页候选词数量
    int GetPageSize() const { return pageSize_; }

private:
    // 通知状态变化
    void NotifyChange(StateChangeType type, const std::string& committedText = "");

    // 更新当前页候选词
    void UpdateCurrentPageCandidates();

private:
    std::string preedit_;
    std::vector<CandidateWord> candidates_;
    std::vector<CandidateWord> allCandidates_;
    int currentPage_;
    int totalPages_;
    InputMode mode_;
    int pageSize_;
    std::vector<StateChangeListener> listeners_;
};

}  // namespace input
}  // namespace ime

#endif  // IME_INPUT_STATE_H
