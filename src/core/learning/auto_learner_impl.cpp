// src/core/learning/auto_learner_impl.cpp
// 自动学词模块实现

#include "auto_learner_impl.h"

#include <algorithm>
#include <chrono>

#include "core/storage/local_storage.h"

namespace ime {
namespace learning {

AutoLearnerImpl::AutoLearnerImpl(storage::ILocalStorage* storage)
    : storage_(storage), config_(AutoLearnConfig::Default()), initialized_(false) {}

AutoLearnerImpl::~AutoLearnerImpl() {
    Shutdown();
}

bool AutoLearnerImpl::Initialize() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (initialized_) {
        return true;
    }

    if (!storage_ || !storage_->IsInitialized()) {
        return false;
    }

    // 从存储加载配置
    std::string enabledStr = storage_->GetConfig("learning.enabled", "true");
    std::string minOccStr = storage_->GetConfig("learning.min_occurrences", "2");
    std::string maxIntervalStr =
        storage_->GetConfig("learning.max_interval", "3000");

    config_.enabled = (enabledStr == "true");

    try {
        config_.minOccurrences = std::stoi(minOccStr);
        config_.maxInputInterval = std::stoll(maxIntervalStr);
    } catch (...) {
        // 使用默认值
    }

    initialized_ = true;
    return true;
}

void AutoLearnerImpl::Shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);

    inputHistory_.clear();
    learnCandidates_.clear();
    rejectedPhrases_.clear();

    initialized_ = false;
}

bool AutoLearnerImpl::IsInitialized() const {
    return initialized_;
}

int64_t AutoLearnerImpl::GetCurrentTimestamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

bool AutoLearnerImpl::IsSingleChar(const std::string& text) {
    return GetCharCount(text) == 1;
}

size_t AutoLearnerImpl::GetCharCount(const std::string& text) {
    size_t count = 0;
    for (size_t i = 0; i < text.length();) {
        unsigned char c = text[i];
        if ((c & 0x80) == 0) {
            // ASCII
            i += 1;
        } else if ((c & 0xE0) == 0xC0) {
            // 2-byte UTF-8
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            // 3-byte UTF-8 (中文字符)
            i += 3;
        } else if ((c & 0xF8) == 0xF0) {
            // 4-byte UTF-8
            i += 4;
        } else {
            // 无效 UTF-8，跳过
            i += 1;
        }
        count++;
    }
    return count;
}

std::string AutoLearnerImpl::MakePhraseKey(const std::string& text,
                                           const std::string& pinyin) {
    return text + "\t" + pinyin;
}

std::vector<LearnCandidate> AutoLearnerImpl::RecordInput(
    const std::string& text, const std::string& pinyin) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || !config_.enabled) {
        return {};
    }

    // 创建输入记录
    InputRecord record(text, pinyin, GetCurrentTimestamp());
    record.isSingleChar = IsSingleChar(text);

    // 添加到历史记录
    inputHistory_.push_back(record);

    // 限制历史记录大小
    while (static_cast<int>(inputHistory_.size()) > config_.historySize) {
        inputHistory_.pop_front();
    }

    // 检测词组
    return DetectPhrases();
}

std::vector<LearnCandidate> AutoLearnerImpl::DetectPhrases() {
    std::vector<LearnCandidate> detected;

    if (inputHistory_.size() < 2) {
        return detected;
    }

    // 从最近的输入开始，向前查找连续的单字输入
    int64_t lastTimestamp = inputHistory_.back().timestamp;

    // 收集连续的单字输入
    std::vector<InputRecord> consecutiveChars;

    for (auto it = inputHistory_.rbegin(); it != inputHistory_.rend(); ++it) {
        // 检查时间间隔
        if (lastTimestamp - it->timestamp > config_.maxInputInterval) {
            break;
        }

        // 只考虑单字输入
        if (!it->isSingleChar) {
            break;
        }

        consecutiveChars.push_back(*it);
        lastTimestamp = it->timestamp;
    }

    // 反转以获得正确的顺序
    std::reverse(consecutiveChars.begin(), consecutiveChars.end());

    // 如果连续单字数量不足，返回
    if (static_cast<int>(consecutiveChars.size()) < config_.minWordLength) {
        return detected;
    }

    // 生成所有可能的词组（长度从 minWordLength 到 maxWordLength）
    for (int len = config_.minWordLength;
         len <= std::min(config_.maxWordLength,
                         static_cast<int>(consecutiveChars.size()));
         ++len) {
        // 从末尾取 len 个字符
        int startIdx = static_cast<int>(consecutiveChars.size()) - len;

        std::string phraseText;
        std::string phrasePinyin;

        for (int i = startIdx; i < static_cast<int>(consecutiveChars.size());
             ++i) {
            phraseText += consecutiveChars[i].text;
            if (!phrasePinyin.empty()) {
                phrasePinyin += " ";
            }
            phrasePinyin += consecutiveChars[i].pinyin;
        }

        // 检查是否已拒绝
        std::string key = MakePhraseKey(phraseText, phrasePinyin);
        if (rejectedPhrases_.find(key) != rejectedPhrases_.end()) {
            continue;
        }

        // 检查是否已在用户词库中
        if (IsWordInUserDict(phraseText, phrasePinyin)) {
            continue;
        }

        // 更新或创建学词候选
        auto it = learnCandidates_.find(key);
        if (it != learnCandidates_.end()) {
            it->second.occurrences++;
            it->second.lastSeen = GetCurrentTimestamp();
        } else {
            LearnCandidate candidate(phraseText, phrasePinyin);
            candidate.lastSeen = GetCurrentTimestamp();
            learnCandidates_[key] = candidate;
        }

        // 如果达到学习阈值，添加到检测结果
        if (learnCandidates_[key].occurrences >= config_.minOccurrences) {
            detected.push_back(learnCandidates_[key]);
        }
    }

    return detected;
}

void AutoLearnerImpl::ClearHistory() {
    std::lock_guard<std::mutex> lock(mutex_);
    inputHistory_.clear();
}

std::vector<InputRecord> AutoLearnerImpl::GetHistory() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::vector<InputRecord>(inputHistory_.begin(), inputHistory_.end());
}

std::vector<LearnCandidate> AutoLearnerImpl::GetLearnCandidates() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<LearnCandidate> result;
    for (const auto& [key, candidate] : learnCandidates_) {
        if (candidate.occurrences >= config_.minOccurrences) {
            result.push_back(candidate);
        }
    }

    // 按出现次数降序排序
    std::sort(result.begin(), result.end(),
              [](const LearnCandidate& a, const LearnCandidate& b) {
                  return a.occurrences > b.occurrences;
              });

    return result;
}

bool AutoLearnerImpl::ConfirmLearn(const std::string& text,
                                   const std::string& pinyin) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        return false;
    }

    // 添加到用户词库
    if (!AddWordToUserDict(text, pinyin)) {
        return false;
    }

    // 从候选列表移除
    std::string key = MakePhraseKey(text, pinyin);
    learnCandidates_.erase(key);

    return true;
}

void AutoLearnerImpl::RejectLearn(const std::string& text,
                                  const std::string& pinyin) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string key = MakePhraseKey(text, pinyin);

    // 添加到拒绝列表
    rejectedPhrases_.insert(key);

    // 从候选列表移除
    learnCandidates_.erase(key);
}

std::vector<LearnCandidate> AutoLearnerImpl::ProcessCandidates() {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<LearnCandidate> learned;

    if (!initialized_ || !config_.enabled) {
        return learned;
    }

    // 找出所有达到阈值的候选
    std::vector<std::string> keysToRemove;

    for (const auto& [key, candidate] : learnCandidates_) {
        if (candidate.occurrences >= config_.minOccurrences) {
            // 自动学习
            if (AddWordToUserDict(candidate.text, candidate.pinyin)) {
                learned.push_back(candidate);
                keysToRemove.push_back(key);
            }
        }
    }

    // 移除已学习的候选
    for (const auto& key : keysToRemove) {
        learnCandidates_.erase(key);
    }

    return learned;
}

AutoLearnConfig AutoLearnerImpl::GetConfig() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_;
}

void AutoLearnerImpl::SetConfig(const AutoLearnConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;

    // 保存配置到存储
    if (storage_ && storage_->IsInitialized()) {
        storage_->SetConfig("learning.enabled", config_.enabled ? "true" : "false");
        storage_->SetConfig("learning.min_occurrences",
                            std::to_string(config_.minOccurrences));
        storage_->SetConfig("learning.max_interval",
                            std::to_string(config_.maxInputInterval));
    }
}

void AutoLearnerImpl::SetEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_.enabled = enabled;

    if (storage_ && storage_->IsInitialized()) {
        storage_->SetConfig("learning.enabled", enabled ? "true" : "false");
    }
}

bool AutoLearnerImpl::IsEnabled() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_.enabled;
}

bool AutoLearnerImpl::IsWordInUserDict(const std::string& text,
                                       const std::string& pinyin) const {
    if (!storage_) {
        return false;
    }

    // 检查用户词频表中是否存在
    int freq = storage_->GetWordFrequency(text, pinyin);
    return freq > 0;
}

bool AutoLearnerImpl::AddWordToUserDict(const std::string& text,
                                        const std::string& pinyin) {
    if (!storage_) {
        return false;
    }

    // 通过增加词频来添加词
    // 初始词频设为 1
    return storage_->IncrementWordFrequency(text, pinyin);
}

}  // namespace learning
}  // namespace ime
