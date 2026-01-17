// src/core/dictionary/dictionary_manager.h
// 多词库管理器接口

#ifndef IME_DICTIONARY_MANAGER_H
#define IME_DICTIONARY_MANAGER_H

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace ime {
namespace dictionary {

// 词库类型
enum class DictionaryType {
    Base,      // 基础词库
    Extended,  // 扩展词库
    Industry,  // 行业词库
    User       // 用户词库
};

// 词库信息
struct DictionaryInfo {
    std::string id;            // 词库唯一标识
    std::string name;          // 词库名称
    DictionaryType type;       // 词库类型
    std::string version;       // 版本号
    int64_t wordCount;         // 词条数量
    std::string filePath;      // 文件路径
    int priority;              // 优先级（数值越大优先级越高）
    bool isEnabled;            // 是否启用
    bool isLoaded;             // 是否已加载到内存

    DictionaryInfo()
        : type(DictionaryType::Base),
          wordCount(0),
          priority(0),
          isEnabled(true),
          isLoaded(false) {}
};

// 词条信息
struct WordEntry {
    std::string text;          // 词文本
    std::string pinyin;        // 拼音
    int64_t frequency;         // 词频
    std::string dictionaryId;  // 来源词库 ID
    int dictionaryPriority;    // 来源词库优先级

    WordEntry() : frequency(0), dictionaryPriority(0) {}

    WordEntry(const std::string& t, const std::string& p, int64_t f,
              const std::string& dictId = "", int dictPriority = 0)
        : text(t),
          pinyin(p),
          frequency(f),
          dictionaryId(dictId),
          dictionaryPriority(dictPriority) {}
};

// 词库查询结果
struct QueryResult {
    std::vector<WordEntry> entries;  // 查询到的词条
    int totalCount;                  // 总匹配数量（可能大于 entries.size()）
    bool hasMore;                    // 是否还有更多结果

    QueryResult() : totalCount(0), hasMore(false) {}
};

// 词库管理器接口
class IDictionaryManager {
public:
    virtual ~IDictionaryManager() = default;

    // ==================== 初始化 ====================

    // 初始化词库管理器
    virtual bool Initialize() = 0;

    // 关闭词库管理器
    virtual void Shutdown() = 0;

    // 检查是否已初始化
    virtual bool IsInitialized() const = 0;

    // ==================== 词库加载/卸载 ====================

    // 加载词库
    // @param dictId 词库 ID
    // @return 是否成功
    virtual bool LoadDictionary(const std::string& dictId) = 0;

    // 卸载词库
    // @param dictId 词库 ID
    virtual void UnloadDictionary(const std::string& dictId) = 0;

    // 重新加载词库
    // @param dictId 词库 ID
    // @return 是否成功
    virtual bool ReloadDictionary(const std::string& dictId) = 0;

    // 加载所有已启用的词库
    // @return 成功加载的词库数量
    virtual int LoadAllEnabledDictionaries() = 0;

    // 卸载所有词库
    virtual void UnloadAllDictionaries() = 0;

    // ==================== 词库信息查询 ====================

    // 获取词库信息
    virtual std::optional<DictionaryInfo> GetDictionaryInfo(
        const std::string& dictId) = 0;

    // 获取所有词库信息
    virtual std::vector<DictionaryInfo> GetAllDictionaries() = 0;

    // 获取已加载的词库列表
    virtual std::vector<DictionaryInfo> GetLoadedDictionaries() = 0;

    // 获取已启用的词库列表
    virtual std::vector<DictionaryInfo> GetEnabledDictionaries() = 0;

    // ==================== 词库管理 ====================

    // 注册词库（添加词库元数据）
    // @param info 词库信息
    // @return 是否成功
    virtual bool RegisterDictionary(const DictionaryInfo& info) = 0;

    // 注销词库（移除词库元数据）
    // @param dictId 词库 ID
    // @return 是否成功
    virtual bool UnregisterDictionary(const std::string& dictId) = 0;

    // 设置词库启用状态
    // @param dictId 词库 ID
    // @param enabled 是否启用
    // @return 是否成功
    virtual bool SetDictionaryEnabled(const std::string& dictId,
                                      bool enabled) = 0;

    // 设置词库优先级
    // @param dictId 词库 ID
    // @param priority 优先级
    // @return 是否成功
    virtual bool SetDictionaryPriority(const std::string& dictId,
                                       int priority) = 0;

    // ==================== 词条查询 ====================

    // 查询词条（合并多词库结果）
    // @param pinyin 拼音
    // @param limit 返回数量限制
    // @return 查询结果
    virtual QueryResult Query(const std::string& pinyin, int limit = 100) = 0;

    // 精确查询词条
    // @param pinyin 完整拼音
    // @param limit 返回数量限制
    // @return 查询结果
    virtual QueryResult QueryExact(const std::string& pinyin,
                                   int limit = 100) = 0;

    // 前缀查询词条
    // @param pinyinPrefix 拼音前缀
    // @param limit 返回数量限制
    // @return 查询结果
    virtual QueryResult QueryPrefix(const std::string& pinyinPrefix,
                                    int limit = 100) = 0;

    // 检查词条是否存在
    // @param text 词文本
    // @param pinyin 拼音
    // @return 是否存在
    virtual bool ContainsWord(const std::string& text,
                              const std::string& pinyin) = 0;

    // 获取词条的词频（使用最高优先级词库的词频）
    // @param text 词文本
    // @param pinyin 拼音
    // @return 词频，如果不存在返回 -1
    virtual int64_t GetWordFrequency(const std::string& text,
                                     const std::string& pinyin) = 0;
};

// 词库类型转换工具
namespace DictionaryTypeUtils {

inline std::string ToString(DictionaryType type) {
    switch (type) {
        case DictionaryType::Base:
            return "base";
        case DictionaryType::Extended:
            return "extended";
        case DictionaryType::Industry:
            return "industry";
        case DictionaryType::User:
            return "user";
        default:
            return "base";
    }
}

inline DictionaryType FromString(const std::string& str) {
    if (str == "extended") {
        return DictionaryType::Extended;
    } else if (str == "industry") {
        return DictionaryType::Industry;
    } else if (str == "user") {
        return DictionaryType::User;
    }
    return DictionaryType::Base;
}

}  // namespace DictionaryTypeUtils

}  // namespace dictionary
}  // namespace ime

#endif  // IME_DICTIONARY_MANAGER_H
