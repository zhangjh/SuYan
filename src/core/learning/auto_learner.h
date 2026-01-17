// src/core/learning/auto_learner.h
// 自动学词模块接口

#ifndef IME_AUTO_LEARNER_H
#define IME_AUTO_LEARNER_H

#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <vector>

namespace ime {
namespace storage {
class ILocalStorage;
}

namespace learning {

// 输入记录（用于检测连续输入）
struct InputRecord {
    std::string text;      // 输入的文字
    std::string pinyin;    // 对应的拼音
    int64_t timestamp;     // 输入时间戳（毫秒）
    bool isSingleChar;     // 是否是单字

    InputRecord() : timestamp(0), isSingleChar(false) {}

    InputRecord(const std::string& t, const std::string& p, int64_t ts)
        : text(t), pinyin(p), timestamp(ts), isSingleChar(t.length() <= 3) {}
};

// 学词候选
struct LearnCandidate {
    std::string text;      // 词组文本
    std::string pinyin;    // 词组拼音
    int occurrences;       // 出现次数
    int64_t lastSeen;      // 最后出现时间

    LearnCandidate() : occurrences(0), lastSeen(0) {}

    LearnCandidate(const std::string& t, const std::string& p)
        : text(t), pinyin(p), occurrences(1), lastSeen(0) {}
};

// 自动学词配置
struct AutoLearnConfig {
    int minWordLength;          // 最小词长（字符数）
    int maxWordLength;          // 最大词长（字符数）
    int minOccurrences;         // 最小出现次数才学习
    int64_t maxInputInterval;   // 最大输入间隔（毫秒），超过则不视为连续输入
    int historySize;            // 输入历史记录大小
    bool enabled;               // 是否启用自动学词

    // 默认配置
    static AutoLearnConfig Default() {
        return AutoLearnConfig{
            .minWordLength = 2,        // 至少 2 个字
            .maxWordLength = 6,        // 最多 6 个字
            .minOccurrences = 2,       // 出现 2 次才学习
            .maxInputInterval = 3000,  // 3 秒内的输入视为连续
            .historySize = 20,         // 保留最近 20 次输入
            .enabled = true,
        };
    }
};

// 自动学词器接口
class IAutoLearner {
public:
    virtual ~IAutoLearner() = default;

    // ==================== 初始化 ====================

    // 初始化学词器
    virtual bool Initialize() = 0;

    // 关闭学词器
    virtual void Shutdown() = 0;

    // 检查是否已初始化
    virtual bool IsInitialized() const = 0;

    // ==================== 输入记录 ====================

    // 记录用户输入
    // @param text 输入的文字
    // @param pinyin 对应的拼音
    // @return 如果检测到新词组，返回学习的词组；否则返回空
    virtual std::vector<LearnCandidate> RecordInput(const std::string& text,
                                                    const std::string& pinyin) = 0;

    // 清空输入历史
    virtual void ClearHistory() = 0;

    // 获取当前输入历史
    virtual std::vector<InputRecord> GetHistory() const = 0;

    // ==================== 学词管理 ====================

    // 获取待学习的词组候选
    virtual std::vector<LearnCandidate> GetLearnCandidates() const = 0;

    // 确认学习词组（添加到用户词库）
    // @param text 词组文本
    // @param pinyin 词组拼音
    // @return 是否成功
    virtual bool ConfirmLearn(const std::string& text,
                              const std::string& pinyin) = 0;

    // 拒绝学习词组
    // @param text 词组文本
    // @param pinyin 词组拼音
    virtual void RejectLearn(const std::string& text,
                             const std::string& pinyin) = 0;

    // 自动处理学词候选（达到阈值自动学习）
    // @return 自动学习的词组列表
    virtual std::vector<LearnCandidate> ProcessCandidates() = 0;

    // ==================== 配置 ====================

    // 获取当前配置
    virtual AutoLearnConfig GetConfig() const = 0;

    // 设置配置
    virtual void SetConfig(const AutoLearnConfig& config) = 0;

    // 启用/禁用自动学词
    virtual void SetEnabled(bool enabled) = 0;

    // 检查是否启用
    virtual bool IsEnabled() const = 0;
};

}  // namespace learning
}  // namespace ime

#endif  // IME_AUTO_LEARNER_H
