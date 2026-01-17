// src/core/learning/auto_learner_impl.h
// 自动学词模块实现

#ifndef IME_AUTO_LEARNER_IMPL_H
#define IME_AUTO_LEARNER_IMPL_H

#include "auto_learner.h"

#include <deque>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace ime {
namespace storage {
class ILocalStorage;
}

namespace learning {

// 自动学词器实现
class AutoLearnerImpl : public IAutoLearner {
public:
    // 构造函数
    // @param storage 本地存储接口（外部管理生命周期）
    explicit AutoLearnerImpl(storage::ILocalStorage* storage);

    // 析构函数
    ~AutoLearnerImpl() override;

    // 禁止拷贝
    AutoLearnerImpl(const AutoLearnerImpl&) = delete;
    AutoLearnerImpl& operator=(const AutoLearnerImpl&) = delete;

    // ==================== IAutoLearner 接口实现 ====================

    bool Initialize() override;
    void Shutdown() override;
    bool IsInitialized() const override;

    std::vector<LearnCandidate> RecordInput(const std::string& text,
                                            const std::string& pinyin) override;
    void ClearHistory() override;
    std::vector<InputRecord> GetHistory() const override;

    std::vector<LearnCandidate> GetLearnCandidates() const override;
    bool ConfirmLearn(const std::string& text,
                      const std::string& pinyin) override;
    void RejectLearn(const std::string& text,
                     const std::string& pinyin) override;
    std::vector<LearnCandidate> ProcessCandidates() override;

    AutoLearnConfig GetConfig() const override;
    void SetConfig(const AutoLearnConfig& config) override;
    void SetEnabled(bool enabled) override;
    bool IsEnabled() const override;

private:
    // 获取当前时间戳（毫秒）
    static int64_t GetCurrentTimestamp();

    // 检测连续输入形成的词组
    // @return 检测到的词组候选列表
    std::vector<LearnCandidate> DetectPhrases();

    // 生成词组的唯一键
    static std::string MakePhraseKey(const std::string& text,
                                     const std::string& pinyin);

    // 检查词组是否已存在于用户词库
    bool IsWordInUserDict(const std::string& text,
                          const std::string& pinyin) const;

    // 添加词组到用户词库
    bool AddWordToUserDict(const std::string& text, const std::string& pinyin);

    // 判断是否是单字（UTF-8 编码）
    static bool IsSingleChar(const std::string& text);

    // 获取 UTF-8 字符串的字符数
    static size_t GetCharCount(const std::string& text);

private:
    storage::ILocalStorage* storage_;  // 本地存储（不拥有）
    AutoLearnConfig config_;           // 配置
    mutable std::mutex mutex_;         // 线程安全锁
    bool initialized_;                 // 是否已初始化

    // 输入历史记录
    std::deque<InputRecord> inputHistory_;

    // 学词候选（phrase_key -> candidate）
    std::unordered_map<std::string, LearnCandidate> learnCandidates_;

    // 已拒绝的词组（避免重复提示）
    std::unordered_set<std::string> rejectedPhrases_;
};

}  // namespace learning
}  // namespace ime

#endif  // IME_AUTO_LEARNER_IMPL_H
