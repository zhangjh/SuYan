// src/core/dictionary/dictionary_manager_impl.h
// 多词库管理器实现

#ifndef IME_DICTIONARY_MANAGER_IMPL_H
#define IME_DICTIONARY_MANAGER_IMPL_H

#include "dictionary_manager.h"

#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <unordered_map>

namespace ime {
namespace storage {
class ILocalStorage;
}

namespace dictionary {

// 内存中的词库数据结构
struct LoadedDictionary {
    DictionaryInfo info;
    // 拼音 -> 词条列表的映射
    std::unordered_map<std::string, std::vector<WordEntry>> entries;
    // 词文本+拼音 -> 词条的映射（用于快速查找）
    std::unordered_map<std::string, WordEntry> wordIndex;

    LoadedDictionary() = default;
    explicit LoadedDictionary(const DictionaryInfo& i) : info(i) {}

    // 生成词条的唯一键
    static std::string MakeWordKey(const std::string& text,
                                   const std::string& pinyin) {
        return text + "\t" + pinyin;
    }
};

// 词库管理器实现
class DictionaryManagerImpl : public IDictionaryManager {
public:
    // 构造函数
    // @param storage 本地存储接口（外部管理生命周期）
    explicit DictionaryManagerImpl(storage::ILocalStorage* storage);

    // 析构函数
    ~DictionaryManagerImpl() override;

    // 禁止拷贝
    DictionaryManagerImpl(const DictionaryManagerImpl&) = delete;
    DictionaryManagerImpl& operator=(const DictionaryManagerImpl&) = delete;

    // ==================== IDictionaryManager 接口实现 ====================

    bool Initialize() override;
    void Shutdown() override;
    bool IsInitialized() const override;

    bool LoadDictionary(const std::string& dictId) override;
    void UnloadDictionary(const std::string& dictId) override;
    bool ReloadDictionary(const std::string& dictId) override;
    int LoadAllEnabledDictionaries() override;
    void UnloadAllDictionaries() override;

    std::optional<DictionaryInfo> GetDictionaryInfo(
        const std::string& dictId) override;
    std::vector<DictionaryInfo> GetAllDictionaries() override;
    std::vector<DictionaryInfo> GetLoadedDictionaries() override;
    std::vector<DictionaryInfo> GetEnabledDictionaries() override;

    bool RegisterDictionary(const DictionaryInfo& info) override;
    bool UnregisterDictionary(const std::string& dictId) override;
    bool SetDictionaryEnabled(const std::string& dictId, bool enabled) override;
    bool SetDictionaryPriority(const std::string& dictId, int priority) override;

    QueryResult Query(const std::string& pinyin, int limit = 100) override;
    QueryResult QueryExact(const std::string& pinyin, int limit = 100) override;
    QueryResult QueryPrefix(const std::string& pinyinPrefix,
                            int limit = 100) override;
    bool ContainsWord(const std::string& text,
                      const std::string& pinyin) override;
    int64_t GetWordFrequency(const std::string& text,
                             const std::string& pinyin) override;

    // ==================== 测试辅助方法 ====================

    // 直接添加词条到已加载的词库（用于测试）
    bool AddWordEntry(const std::string& dictId, const WordEntry& entry);

    // 获取已加载词库的词条数量
    size_t GetLoadedWordCount(const std::string& dictId) const;

private:
    // 从文件加载词库数据
    bool LoadDictionaryFromFile(LoadedDictionary& dict);

    // 解析 RIME 词库文件 (.dict.yaml)
    bool ParseRimeDictFile(const std::string& filePath, LoadedDictionary& dict);

    // 合并多词库查询结果
    // @param results 各词库的查询结果
    // @param limit 返回数量限制
    // @return 合并后的结果（按优先级和词频排序，去重）
    QueryResult MergeQueryResults(
        const std::vector<std::pair<int, std::vector<WordEntry>>>& results,
        int limit);

    // 获取已加载词库（按优先级排序）
    std::vector<LoadedDictionary*> GetLoadedDictionariesByPriority();

    // 从存储同步词库元数据
    void SyncDictionaryMetaFromStorage();

    // 保存词库元数据到存储
    bool SaveDictionaryMetaToStorage(const DictionaryInfo& info);

private:
    storage::ILocalStorage* storage_;  // 本地存储（不拥有）
    mutable std::mutex mutex_;         // 线程安全锁
    bool initialized_;                 // 是否已初始化

    // 词库元数据缓存（dictId -> info）
    std::unordered_map<std::string, DictionaryInfo> dictionaryMeta_;

    // 已加载的词库（dictId -> LoadedDictionary）
    std::unordered_map<std::string, std::unique_ptr<LoadedDictionary>>
        loadedDictionaries_;

    // 按优先级排序的已加载词库 ID 列表
    std::vector<std::string> loadedDictionaryOrder_;
};

}  // namespace dictionary
}  // namespace ime

#endif  // IME_DICTIONARY_MANAGER_IMPL_H
