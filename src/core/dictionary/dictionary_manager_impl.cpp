// src/core/dictionary/dictionary_manager_impl.cpp
// 多词库管理器实现

#include "dictionary_manager_impl.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_set>

#include "core/storage/local_storage.h"

namespace ime {
namespace dictionary {

DictionaryManagerImpl::DictionaryManagerImpl(storage::ILocalStorage* storage)
    : storage_(storage), initialized_(false) {}

DictionaryManagerImpl::~DictionaryManagerImpl() {
    Shutdown();
}

bool DictionaryManagerImpl::Initialize() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (initialized_) {
        return true;
    }

    if (!storage_ || !storage_->IsInitialized()) {
        return false;
    }

    // 从存储同步词库元数据
    SyncDictionaryMetaFromStorage();

    initialized_ = true;
    return true;
}

void DictionaryManagerImpl::Shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);

    // 卸载所有词库
    loadedDictionaries_.clear();
    loadedDictionaryOrder_.clear();

    initialized_ = false;
}

bool DictionaryManagerImpl::IsInitialized() const {
    return initialized_;
}

void DictionaryManagerImpl::SyncDictionaryMetaFromStorage() {
    if (!storage_) {
        return;
    }

    auto dictMetas = storage_->GetAllDictionaries();
    dictionaryMeta_.clear();

    for (const auto& meta : dictMetas) {
        DictionaryInfo info;
        info.id = meta.id;
        info.name = meta.name;
        info.type = DictionaryTypeUtils::FromString(meta.type);
        info.version = meta.localVersion;
        info.wordCount = meta.wordCount;
        info.filePath = meta.filePath;
        info.priority = meta.priority;
        info.isEnabled = meta.isEnabled;
        info.isLoaded = false;

        dictionaryMeta_[info.id] = info;
    }
}

bool DictionaryManagerImpl::SaveDictionaryMetaToStorage(
    const DictionaryInfo& info) {
    if (!storage_) {
        return false;
    }

    storage::LocalDictionaryMeta meta;
    meta.id = info.id;
    meta.name = info.name;
    meta.type = DictionaryTypeUtils::ToString(info.type);
    meta.localVersion = info.version;
    meta.wordCount = info.wordCount;
    meta.filePath = info.filePath;
    meta.priority = info.priority;
    meta.isEnabled = info.isEnabled;
    meta.checksum = "";  // TODO: 计算校验和

    return storage_->SaveDictionaryMeta(meta);
}

bool DictionaryManagerImpl::LoadDictionary(const std::string& dictId) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        return false;
    }

    // 检查词库是否已注册
    auto it = dictionaryMeta_.find(dictId);
    if (it == dictionaryMeta_.end()) {
        return false;
    }

    // 检查是否已加载
    if (loadedDictionaries_.find(dictId) != loadedDictionaries_.end()) {
        return true;  // 已加载
    }

    // 创建词库数据结构
    auto dict = std::make_unique<LoadedDictionary>(it->second);

    // 从文件加载词库数据
    if (!LoadDictionaryFromFile(*dict)) {
        return false;
    }

    // 更新状态
    dict->info.isLoaded = true;
    it->second.isLoaded = true;

    // 添加到已加载列表
    loadedDictionaries_[dictId] = std::move(dict);

    // 更新排序列表
    loadedDictionaryOrder_.push_back(dictId);
    std::sort(loadedDictionaryOrder_.begin(), loadedDictionaryOrder_.end(),
              [this](const std::string& a, const std::string& b) {
                  return dictionaryMeta_[a].priority >
                         dictionaryMeta_[b].priority;
              });

    return true;
}

void DictionaryManagerImpl::UnloadDictionary(const std::string& dictId) {
    std::lock_guard<std::mutex> lock(mutex_);

    // 从已加载列表移除
    loadedDictionaries_.erase(dictId);

    // 从排序列表移除
    loadedDictionaryOrder_.erase(
        std::remove(loadedDictionaryOrder_.begin(),
                    loadedDictionaryOrder_.end(), dictId),
        loadedDictionaryOrder_.end());

    // 更新元数据状态
    auto it = dictionaryMeta_.find(dictId);
    if (it != dictionaryMeta_.end()) {
        it->second.isLoaded = false;
    }
}

bool DictionaryManagerImpl::ReloadDictionary(const std::string& dictId) {
    UnloadDictionary(dictId);
    return LoadDictionary(dictId);
}

int DictionaryManagerImpl::LoadAllEnabledDictionaries() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        return 0;
    }

    int loadedCount = 0;

    for (const auto& [dictId, info] : dictionaryMeta_) {
        if (info.isEnabled && !info.isLoaded) {
            // 临时解锁以调用 LoadDictionary
            mutex_.unlock();
            if (LoadDictionary(dictId)) {
                loadedCount++;
            }
            mutex_.lock();
        }
    }

    return loadedCount;
}

void DictionaryManagerImpl::UnloadAllDictionaries() {
    std::lock_guard<std::mutex> lock(mutex_);

    loadedDictionaries_.clear();
    loadedDictionaryOrder_.clear();

    // 更新所有元数据状态
    for (auto& [dictId, info] : dictionaryMeta_) {
        info.isLoaded = false;
    }
}

std::optional<DictionaryInfo> DictionaryManagerImpl::GetDictionaryInfo(
    const std::string& dictId) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = dictionaryMeta_.find(dictId);
    if (it != dictionaryMeta_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<DictionaryInfo> DictionaryManagerImpl::GetAllDictionaries() {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<DictionaryInfo> result;
    result.reserve(dictionaryMeta_.size());

    for (const auto& [dictId, info] : dictionaryMeta_) {
        result.push_back(info);
    }

    // 按优先级排序
    std::sort(result.begin(), result.end(),
              [](const DictionaryInfo& a, const DictionaryInfo& b) {
                  return a.priority > b.priority;
              });

    return result;
}

std::vector<DictionaryInfo> DictionaryManagerImpl::GetLoadedDictionaries() {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<DictionaryInfo> result;

    for (const auto& dictId : loadedDictionaryOrder_) {
        auto it = dictionaryMeta_.find(dictId);
        if (it != dictionaryMeta_.end()) {
            result.push_back(it->second);
        }
    }

    return result;
}

std::vector<DictionaryInfo> DictionaryManagerImpl::GetEnabledDictionaries() {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<DictionaryInfo> result;

    for (const auto& [dictId, info] : dictionaryMeta_) {
        if (info.isEnabled) {
            result.push_back(info);
        }
    }

    // 按优先级排序
    std::sort(result.begin(), result.end(),
              [](const DictionaryInfo& a, const DictionaryInfo& b) {
                  return a.priority > b.priority;
              });

    return result;
}

bool DictionaryManagerImpl::RegisterDictionary(const DictionaryInfo& info) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        return false;
    }

    // 保存到存储
    if (!SaveDictionaryMetaToStorage(info)) {
        return false;
    }

    // 更新缓存
    dictionaryMeta_[info.id] = info;

    return true;
}

bool DictionaryManagerImpl::UnregisterDictionary(const std::string& dictId) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        return false;
    }

    // 先卸载
    mutex_.unlock();
    UnloadDictionary(dictId);
    mutex_.lock();

    // 从存储删除
    if (storage_) {
        storage_->DeleteDictionaryMeta(dictId);
    }

    // 从缓存删除
    dictionaryMeta_.erase(dictId);

    return true;
}

bool DictionaryManagerImpl::SetDictionaryEnabled(const std::string& dictId,
                                                 bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        return false;
    }

    auto it = dictionaryMeta_.find(dictId);
    if (it == dictionaryMeta_.end()) {
        return false;
    }

    // 更新存储
    if (storage_ && !storage_->SetDictionaryEnabled(dictId, enabled)) {
        return false;
    }

    // 更新缓存
    it->second.isEnabled = enabled;

    // 如果禁用，卸载词库
    if (!enabled) {
        mutex_.unlock();
        UnloadDictionary(dictId);
        mutex_.lock();
    }

    return true;
}

bool DictionaryManagerImpl::SetDictionaryPriority(const std::string& dictId,
                                                  int priority) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        return false;
    }

    auto it = dictionaryMeta_.find(dictId);
    if (it == dictionaryMeta_.end()) {
        return false;
    }

    // 更新存储
    if (storage_ && !storage_->SetDictionaryPriority(dictId, priority)) {
        return false;
    }

    // 更新缓存
    it->second.priority = priority;

    // 更新已加载词库的优先级
    auto loadedIt = loadedDictionaries_.find(dictId);
    if (loadedIt != loadedDictionaries_.end()) {
        loadedIt->second->info.priority = priority;
    }

    // 重新排序已加载词库列表
    std::sort(loadedDictionaryOrder_.begin(), loadedDictionaryOrder_.end(),
              [this](const std::string& a, const std::string& b) {
                  return dictionaryMeta_[a].priority >
                         dictionaryMeta_[b].priority;
              });

    return true;
}

QueryResult DictionaryManagerImpl::Query(const std::string& pinyin, int limit) {
    return QueryExact(pinyin, limit);
}

QueryResult DictionaryManagerImpl::QueryExact(const std::string& pinyin,
                                              int limit) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || pinyin.empty()) {
        return QueryResult();
    }

    // 收集各词库的查询结果
    std::vector<std::pair<int, std::vector<WordEntry>>> results;

    for (const auto& dictId : loadedDictionaryOrder_) {
        auto it = loadedDictionaries_.find(dictId);
        if (it == loadedDictionaries_.end()) {
            continue;
        }

        const auto& dict = it->second;
        auto entryIt = dict->entries.find(pinyin);
        if (entryIt != dict->entries.end()) {
            results.emplace_back(dict->info.priority, entryIt->second);
        }
    }

    return MergeQueryResults(results, limit);
}

QueryResult DictionaryManagerImpl::QueryPrefix(const std::string& pinyinPrefix,
                                               int limit) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || pinyinPrefix.empty()) {
        return QueryResult();
    }

    // 收集各词库的查询结果
    std::vector<std::pair<int, std::vector<WordEntry>>> results;

    for (const auto& dictId : loadedDictionaryOrder_) {
        auto it = loadedDictionaries_.find(dictId);
        if (it == loadedDictionaries_.end()) {
            continue;
        }

        const auto& dict = it->second;
        std::vector<WordEntry> matchedEntries;

        // 遍历所有拼音，查找前缀匹配
        for (const auto& [pinyin, entries] : dict->entries) {
            if (pinyin.compare(0, pinyinPrefix.length(), pinyinPrefix) == 0) {
                matchedEntries.insert(matchedEntries.end(), entries.begin(),
                                      entries.end());
            }
        }

        if (!matchedEntries.empty()) {
            results.emplace_back(dict->info.priority, std::move(matchedEntries));
        }
    }

    return MergeQueryResults(results, limit);
}

bool DictionaryManagerImpl::ContainsWord(const std::string& text,
                                         const std::string& pinyin) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        return false;
    }

    std::string key = LoadedDictionary::MakeWordKey(text, pinyin);

    for (const auto& [dictId, dict] : loadedDictionaries_) {
        if (dict->wordIndex.find(key) != dict->wordIndex.end()) {
            return true;
        }
    }

    return false;
}

int64_t DictionaryManagerImpl::GetWordFrequency(const std::string& text,
                                                const std::string& pinyin) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        return -1;
    }

    std::string key = LoadedDictionary::MakeWordKey(text, pinyin);

    // 按优先级顺序查找，返回最高优先级词库的词频
    for (const auto& dictId : loadedDictionaryOrder_) {
        auto it = loadedDictionaries_.find(dictId);
        if (it == loadedDictionaries_.end()) {
            continue;
        }

        auto entryIt = it->second->wordIndex.find(key);
        if (entryIt != it->second->wordIndex.end()) {
            return entryIt->second.frequency;
        }
    }

    return -1;
}

QueryResult DictionaryManagerImpl::MergeQueryResults(
    const std::vector<std::pair<int, std::vector<WordEntry>>>& results,
    int limit) {
    QueryResult result;

    if (results.empty()) {
        return result;
    }

    // 用于去重的集合（text + pinyin）
    std::unordered_set<std::string> seenWords;

    // 收集所有词条，按优先级和词频排序
    std::vector<WordEntry> allEntries;

    for (const auto& [priority, entries] : results) {
        for (const auto& entry : entries) {
            std::string key =
                LoadedDictionary::MakeWordKey(entry.text, entry.pinyin);

            // 去重：只保留最高优先级词库的词条
            if (seenWords.find(key) != seenWords.end()) {
                continue;
            }
            seenWords.insert(key);

            WordEntry e = entry;
            e.dictionaryPriority = priority;
            allEntries.push_back(std::move(e));
        }
    }

    // 按词频降序排序
    std::sort(allEntries.begin(), allEntries.end(),
              [](const WordEntry& a, const WordEntry& b) {
                  // 首先按词频排序
                  if (a.frequency != b.frequency) {
                      return a.frequency > b.frequency;
                  }
                  // 词频相同时按优先级排序
                  return a.dictionaryPriority > b.dictionaryPriority;
              });

    // 限制返回数量
    result.totalCount = static_cast<int>(allEntries.size());
    result.hasMore = result.totalCount > limit;

    int count = std::min(limit, result.totalCount);
    result.entries.reserve(count);
    for (int i = 0; i < count; ++i) {
        result.entries.push_back(std::move(allEntries[i]));
    }

    return result;
}

std::vector<LoadedDictionary*>
DictionaryManagerImpl::GetLoadedDictionariesByPriority() {
    std::vector<LoadedDictionary*> result;
    result.reserve(loadedDictionaryOrder_.size());

    for (const auto& dictId : loadedDictionaryOrder_) {
        auto it = loadedDictionaries_.find(dictId);
        if (it != loadedDictionaries_.end()) {
            result.push_back(it->second.get());
        }
    }

    return result;
}

bool DictionaryManagerImpl::LoadDictionaryFromFile(LoadedDictionary& dict) {
    const std::string& filePath = dict.info.filePath;

    if (filePath.empty()) {
        return false;
    }

    // 检查文件扩展名
    if (filePath.find(".dict.yaml") != std::string::npos ||
        filePath.find(".yaml") != std::string::npos) {
        return ParseRimeDictFile(filePath, dict);
    }

    // 不支持的格式
    return false;
}

bool DictionaryManagerImpl::ParseRimeDictFile(const std::string& filePath,
                                              LoadedDictionary& dict) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    bool inHeader = true;
    bool headerStarted = false;
    int64_t wordCount = 0;

    while (std::getline(file, line)) {
        // 跳过空行
        if (line.empty()) {
            continue;
        }

        // 处理 YAML 头部
        if (line == "---") {
            if (!headerStarted) {
                headerStarted = true;
                inHeader = true;
            }
            continue;
        }

        if (line == "...") {
            inHeader = false;
            continue;
        }

        if (inHeader) {
            // 解析头部信息（可选）
            continue;
        }

        // 解析词条行
        // 格式: 词\t拼音\t词频 或 词\t拼音
        std::istringstream iss(line);
        std::string text, pinyin;
        int64_t frequency = 0;

        // 使用 tab 分隔
        if (!std::getline(iss, text, '\t')) {
            continue;
        }
        if (!std::getline(iss, pinyin, '\t')) {
            continue;
        }

        // 词频是可选的
        std::string freqStr;
        if (std::getline(iss, freqStr, '\t')) {
            try {
                frequency = std::stoll(freqStr);
            } catch (...) {
                frequency = 0;
            }
        }

        // 跳过空词条
        if (text.empty() || pinyin.empty()) {
            continue;
        }

        // 创建词条
        WordEntry entry(text, pinyin, frequency, dict.info.id,
                        dict.info.priority);

        // 添加到拼音索引
        dict.entries[pinyin].push_back(entry);

        // 添加到词索引
        std::string key = LoadedDictionary::MakeWordKey(text, pinyin);
        dict.wordIndex[key] = entry;

        wordCount++;
    }

    // 更新词条数量
    dict.info.wordCount = wordCount;

    // 对每个拼音的词条按词频排序
    for (auto& [pinyin, entries] : dict.entries) {
        std::sort(entries.begin(), entries.end(),
                  [](const WordEntry& a, const WordEntry& b) {
                      return a.frequency > b.frequency;
                  });
    }

    return wordCount > 0;
}

bool DictionaryManagerImpl::AddWordEntry(const std::string& dictId,
                                         const WordEntry& entry) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = loadedDictionaries_.find(dictId);
    if (it == loadedDictionaries_.end()) {
        return false;
    }

    auto& dict = it->second;

    // 添加到拼音索引
    dict->entries[entry.pinyin].push_back(entry);

    // 添加到词索引
    std::string key = LoadedDictionary::MakeWordKey(entry.text, entry.pinyin);
    dict->wordIndex[key] = entry;

    // 更新词条数量
    dict->info.wordCount++;

    return true;
}

size_t DictionaryManagerImpl::GetLoadedWordCount(
    const std::string& dictId) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = loadedDictionaries_.find(dictId);
    if (it == loadedDictionaries_.end()) {
        return 0;
    }

    return it->second->wordIndex.size();
}

}  // namespace dictionary
}  // namespace ime
