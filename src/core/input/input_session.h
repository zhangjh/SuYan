// src/core/input/input_session.h
// 输入会话管理 - 管理单次输入的状态和操作

#ifndef IME_INPUT_SESSION_H
#define IME_INPUT_SESSION_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "candidate_merger.h"

namespace ime {

namespace storage {
class ILocalStorage;
}

namespace frequency {
class IFrequencyManager;
}

namespace input {

// InputMode 已在 candidate_merger.h 中定义

// 按键类型
enum class KeyType {
    Letter,     // 字母键 (a-z, A-Z)
    Digit,      // 数字键 (0-9)
    Space,      // 空格键
    Enter,      // 回车键
    Escape,     // Esc 键
    Backspace,  // 退格键
    Delete,     // Delete 键
    PageUp,     // Page Up
    PageDown,   // Page Down
    Minus,      // 减号 (翻页)
    Equal,      // 等号 (翻页)
    Shift,      // Shift 键
    Other       // 其他键
};

// 按键事件
struct KeyEvent {
    KeyType type;
    char character;     // 对于字母/数字键，存储具体字符
    bool shift;         // Shift 是否按下
    bool ctrl;          // Ctrl 是否按下
    bool alt;           // Alt 是否按下

    KeyEvent() : type(KeyType::Other), character(0), 
                 shift(false), ctrl(false), alt(false) {}

    // 从字符创建按键事件
    static KeyEvent FromChar(char c);

    // 从特殊键创建按键事件
    static KeyEvent FromSpecial(KeyType type, bool shift = false,
                                bool ctrl = false, bool alt = false);
};

// 输入结果
struct InputResult {
    bool consumed;          // 按键是否被消费
    bool needsCommit;       // 是否需要提交文本
    std::string commitText; // 要提交的文本
    bool needsUpdate;       // 是否需要更新候选词窗口
    bool needsHide;         // 是否需要隐藏候选词窗口

    InputResult() : consumed(false), needsCommit(false), 
                    needsUpdate(false), needsHide(false) {}

    // 创建消费但不提交的结果
    static InputResult Consumed() {
        InputResult r;
        r.consumed = true;
        return r;
    }

    // 创建提交结果
    static InputResult Commit(const std::string& text) {
        InputResult r;
        r.consumed = true;
        r.needsCommit = true;
        r.commitText = text;
        r.needsHide = true;
        return r;
    }

    // 创建更新候选词的结果
    static InputResult Update() {
        InputResult r;
        r.consumed = true;
        r.needsUpdate = true;
        return r;
    }

    // 创建隐藏候选词窗口的结果
    static InputResult Hide() {
        InputResult r;
        r.consumed = true;
        r.needsHide = true;
        return r;
    }

    // 创建不消费的结果（透传给应用）
    static InputResult PassThrough() {
        return InputResult();
    }
};

// 输入会话状态
struct SessionState {
    std::string preedit;                    // 预编辑文本（拼音）
    std::vector<CandidateWord> candidates;  // 当前候选词列表
    std::vector<CandidateWord> allCandidates; // 所有候选词（用于分页）
    int currentPage;                        // 当前页码
    int totalPages;                         // 总页数
    InputMode mode;                         // 输入模式
    bool isComposing;                       // 是否正在输入

    SessionState() : currentPage(0), totalPages(0), 
                     mode(InputMode::Chinese), isComposing(false) {}

    void Reset() {
        preedit.clear();
        candidates.clear();
        allCandidates.clear();
        currentPage = 0;
        totalPages = 0;
        isComposing = false;
    }
};

// 输入会话
// 管理单次输入的完整生命周期
class InputSession {
public:
    // 候选词查询回调（用于从 librime 获取候选词）
    using CandidateQueryCallback = std::function<std::vector<CandidateWord>(
        const std::string& pinyin)>;

    // 构造函数
    InputSession(storage::ILocalStorage* storage,
                 frequency::IFrequencyManager* frequencyManager);

    // 析构函数
    ~InputSession();

    // 禁止拷贝
    InputSession(const InputSession&) = delete;
    InputSession& operator=(const InputSession&) = delete;

    // ==================== 按键处理 ====================

    // 处理按键事件
    InputResult ProcessKey(const KeyEvent& event);

    // 处理字符输入
    InputResult ProcessChar(char c);

    // ==================== 候选词操作 ====================

    // 选择候选词（通过索引 1-9）
    InputResult SelectCandidate(int index);

    // 选择首选候选词（空格键）
    InputResult SelectFirstCandidate();

    // 提交原始拼音（回车键）
    InputResult CommitRawInput();

    // ==================== 编辑操作 ====================

    // 退格删除
    InputResult Backspace();

    // 清空输入（Esc 键）
    InputResult Cancel();

    // ==================== 翻页操作 ====================

    // 上一页
    InputResult PageUp();

    // 下一页
    InputResult PageDown();

    // ==================== 模式切换 ====================

    // 切换中英文模式
    void ToggleInputMode();

    // 设置输入模式
    // @param mode 输入模式
    // @param persist 是否持久化到存储（默认 true）
    void SetInputMode(InputMode mode, bool persist = true);

    // 获取输入模式
    InputMode GetInputMode() const { return state_.mode; }

    // 从存储加载输入模式
    void LoadInputModeFromStorage();

    // 保存输入模式到存储
    void SaveInputModeToStorage();

    // ==================== 状态查询 ====================

    // 获取当前状态
    const SessionState& GetState() const { return state_; }

    // 是否正在输入
    bool IsComposing() const { return state_.isComposing; }

    // 获取预编辑文本
    const std::string& GetPreedit() const { return state_.preedit; }

    // 获取当前页候选词
    const std::vector<CandidateWord>& GetCandidates() const {
        return state_.candidates;
    }

    // ==================== 配置 ====================

    // 设置候选词查询回调
    void SetCandidateQueryCallback(CandidateQueryCallback callback) {
        candidateQueryCallback_ = callback;
    }

    // 设置每页候选词数量
    void SetPageSize(int size) { pageSize_ = size; }

private:
    // 更新候选词列表
    void UpdateCandidates();

    // 更新当前页
    void UpdateCurrentPage();

    // 记录词频
    void RecordWordSelection(const std::string& word, const std::string& pinyin);

    // 检查是否是临时英文模式触发
    bool IsTempEnglishTrigger(char c) const;

    // 输入模式转换为字符串
    static std::string InputModeToString(InputMode mode);

    // 字符串转换为输入模式
    static InputMode StringToInputMode(const std::string& str);

private:
    storage::ILocalStorage* storage_;
    frequency::IFrequencyManager* frequencyManager_;
    std::unique_ptr<CandidateMerger> merger_;
    CandidateQueryCallback candidateQueryCallback_;
    SessionState state_;
    int pageSize_;
};

}  // namespace input
}  // namespace ime

#endif  // IME_INPUT_SESSION_H
