// src/core/frequency/frequency_manager_impl.cpp
// 用户词频管理器实现

#include "frequency_manager_impl.h"

#include <algorithm>
#include <chrono>
#include <cmath>

#include "core/storage/local_storage.h"

namespace ime {
namespace frequency {

FrequencyManagerImpl::FrequencyManagerImpl(storage::ILocalStorage* storage)
    : storage_(storage),
      config_(FrequencyConfig::Default()),
      initialized_(false) {}

FrequencyManagerImpl::~FrequencyManagerImpl() {
    Shutdown();
}

bool FrequencyManagerImpl::Initialize() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (initialized_) {
        return true;
    }

    if (!storage_ || !storage_->IsInitialized()) {
        return false;
    }

    // 从存储中加载配置
    std::string userWeightStr =
        storage_->GetConfig("frequency.user_weight", "0.6");
    std::string baseWeightStr =
        storage_->GetConfig("frequency.base_weight", "0.3");
    std::string recencyWeightStr =
        storage_->GetConfig("frequency.recency_weight", "0.1");
    std::string recencyDecayStr =
        storage_->GetConfig("frequency.recency_decay_days", "30");
    std::string maxFreqStr =
        storage_->GetConfig("frequency.max_user_frequency", "100000");

    try {
        config_.userFrequencyWeight = std::stod(userWeightStr);
        config_.baseFrequencyWeight = std::stod(baseWeightStr);
        config_.recencyWeight = std::stod(recencyWeightStr);
        config_.recencyDecayDays = std::stoll(recencyDecayStr);
        config_.maxUserFrequency = std::stoi(maxFreqStr);
    } catch (...) {
        // 使用默认配置
        config_ = FrequencyConfig::Default();
    }

    initialized_ = true;
    return true;
}

void FrequencyManagerImpl::Shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    initialized_ = false;
}

bool FrequencyManagerImpl::IsInitialized() const {
    return initialized_;
}

int FrequencyManagerImpl::RecordWordSelection(const std::string& word,
                                              const std::string& pinyin) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || !storage_) {
        return 0;
    }

    // 增加词频
    if (!storage_->IncrementWordFrequency(word, pinyin)) {
        return 0;
    }

    // 返回更新后的词频
    return storage_->GetWordFrequency(word, pinyin);
}

void FrequencyManagerImpl::RecordWordSelections(
    const std::vector<std::pair<std::string, std::string>>& words) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || !storage_) {
        return;
    }

    for (const auto& [word, pinyin] : words) {
        storage_->IncrementWordFrequency(word, pinyin);
    }
}

int FrequencyManagerImpl::GetUserFrequency(const std::string& word,
                                           const std::string& pinyin) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || !storage_) {
        return 0;
    }

    return storage_->GetWordFrequency(word, pinyin);
}

std::vector<CandidateWord> FrequencyManagerImpl::GetTopUserWords(
    const std::string& pinyin, int limit) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<CandidateWord> result;

    if (!initialized_ || !storage_) {
        return result;
    }

    auto frequencies = storage_->GetTopFrequencyWords(pinyin, limit);

    for (const auto& wf : frequencies) {
        CandidateWord candidate;
        candidate.text = wf.word;
        candidate.pinyin = wf.pinyin;
        candidate.baseFrequency = 0;  // 用户词没有基础词频
        candidate.userFrequency = wf.frequency;
        candidate.combinedScore = wf.frequency;  // 简单使用词频作为得分
        candidate.source = "user";

        result.push_back(std::move(candidate));
    }

    return result;
}

void FrequencyManagerImpl::SortCandidates(std::vector<CandidateWord>& candidates,
                                          const std::string& pinyin) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || !storage_) {
        return;
    }

    // 为每个候选词计算综合得分
    for (auto& candidate : candidates) {
        // 获取用户词频
        candidate.userFrequency =
            storage_->GetWordFrequency(candidate.text, pinyin);

        // 计算综合得分
        CalculateCombinedScoreInternal(candidate);
    }

    // 按综合得分降序排序
    std::sort(candidates.begin(), candidates.end(),
              [](const CandidateWord& a, const CandidateWord& b) {
                  return a.combinedScore > b.combinedScore;
              });
}

void FrequencyManagerImpl::CalculateCombinedScore(CandidateWord& candidate) {
    std::lock_guard<std::mutex> lock(mutex_);
    CalculateCombinedScoreInternal(candidate);
}

void FrequencyManagerImpl::CalculateCombinedScoreInternal(
    CandidateWord& candidate) {
    // 综合得分计算公式：
    // score = baseWeight * normalizedBase + userWeight * normalizedUser +
    // recencyBonus
    //
    // 其中：
    // - normalizedBase = baseFrequency / maxBaseFrequency (归一化到 0-1)
    // - normalizedUser = userFrequency / maxUserFrequency (归一化到 0-1)
    // - recencyBonus = recencyWeight * exp(-daysSinceLastUse / decayDays)

    // 假设最大基础词频为 100000（可以根据实际词库调整）
    const int64_t maxBaseFrequency = 100000;

    double normalizedBase =
        NormalizeFrequency(candidate.baseFrequency, maxBaseFrequency);
    double normalizedUser =
        NormalizeFrequency(candidate.userFrequency, config_.maxUserFrequency);

    // 计算基础得分
    double score = config_.baseFrequencyWeight * normalizedBase +
                   config_.userFrequencyWeight * normalizedUser;

    // 将得分转换为整数（乘以一个大数以保持精度）
    candidate.combinedScore = static_cast<int64_t>(score * 1000000);

    // 如果用户词频很高，给予额外加成
    if (candidate.userFrequency > 10) {
        candidate.combinedScore +=
            static_cast<int64_t>(candidate.userFrequency * 100);
    }
}

double FrequencyManagerImpl::CalculateRecencyBonus(
    int64_t lastUsedTimestamp) const {
    if (lastUsedTimestamp <= 0) {
        return 0.0;
    }

    auto now = std::chrono::duration_cast<std::chrono::seconds>(
                   std::chrono::system_clock::now().time_since_epoch())
                   .count();

    int64_t secondsSinceLastUse = now - lastUsedTimestamp;
    double daysSinceLastUse = secondsSinceLastUse / 86400.0;

    // 指数衰减
    return config_.recencyWeight *
           std::exp(-daysSinceLastUse / config_.recencyDecayDays);
}

double FrequencyManagerImpl::NormalizeFrequency(int64_t frequency,
                                                int64_t maxFrequency) const {
    if (maxFrequency <= 0) {
        return 0.0;
    }

    // 使用对数归一化，避免高频词过度主导
    // log(1 + freq) / log(1 + maxFreq)
    return std::log1p(static_cast<double>(frequency)) /
           std::log1p(static_cast<double>(maxFrequency));
}

FrequencyConfig FrequencyManagerImpl::GetConfig() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_;
}

void FrequencyManagerImpl::SetConfig(const FrequencyConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;

    // 保存配置到存储
    if (storage_ && storage_->IsInitialized()) {
        storage_->SetConfig("frequency.user_weight",
                            std::to_string(config_.userFrequencyWeight));
        storage_->SetConfig("frequency.base_weight",
                            std::to_string(config_.baseFrequencyWeight));
        storage_->SetConfig("frequency.recency_weight",
                            std::to_string(config_.recencyWeight));
        storage_->SetConfig("frequency.recency_decay_days",
                            std::to_string(config_.recencyDecayDays));
        storage_->SetConfig("frequency.max_user_frequency",
                            std::to_string(config_.maxUserFrequency));
    }
}

bool FrequencyManagerImpl::ClearAllUserFrequencies() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || !storage_) {
        return false;
    }

    return storage_->ClearAllWordFrequencies();
}

void FrequencyManagerImpl::ExportUserFrequencies(
    std::function<void(const std::string& word, const std::string& pinyin,
                       int frequency)>
        callback) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || !storage_ || !callback) {
        return;
    }

    auto frequencies = storage_->GetAllWordFrequencies();
    for (const auto& wf : frequencies) {
        callback(wf.word, wf.pinyin, wf.frequency);
    }
}

bool FrequencyManagerImpl::ImportUserFrequency(const std::string& word,
                                               const std::string& pinyin,
                                               int frequency) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || !storage_) {
        return false;
    }

    // 多次调用 IncrementWordFrequency 来设置词频
    // 注意：这不是最高效的方式，但保持了接口的简单性
    // 如果需要高效导入，可以在 ILocalStorage 中添加 SetWordFrequency 方法

    // 首先删除现有记录
    storage_->DeleteWordFrequency(word, pinyin);

    // 然后增加指定次数
    for (int i = 0; i < frequency; ++i) {
        if (!storage_->IncrementWordFrequency(word, pinyin)) {
            return false;
        }
    }

    return true;
}

}  // namespace frequency
}  // namespace ime
