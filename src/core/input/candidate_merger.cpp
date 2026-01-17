// src/core/input/candidate_merger.cpp
// 候选词合并器实现

#include "candidate_merger.h"

#include <algorithm>
#include <unordered_set>

#include "core/storage/local_storage.h"

namespace ime {
namespace input {

CandidateMerger::CandidateMerger(storage::ILocalStorage* storage)
    : storage_(storage), config_(MergeConfig::Default()) {}

CandidateMerger::~CandidateMerger() = default;

std::vector<CandidateWord> CandidateMerger::Merge(
    const std::vector<CandidateWord>& rimeCandidates,
    const std::string& pinyin) {
    // Step 1: 查询用户高频词
    auto userWords = QueryUserWords(pinyin, config_.maxUserWords);

    // Step 2: 使用静态方法合并
    auto merged = MergeStatic(userWords, rimeCandidates, config_);

    // Step 3: 更新索引
    CandidateUtils::UpdateIndices(merged);

    return merged;
}

std::vector<CandidateWord> CandidateMerger::MergeAll(
    const std::vector<CandidateWord>& rimeCandidates,
    const std::string& pinyin) {
    // Step 1: 查询用户高频词
    auto userWords = QueryUserWords(pinyin, config_.maxUserWords);

    // Step 2: 使用静态方法合并（返回所有候选词）
    auto merged = MergeAllStatic(userWords, rimeCandidates, config_);

    // Step 3: 更新索引
    CandidateUtils::UpdateIndices(merged);

    return merged;
}

std::vector<CandidateWord> CandidateMerger::MergeStatic(
    const std::vector<CandidateWord>& userWords,
    const std::vector<CandidateWord>& rimeCandidates,
    const MergeConfig& config) {
    std::vector<CandidateWord> result;
    result.reserve(config.pageSize);

    // 用于去重的集合
    std::unordered_set<std::string> seenWords;

    // Step 1: 如果配置为用户词优先，先添加用户高频词
    if (config.userWordsFirst) {
        int userCount = 0;
        for (const auto& word : userWords) {
            // 检查词频阈值
            if (word.frequency < config.minUserFrequency) {
                continue;
            }

            // 检查数量限制
            if (userCount >= config.maxUserWords) {
                break;
            }

            // 检查是否已存在
            if (seenWords.find(word.text) != seenWords.end()) {
                continue;
            }

            // 添加用户词
            CandidateWord candidate = word;
            candidate.isUserWord = true;
            result.push_back(candidate);
            seenWords.insert(word.text);
            userCount++;
        }
    }

    // Step 2: 添加 librime 候选词（去重）
    for (const auto& word : rimeCandidates) {
        // 检查页面大小限制
        if (static_cast<int>(result.size()) >= config.pageSize) {
            break;
        }

        // 检查是否已存在（去重）
        if (seenWords.find(word.text) != seenWords.end()) {
            continue;
        }

        result.push_back(word);
        seenWords.insert(word.text);
    }

    // Step 3: 如果配置为用户词不优先，则在末尾添加用户词
    if (!config.userWordsFirst) {
        for (const auto& word : userWords) {
            // 检查页面大小限制
            if (static_cast<int>(result.size()) >= config.pageSize) {
                break;
            }

            // 检查词频阈值
            if (word.frequency < config.minUserFrequency) {
                continue;
            }

            // 检查是否已存在
            if (seenWords.find(word.text) != seenWords.end()) {
                continue;
            }

            CandidateWord candidate = word;
            candidate.isUserWord = true;
            result.push_back(candidate);
            seenWords.insert(word.text);
        }
    }

    return result;
}

std::vector<CandidateWord> CandidateMerger::MergeAllStatic(
    const std::vector<CandidateWord>& userWords,
    const std::vector<CandidateWord>& rimeCandidates,
    const MergeConfig& config) {
    std::vector<CandidateWord> result;
    result.reserve(userWords.size() + rimeCandidates.size());

    // 用于去重的集合
    std::unordered_set<std::string> seenWords;

    // Step 1: 如果配置为用户词优先，先添加用户高频词
    if (config.userWordsFirst) {
        int userCount = 0;
        for (const auto& word : userWords) {
            // 检查词频阈值
            if (word.frequency < config.minUserFrequency) {
                continue;
            }

            // 检查数量限制
            if (userCount >= config.maxUserWords) {
                break;
            }

            // 检查是否已存在
            if (seenWords.find(word.text) != seenWords.end()) {
                continue;
            }

            // 添加用户词
            CandidateWord candidate = word;
            candidate.isUserWord = true;
            result.push_back(candidate);
            seenWords.insert(word.text);
            userCount++;
        }
    }

    // Step 2: 添加所有 librime 候选词（去重，不限制数量）
    for (const auto& word : rimeCandidates) {
        // 检查是否已存在（去重）
        if (seenWords.find(word.text) != seenWords.end()) {
            continue;
        }

        result.push_back(word);
        seenWords.insert(word.text);
    }

    // Step 3: 如果配置为用户词不优先，则在末尾添加用户词
    if (!config.userWordsFirst) {
        int userCount = 0;
        for (const auto& word : userWords) {
            // 检查词频阈值
            if (word.frequency < config.minUserFrequency) {
                continue;
            }

            // 检查数量限制
            if (userCount >= config.maxUserWords) {
                break;
            }

            // 检查是否已存在
            if (seenWords.find(word.text) != seenWords.end()) {
                continue;
            }

            CandidateWord candidate = word;
            candidate.isUserWord = true;
            result.push_back(candidate);
            seenWords.insert(word.text);
            userCount++;
        }
    }

    return result;
}

std::vector<CandidateWord> CandidateMerger::QueryUserWords(
    const std::string& pinyin, int limit) {
    std::vector<CandidateWord> result;

    if (!storage_ || !storage_->IsInitialized()) {
        return result;
    }

    // 查询用户高频词
    auto frequencies = storage_->GetTopFrequencyWords(pinyin, limit);

    for (const auto& wf : frequencies) {
        // 只返回达到阈值的词
        if (wf.frequency >= config_.minUserFrequency) {
            CandidateWord candidate;
            candidate.text = wf.word;
            candidate.pinyin = wf.pinyin;
            candidate.frequency = wf.frequency;
            candidate.isUserWord = true;
            result.push_back(candidate);
        }
    }

    return result;
}

// ==================== 工具函数实现 ====================

namespace CandidateUtils {

std::vector<CandidateWord> RemoveDuplicates(
    const std::vector<CandidateWord>& candidates) {
    std::vector<CandidateWord> result;
    std::unordered_set<std::string> seen;

    for (const auto& candidate : candidates) {
        if (seen.find(candidate.text) == seen.end()) {
            result.push_back(candidate);
            seen.insert(candidate.text);
        }
    }

    return result;
}

void UpdateIndices(std::vector<CandidateWord>& candidates, int startIndex) {
    int index = startIndex;
    for (auto& candidate : candidates) {
        candidate.index = index++;
        // 索引超过 9 后重置为 0（表示需要翻页）
        if (index > 9) {
            index = 0;
        }
    }
}

std::vector<CandidateWord> GetPage(const std::vector<CandidateWord>& candidates,
                                   int pageIndex, int pageSize) {
    std::vector<CandidateWord> result;

    int startIdx = pageIndex * pageSize;
    int endIdx = std::min(startIdx + pageSize,
                          static_cast<int>(candidates.size()));

    if (startIdx >= static_cast<int>(candidates.size())) {
        return result;
    }

    for (int i = startIdx; i < endIdx; ++i) {
        CandidateWord candidate = candidates[i];
        candidate.index = (i - startIdx) + 1;  // 1-based index
        result.push_back(candidate);
    }

    return result;
}

int GetTotalPages(int totalCandidates, int pageSize) {
    if (totalCandidates <= 0 || pageSize <= 0) {
        return 0;
    }
    return (totalCandidates + pageSize - 1) / pageSize;
}

}  // namespace CandidateUtils

}  // namespace input
}  // namespace ime
