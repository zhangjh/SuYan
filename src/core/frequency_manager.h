/**
 * FrequencyManager - 用户词频管理器
 *
 * 管理用户词频数据，支持词频记录、查询和排序。
 * 使用 SQLite 进行本地持久化存储。
 *
 * 功能：
 * - 记录用户选择的词及其频率
 * - 支持词库中已有词和用户自造词
 * - 提供词频查询和排序合并
 */

#ifndef SUYAN_CORE_FREQUENCY_MANAGER_H
#define SUYAN_CORE_FREQUENCY_MANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <QObject>

// 前向声明 SQLite
struct sqlite3;
struct sqlite3_stmt;

namespace suyan {

/**
 * 词频记录结构
 */
struct WordFrequency {
    int64_t id;                 // 数据库 ID
    std::string word;           // 词文本
    std::string pinyin;         // 拼音
    int frequency;              // 使用频率
    int64_t lastUsedAt;         // 最后使用时间戳（Unix 时间）
    int64_t createdAt;          // 创建时间戳（Unix 时间）
};

/**
 * 候选词排序信息（用于合并排序）
 */
struct CandidateFrequencyInfo {
    std::string text;           // 候选词文本
    std::string comment;        // 注释（拼音）
    int originalIndex;          // 原始索引
    int userFrequency;          // 用户词频
    double score;               // 综合得分（用于排序）
};

/**
 * FrequencyManager - 词频管理器类
 *
 * 单例模式，管理用户词频数据的存储和查询。
 */
class FrequencyManager : public QObject {
    Q_OBJECT

public:
    /**
     * 获取单例实例
     */
    static FrequencyManager& instance();

    // 禁止拷贝和移动
    FrequencyManager(const FrequencyManager&) = delete;
    FrequencyManager& operator=(const FrequencyManager&) = delete;
    FrequencyManager(FrequencyManager&&) = delete;
    FrequencyManager& operator=(FrequencyManager&&) = delete;

    /**
     * 初始化词频管理器
     *
     * @param dataDir 数据目录（存放 user_data.db）
     * @return 是否成功
     */
    bool initialize(const std::string& dataDir);

    /**
     * 关闭词频管理器
     */
    void shutdown();

    /**
     * 检查是否已初始化
     */
    bool isInitialized() const { return initialized_; }

    /**
     * 获取数据库路径
     */
    std::string getDatabasePath() const { return dbPath_; }

    // ========== 词频更新 ==========

    /**
     * 更新词频（用户选择候选词时调用）
     *
     * 如果词已存在，增加频率；否则创建新记录。
     *
     * @param word 词文本
     * @param pinyin 拼音
     * @return 是否成功
     */
    bool updateFrequency(const std::string& word, const std::string& pinyin);

    /**
     * 批量更新词频
     *
     * @param words 词和拼音的列表
     * @return 成功更新的数量
     */
    int updateFrequencyBatch(const std::vector<std::pair<std::string, std::string>>& words);

    /**
     * 设置词频（直接设置，而非增加）
     *
     * @param word 词文本
     * @param pinyin 拼音
     * @param frequency 频率值
     * @return 是否成功
     */
    bool setFrequency(const std::string& word, const std::string& pinyin, int frequency);

    // ========== 词频查询 ==========

    /**
     * 获取词频
     *
     * @param word 词文本
     * @param pinyin 拼音
     * @return 词频，不存在返回 0
     */
    int getFrequency(const std::string& word, const std::string& pinyin) const;

    /**
     * 获取词频记录
     *
     * @param word 词文本
     * @param pinyin 拼音
     * @return 词频记录，不存在返回空 optional
     */
    std::optional<WordFrequency> getWordFrequency(const std::string& word, 
                                                   const std::string& pinyin) const;

    /**
     * 根据拼音查询所有匹配的词频记录
     *
     * @param pinyin 拼音
     * @param limit 最大返回数量，0 表示不限制
     * @return 词频记录列表，按频率降序排列
     */
    std::vector<WordFrequency> queryByPinyin(const std::string& pinyin, 
                                              int limit = 0) const;

    /**
     * 获取高频词列表
     *
     * @param minFrequency 最小频率阈值
     * @param limit 最大返回数量
     * @return 词频记录列表，按频率降序排列
     */
    std::vector<WordFrequency> getHighFrequencyWords(int minFrequency, 
                                                      int limit = 100) const;

    // ========== 排序合并 ==========

    /**
     * 合并用户词频进行候选词排序
     *
     * 将用户词频与原始候选词列表合并，返回重新排序后的结果。
     * 排序算法：综合考虑原始排序和用户词频。
     *
     * @param candidates 原始候选词列表（text, comment）
     * @param pinyin 当前输入的拼音
     * @param minFrequency 参与排序的最小词频阈值
     * @return 排序后的候选词信息列表
     */
    std::vector<CandidateFrequencyInfo> mergeSortCandidates(
        const std::vector<std::pair<std::string, std::string>>& candidates,
        const std::string& pinyin,
        int minFrequency = 1) const;

    /**
     * 获取候选词的用户词频
     *
     * @param word 词文本
     * @param pinyin 拼音
     * @return 用户词频，不存在返回 0
     */
    int getCandidateUserFrequency(const std::string& word, 
                                   const std::string& pinyin) const;

    // ========== 数据管理 ==========

    /**
     * 删除词频记录
     *
     * @param word 词文本
     * @param pinyin 拼音
     * @return 是否成功
     */
    bool deleteFrequency(const std::string& word, const std::string& pinyin);

    /**
     * 清空所有词频数据
     *
     * @return 是否成功
     */
    bool clearAll();

    /**
     * 获取词频记录总数
     */
    int64_t getRecordCount() const;

    /**
     * 清理低频词（频率低于阈值的记录）
     *
     * @param minFrequency 最小频率阈值
     * @return 删除的记录数
     */
    int cleanupLowFrequency(int minFrequency);

    /**
     * 清理长期未使用的词（超过指定天数未使用）
     *
     * @param days 天数阈值
     * @return 删除的记录数
     */
    int cleanupUnused(int days);

    // ========== 导入导出 ==========

    /**
     * 导出词频数据到文件
     *
     * @param filePath 文件路径
     * @return 是否成功
     */
    bool exportToFile(const std::string& filePath) const;

    /**
     * 从文件导入词频数据
     *
     * @param filePath 文件路径
     * @param merge 是否合并（true: 合并，false: 替换）
     * @return 导入的记录数，-1 表示失败
     */
    int importFromFile(const std::string& filePath, bool merge = true);

signals:
    /**
     * 词频更新信号
     */
    void frequencyUpdated(const QString& word, const QString& pinyin, int frequency);

    /**
     * 数据清空信号
     */
    void dataCleared();

private:
    FrequencyManager();
    ~FrequencyManager();

    // 数据库操作
    bool openDatabase();
    void closeDatabase();
    bool createTables();
    bool prepareStatements();
    void finalizeStatements();

    // 内部查询
    WordFrequency rowToWordFrequency(sqlite3_stmt* stmt) const;

    // 成员变量
    bool initialized_ = false;
    std::string dataDir_;
    std::string dbPath_;
    sqlite3* db_ = nullptr;

    // 预编译语句
    sqlite3_stmt* stmtInsert_ = nullptr;
    sqlite3_stmt* stmtUpdate_ = nullptr;
    sqlite3_stmt* stmtSelect_ = nullptr;
    sqlite3_stmt* stmtSelectByPinyin_ = nullptr;
    sqlite3_stmt* stmtDelete_ = nullptr;
};

} // namespace suyan

#endif // SUYAN_CORE_FREQUENCY_MANAGER_H
