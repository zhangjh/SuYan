// src/core/frequency/frequency_manager_impl.h
// 用户词频管理器实现

#ifndef IME_FREQUENCY_MANAGER_IMPL_H
#define IME_FREQUENCY_MANAGER_IMPL_H

#include "frequency_manager.h"

#include <memory>
#include <mutex>

namespace ime {
namespace storage {
class ILocalStorage;
}

namespace frequency {

// 词频管理器实现
class FrequencyManagerImpl : public IFrequencyManager {
public:
    // 构造函数
    // @param storage 本地存储接口（外部管理生命周期）
    explicit FrequencyManagerImpl(storage::ILocalStorage* storage);

    // 析构函数
    ~FrequencyManagerImpl() override;

    // 禁止拷贝
    FrequencyManagerImpl(const FrequencyManagerImpl&) = delete;
    FrequencyManagerImpl& operator=(const FrequencyManagerImpl&) = delete;

    // ==================== IFrequencyManager 接口实现 ====================

    bool Initialize() override;
    void Shutdown() override;
    bool IsInitialized() const override;

    int RecordWordSelection(const std::string& word,
                            const std::string& pinyin) override;
    void RecordWordSelections(
        const std::vector<std::pair<std::string, std::string>>& words) override;

    int GetUserFrequency(const std::string& word,
                         const std::string& pinyin) override;
    std::vector<CandidateWord> GetTopUserWords(const std::string& pinyin,
                                               int limit = 10) override;

    void SortCandidates(std::vector<CandidateWord>& candidates,
                        const std::string& pinyin) override;
    void CalculateCombinedScore(CandidateWord& candidate) override;

    FrequencyConfig GetConfig() const override;
    void SetConfig(const FrequencyConfig& config) override;

    bool ClearAllUserFrequencies() override;
    void ExportUserFrequencies(
        std::function<void(const std::string& word, const std::string& pinyin,
                           int frequency)>
            callback) override;
    bool ImportUserFrequency(const std::string& word, const std::string& pinyin,
                             int frequency) override;

private:
    // 计算综合得分（内部方法，不加锁）
    void CalculateCombinedScoreInternal(CandidateWord& candidate);

    // 计算最近使用加成
    double CalculateRecencyBonus(int64_t lastUsedTimestamp) const;

    // 归一化词频到 0-1 范围
    double NormalizeFrequency(int64_t frequency, int64_t maxFrequency) const;

private:
    storage::ILocalStorage* storage_;  // 本地存储（不拥有）
    FrequencyConfig config_;           // 配置
    mutable std::mutex mutex_;         // 线程安全锁
    bool initialized_;                 // 是否已初始化
};

}  // namespace frequency
}  // namespace ime

#endif  // IME_FREQUENCY_MANAGER_IMPL_H
