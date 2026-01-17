// src/core/frequency/frequency_manager.h
// 用户词频管理器接口

#ifndef IME_FREQUENCY_MANAGER_H
#define IME_FREQUENCY_MANAGER_H

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ime {
namespace frequency {

// 候选词信息（用于排序）
struct CandidateWord {
    std::string text;           // 词文本
    std::string pinyin;         // 拼音
    int64_t baseFrequency;      // 基础词频（来自词库）
    int64_t userFrequency;      // 用户词频
    int64_t combinedScore;      // 综合得分（用于排序）
    std::string source;         // 来源词库 ID
};

// 词频排序配置
struct FrequencyConfig {
    double userFrequencyWeight;   // 用户词频权重 (0.0 - 1.0)
    double baseFrequencyWeight;   // 基础词频权重 (0.0 - 1.0)
    double recencyWeight;         // 最近使用权重 (0.0 - 1.0)
    int64_t recencyDecayDays;     // 最近使用衰减天数
    int maxUserFrequency;         // 用户词频上限（防止溢出）

    // 默认配置
    static FrequencyConfig Default() {
        return FrequencyConfig{
            .userFrequencyWeight = 0.6,
            .baseFrequencyWeight = 0.3,
            .recencyWeight = 0.1,
            .recencyDecayDays = 30,
            .maxUserFrequency = 100000,
        };
    }
};

// 词频管理器接口
class IFrequencyManager {
public:
    virtual ~IFrequencyManager() = default;

    // ==================== 初始化 ====================

    // 初始化词频管理器
    virtual bool Initialize() = 0;

    // 关闭词频管理器
    virtual void Shutdown() = 0;

    // 检查是否已初始化
    virtual bool IsInitialized() const = 0;

    // ==================== 词频更新 ====================

    // 记录用户选择了某个词（增加词频）
    // @param word 选择的词
    // @param pinyin 对应的拼音
    // @return 更新后的词频
    virtual int RecordWordSelection(const std::string& word,
                                    const std::string& pinyin) = 0;

    // 批量记录词选择（用于学词功能）
    // @param words 词列表 (word, pinyin) 对
    virtual void RecordWordSelections(
        const std::vector<std::pair<std::string, std::string>>& words) = 0;

    // ==================== 词频查询 ====================

    // 获取指定词的用户词频
    virtual int GetUserFrequency(const std::string& word,
                                 const std::string& pinyin) = 0;

    // 获取指定拼音的高频词列表
    // @param pinyin 拼音
    // @param limit 返回数量限制
    // @return 按词频降序排列的词列表
    virtual std::vector<CandidateWord> GetTopUserWords(const std::string& pinyin,
                                                       int limit = 10) = 0;

    // ==================== 候选词排序 ====================

    // 对候选词列表进行排序（结合基础词频和用户词频）
    // @param candidates 候选词列表（会被原地修改）
    // @param pinyin 当前输入的拼音
    virtual void SortCandidates(std::vector<CandidateWord>& candidates,
                                const std::string& pinyin) = 0;

    // 计算单个候选词的综合得分
    // @param candidate 候选词（会更新 combinedScore 字段）
    virtual void CalculateCombinedScore(CandidateWord& candidate) = 0;

    // ==================== 配置 ====================

    // 获取当前配置
    virtual FrequencyConfig GetConfig() const = 0;

    // 设置配置
    virtual void SetConfig(const FrequencyConfig& config) = 0;

    // ==================== 数据管理 ====================

    // 清空所有用户词频数据
    virtual bool ClearAllUserFrequencies() = 0;

    // 导出用户词频数据
    // @param callback 回调函数，每条记录调用一次
    virtual void ExportUserFrequencies(
        std::function<void(const std::string& word, const std::string& pinyin,
                           int frequency)>
            callback) = 0;

    // 导入用户词频数据
    // @param word 词
    // @param pinyin 拼音
    // @param frequency 词频
    virtual bool ImportUserFrequency(const std::string& word,
                                     const std::string& pinyin,
                                     int frequency) = 0;
};

}  // namespace frequency
}  // namespace ime

#endif  // IME_FREQUENCY_MANAGER_H
