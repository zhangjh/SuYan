// src/core/input/candidate_merger.h
// 候选词合并器 - 用户高频词优先注入策略

#ifndef IME_CANDIDATE_MERGER_H
#define IME_CANDIDATE_MERGER_H

#include <memory>
#include <string>
#include <vector>

namespace ime {

namespace storage {
class ILocalStorage;
}

namespace input {

// 输入模式（定义在此处供其他模块使用）
enum class InputMode {
    Chinese,    // 中文模式
    English,    // 英文模式
    TempEnglish // 临时英文模式（大写字母开头）
};

// 候选词结构
struct CandidateWord {
    std::string text;       // 候选词文本
    std::string pinyin;     // 拼音
    std::string comment;    // 注释（如词性、来源等）
    int64_t frequency;      // 词频
    int index;              // 在列表中的索引（1-9）
    bool isUserWord;        // 是否来自用户词库

    CandidateWord() : frequency(0), index(0), isUserWord(false) {}

    CandidateWord(const std::string& t, const std::string& p, int64_t f = 0)
        : text(t), pinyin(p), frequency(f), index(0), isUserWord(false) {}
};

// 合并配置
struct MergeConfig {
    int maxUserWords;           // 最多注入的用户高频词数量
    int minUserFrequency;       // 用户词频阈值（低于此值不参与注入）
    int pageSize;               // 每页候选词数量
    bool userWordsFirst;        // 用户高频词是否放在最前面

    // 默认配置
    static MergeConfig Default() {
        return MergeConfig{
            .maxUserWords = 5,
            .minUserFrequency = 3,
            .pageSize = 9,
            .userWordsFirst = true
        };
    }
};

// 候选词合并器
// 实现用户高频词优先注入策略
class CandidateMerger {
public:
    // 构造函数
    // @param storage 本地存储接口（用于查询用户词频）
    explicit CandidateMerger(storage::ILocalStorage* storage);

    // 析构函数
    ~CandidateMerger();

    // 禁止拷贝
    CandidateMerger(const CandidateMerger&) = delete;
    CandidateMerger& operator=(const CandidateMerger&) = delete;

    // 合并用户高频词和 librime 候选词
    // @param rimeCandidates librime 返回的候选词列表
    // @param pinyin 当前输入的拼音
    // @return 合并后的候选词列表
    std::vector<CandidateWord> Merge(
        const std::vector<CandidateWord>& rimeCandidates,
        const std::string& pinyin);

    // 合并用户高频词和 librime 候选词（返回所有候选词，用于分页）
    // @param rimeCandidates librime 返回的候选词列表
    // @param pinyin 当前输入的拼音
    // @return 合并后的所有候选词列表（不限制数量）
    std::vector<CandidateWord> MergeAll(
        const std::vector<CandidateWord>& rimeCandidates,
        const std::string& pinyin);

    // 合并用户高频词和 librime 候选词（静态版本，用于测试）
    // @param userWords 用户高频词列表（已按词频排序）
    // @param rimeCandidates librime 返回的候选词列表
    // @param config 合并配置
    // @return 合并后的候选词列表
    static std::vector<CandidateWord> MergeStatic(
        const std::vector<CandidateWord>& userWords,
        const std::vector<CandidateWord>& rimeCandidates,
        const MergeConfig& config = MergeConfig::Default());

    // 合并用户高频词和 librime 候选词（静态版本，返回所有候选词）
    // @param userWords 用户高频词列表（已按词频排序）
    // @param rimeCandidates librime 返回的候选词列表
    // @param config 合并配置
    // @return 合并后的所有候选词列表（不限制数量）
    static std::vector<CandidateWord> MergeAllStatic(
        const std::vector<CandidateWord>& userWords,
        const std::vector<CandidateWord>& rimeCandidates,
        const MergeConfig& config = MergeConfig::Default());

    // 获取/设置配置
    MergeConfig GetConfig() const { return config_; }
    void SetConfig(const MergeConfig& config) { config_ = config; }

    // 查询用户高频词
    // @param pinyin 拼音
    // @param limit 返回数量限制
    // @return 按词频降序排列的用户高频词列表
    std::vector<CandidateWord> QueryUserWords(const std::string& pinyin,
                                               int limit = 5);

private:
    storage::ILocalStorage* storage_;  // 本地存储（不拥有）
    MergeConfig config_;               // 合并配置
};

// 候选词工具函数
namespace CandidateUtils {

// 去重（保留第一个出现的）
std::vector<CandidateWord> RemoveDuplicates(
    const std::vector<CandidateWord>& candidates);

// 更新索引（1-9）
void UpdateIndices(std::vector<CandidateWord>& candidates, int startIndex = 1);

// 分页
std::vector<CandidateWord> GetPage(const std::vector<CandidateWord>& candidates,
                                   int pageIndex, int pageSize = 9);

// 计算总页数
int GetTotalPages(int totalCandidates, int pageSize = 9);

}  // namespace CandidateUtils

}  // namespace input
}  // namespace ime

#endif  // IME_CANDIDATE_MERGER_H
