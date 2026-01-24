/**
 * FrequencyManager 实现
 *
 * 使用 SQLite 进行词频数据的持久化存储。
 */

#include "frequency_manager.h"
#include <sqlite3.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <iostream>

namespace fs = std::filesystem;

namespace suyan {

// ========== 单例实现 ==========

FrequencyManager& FrequencyManager::instance() {
    static FrequencyManager instance;
    return instance;
}

FrequencyManager::FrequencyManager() = default;

FrequencyManager::~FrequencyManager() {
    shutdown();
}

// ========== 初始化和关闭 ==========

bool FrequencyManager::initialize(const std::string& dataDir) {
    if (initialized_) {
        // 如果已初始化且目录相同，直接返回成功
        if (dataDir_ == dataDir) {
            return true;
        }
        // 目录不同，先关闭
        shutdown();
    }

    dataDir_ = dataDir;
    dbPath_ = dataDir + "/user_data.db";

    // 确保目录存在
    try {
        fs::create_directories(dataDir);
    } catch (const std::exception& e) {
        std::cerr << "FrequencyManager: 创建目录失败: " << e.what() << std::endl;
        return false;
    }

    // 打开数据库
    if (!openDatabase()) {
        return false;
    }

    // 创建表
    if (!createTables()) {
        closeDatabase();
        return false;
    }

    // 准备预编译语句
    if (!prepareStatements()) {
        closeDatabase();
        return false;
    }

    initialized_ = true;
    return true;
}

void FrequencyManager::shutdown() {
    if (!initialized_) {
        return;
    }

    finalizeStatements();
    closeDatabase();
    initialized_ = false;
}

// ========== 数据库操作 ==========

bool FrequencyManager::openDatabase() {
    int rc = sqlite3_open(dbPath_.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::cerr << "FrequencyManager: 打开数据库失败: " << sqlite3_errmsg(db_) << std::endl;
        sqlite3_close(db_);
        db_ = nullptr;
        return false;
    }

    // 启用 WAL 模式提高性能
    char* errMsg = nullptr;
    rc = sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "FrequencyManager: 设置 WAL 模式失败: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        // 不是致命错误，继续
    }

    // 启用外键约束
    sqlite3_exec(db_, "PRAGMA foreign_keys=ON;", nullptr, nullptr, nullptr);

    return true;
}

void FrequencyManager::closeDatabase() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool FrequencyManager::createTables() {
    const char* createTableSQL = R"(
        CREATE TABLE IF NOT EXISTS user_word_frequency (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            word TEXT NOT NULL,
            pinyin TEXT NOT NULL,
            frequency INTEGER DEFAULT 1,
            last_used_at INTEGER DEFAULT (strftime('%s', 'now')),
            created_at INTEGER DEFAULT (strftime('%s', 'now')),
            UNIQUE(word, pinyin)
        );
        
        CREATE INDEX IF NOT EXISTS idx_frequency_pinyin 
            ON user_word_frequency(pinyin);
        CREATE INDEX IF NOT EXISTS idx_frequency_freq 
            ON user_word_frequency(frequency DESC);
        CREATE INDEX IF NOT EXISTS idx_frequency_last_used 
            ON user_word_frequency(last_used_at DESC);
    )";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, createTableSQL, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "FrequencyManager: 创建表失败: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }

    return true;
}

bool FrequencyManager::prepareStatements() {
    // INSERT OR REPLACE 语句（用于更新词频）
    const char* insertSQL = R"(
        INSERT INTO user_word_frequency (word, pinyin, frequency, last_used_at, created_at)
        VALUES (?, ?, 1, strftime('%s', 'now'), strftime('%s', 'now'))
        ON CONFLICT(word, pinyin) DO UPDATE SET
            frequency = frequency + 1,
            last_used_at = strftime('%s', 'now')
    )";

    // UPDATE 语句（直接设置词频）
    const char* updateSQL = R"(
        INSERT INTO user_word_frequency (word, pinyin, frequency, last_used_at, created_at)
        VALUES (?, ?, ?, strftime('%s', 'now'), strftime('%s', 'now'))
        ON CONFLICT(word, pinyin) DO UPDATE SET
            frequency = ?,
            last_used_at = strftime('%s', 'now')
    )";

    // SELECT 语句（查询单个词）
    const char* selectSQL = R"(
        SELECT id, word, pinyin, frequency, last_used_at, created_at
        FROM user_word_frequency
        WHERE word = ? AND pinyin = ?
    )";

    // SELECT BY PINYIN 语句
    const char* selectByPinyinSQL = R"(
        SELECT id, word, pinyin, frequency, last_used_at, created_at
        FROM user_word_frequency
        WHERE pinyin = ?
        ORDER BY frequency DESC
    )";

    // DELETE 语句
    const char* deleteSQL = R"(
        DELETE FROM user_word_frequency
        WHERE word = ? AND pinyin = ?
    )";

    int rc;

    rc = sqlite3_prepare_v2(db_, insertSQL, -1, &stmtInsert_, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "FrequencyManager: 准备 INSERT 语句失败: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    rc = sqlite3_prepare_v2(db_, updateSQL, -1, &stmtUpdate_, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "FrequencyManager: 准备 UPDATE 语句失败: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    rc = sqlite3_prepare_v2(db_, selectSQL, -1, &stmtSelect_, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "FrequencyManager: 准备 SELECT 语句失败: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    rc = sqlite3_prepare_v2(db_, selectByPinyinSQL, -1, &stmtSelectByPinyin_, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "FrequencyManager: 准备 SELECT BY PINYIN 语句失败: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    rc = sqlite3_prepare_v2(db_, deleteSQL, -1, &stmtDelete_, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "FrequencyManager: 准备 DELETE 语句失败: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    return true;
}

void FrequencyManager::finalizeStatements() {
    if (stmtInsert_) {
        sqlite3_finalize(stmtInsert_);
        stmtInsert_ = nullptr;
    }
    if (stmtUpdate_) {
        sqlite3_finalize(stmtUpdate_);
        stmtUpdate_ = nullptr;
    }
    if (stmtSelect_) {
        sqlite3_finalize(stmtSelect_);
        stmtSelect_ = nullptr;
    }
    if (stmtSelectByPinyin_) {
        sqlite3_finalize(stmtSelectByPinyin_);
        stmtSelectByPinyin_ = nullptr;
    }
    if (stmtDelete_) {
        sqlite3_finalize(stmtDelete_);
        stmtDelete_ = nullptr;
    }
}

WordFrequency FrequencyManager::rowToWordFrequency(sqlite3_stmt* stmt) const {
    WordFrequency wf;
    wf.id = sqlite3_column_int64(stmt, 0);
    wf.word = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    wf.pinyin = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    wf.frequency = sqlite3_column_int(stmt, 3);
    wf.lastUsedAt = sqlite3_column_int64(stmt, 4);
    wf.createdAt = sqlite3_column_int64(stmt, 5);
    return wf;
}

// ========== 词频更新 ==========

bool FrequencyManager::updateFrequency(const std::string& word, const std::string& pinyin) {
    if (!initialized_ || word.empty()) {
        return false;
    }

    sqlite3_reset(stmtInsert_);
    sqlite3_bind_text(stmtInsert_, 1, word.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmtInsert_, 2, pinyin.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmtInsert_);
    if (rc != SQLITE_DONE) {
        std::cerr << "FrequencyManager: 更新词频失败: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    // 获取更新后的词频
    int newFreq = getFrequency(word, pinyin);
    emit frequencyUpdated(QString::fromStdString(word), 
                          QString::fromStdString(pinyin), 
                          newFreq);

    return true;
}

int FrequencyManager::updateFrequencyBatch(
    const std::vector<std::pair<std::string, std::string>>& words) {
    if (!initialized_ || words.empty()) {
        return 0;
    }

    // 开始事务
    char* errMsg = nullptr;
    sqlite3_exec(db_, "BEGIN TRANSACTION;", nullptr, nullptr, &errMsg);

    int successCount = 0;
    for (const auto& [word, pinyin] : words) {
        if (updateFrequency(word, pinyin)) {
            successCount++;
        }
    }

    // 提交事务
    sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, &errMsg);

    return successCount;
}

bool FrequencyManager::setFrequency(const std::string& word, 
                                     const std::string& pinyin, 
                                     int frequency) {
    if (!initialized_ || word.empty() || frequency < 0) {
        return false;
    }

    sqlite3_reset(stmtUpdate_);
    sqlite3_bind_text(stmtUpdate_, 1, word.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmtUpdate_, 2, pinyin.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmtUpdate_, 3, frequency);
    sqlite3_bind_int(stmtUpdate_, 4, frequency);

    int rc = sqlite3_step(stmtUpdate_);
    if (rc != SQLITE_DONE) {
        std::cerr << "FrequencyManager: 设置词频失败: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    emit frequencyUpdated(QString::fromStdString(word), 
                          QString::fromStdString(pinyin), 
                          frequency);

    return true;
}

// ========== 词频查询 ==========

int FrequencyManager::getFrequency(const std::string& word, 
                                    const std::string& pinyin) const {
    auto wf = getWordFrequency(word, pinyin);
    return wf ? wf->frequency : 0;
}

std::optional<WordFrequency> FrequencyManager::getWordFrequency(
    const std::string& word, 
    const std::string& pinyin) const {
    if (!initialized_) {
        return std::nullopt;
    }

    sqlite3_reset(stmtSelect_);
    sqlite3_bind_text(stmtSelect_, 1, word.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmtSelect_, 2, pinyin.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmtSelect_);
    if (rc == SQLITE_ROW) {
        return rowToWordFrequency(stmtSelect_);
    }

    return std::nullopt;
}

std::vector<WordFrequency> FrequencyManager::queryByPinyin(
    const std::string& pinyin, 
    int limit) const {
    std::vector<WordFrequency> results;

    if (!initialized_) {
        return results;
    }

    sqlite3_reset(stmtSelectByPinyin_);
    sqlite3_bind_text(stmtSelectByPinyin_, 1, pinyin.c_str(), -1, SQLITE_TRANSIENT);

    int count = 0;
    while (sqlite3_step(stmtSelectByPinyin_) == SQLITE_ROW) {
        results.push_back(rowToWordFrequency(stmtSelectByPinyin_));
        count++;
        if (limit > 0 && count >= limit) {
            break;
        }
    }

    return results;
}

std::vector<WordFrequency> FrequencyManager::getHighFrequencyWords(
    int minFrequency, 
    int limit) const {
    std::vector<WordFrequency> results;

    if (!initialized_) {
        return results;
    }

    std::string sql = R"(
        SELECT id, word, pinyin, frequency, last_used_at, created_at
        FROM user_word_frequency
        WHERE frequency >= ?
        ORDER BY frequency DESC
    )";
    
    if (limit > 0) {
        sql += " LIMIT " + std::to_string(limit);
    }

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return results;
    }

    sqlite3_bind_int(stmt, 1, minFrequency);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        results.push_back(rowToWordFrequency(stmt));
    }

    sqlite3_finalize(stmt);
    return results;
}

// ========== 排序合并 ==========

std::vector<CandidateFrequencyInfo> FrequencyManager::mergeSortCandidates(
    const std::vector<std::pair<std::string, std::string>>& candidates,
    const std::string& pinyin,
    int minFrequency) const {
    
    std::vector<CandidateFrequencyInfo> result;
    result.reserve(candidates.size());

    // 获取该拼音下的所有用户词频
    auto userFreqs = queryByPinyin(pinyin, 0);
    
    // 构建词频查找表
    std::unordered_map<std::string, int> freqMap;
    for (const auto& wf : userFreqs) {
        if (wf.frequency >= minFrequency) {
            freqMap[wf.word] = wf.frequency;
        }
    }

    // 构建候选词信息
    for (size_t i = 0; i < candidates.size(); ++i) {
        CandidateFrequencyInfo info;
        info.text = candidates[i].first;
        info.comment = candidates[i].second;
        info.originalIndex = static_cast<int>(i);
        
        auto it = freqMap.find(info.text);
        info.userFrequency = (it != freqMap.end()) ? it->second : 0;
        
        // 计算综合得分
        // 算法：原始排序权重 + 用户词频权重
        // 原始排序越靠前，权重越高（使用倒数）
        // 用户词频越高，权重越高
        // 调整：让用户选择一次就能有明显效果
        double positionWeight = 1.0 / (1.0 + i * 0.2);  // 位置权重（衰减更快）
        double frequencyWeight = info.userFrequency * 1.0;  // 词频权重（增加权重）
        info.score = positionWeight + frequencyWeight;
        
        result.push_back(info);
    }

    // 按综合得分降序排序
    std::sort(result.begin(), result.end(), 
              [](const CandidateFrequencyInfo& a, const CandidateFrequencyInfo& b) {
                  return a.score > b.score;
              });

    return result;
}

int FrequencyManager::getCandidateUserFrequency(const std::string& word, 
                                                 const std::string& pinyin) const {
    return getFrequency(word, pinyin);
}

// ========== 数据管理 ==========

bool FrequencyManager::deleteFrequency(const std::string& word, 
                                        const std::string& pinyin) {
    if (!initialized_) {
        return false;
    }

    sqlite3_reset(stmtDelete_);
    sqlite3_bind_text(stmtDelete_, 1, word.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmtDelete_, 2, pinyin.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmtDelete_);
    return rc == SQLITE_DONE;
}

bool FrequencyManager::clearAll() {
    if (!initialized_) {
        return false;
    }

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, "DELETE FROM user_word_frequency;", 
                          nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "FrequencyManager: 清空数据失败: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }

    emit dataCleared();
    return true;
}

int64_t FrequencyManager::getRecordCount() const {
    if (!initialized_) {
        return 0;
    }

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, 
        "SELECT COUNT(*) FROM user_word_frequency;", 
        -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        return 0;
    }

    int64_t count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int64(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return count;
}

int FrequencyManager::cleanupLowFrequency(int minFrequency) {
    if (!initialized_) {
        return 0;
    }

    std::string sql = "DELETE FROM user_word_frequency WHERE frequency < ?;";
    
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return 0;
    }

    sqlite3_bind_int(stmt, 1, minFrequency);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return 0;
    }

    return sqlite3_changes(db_);
}

int FrequencyManager::cleanupUnused(int days) {
    if (!initialized_ || days <= 0) {
        return 0;
    }

    // 计算阈值时间戳
    int64_t threshold = std::time(nullptr) - (days * 24 * 60 * 60);

    std::string sql = "DELETE FROM user_word_frequency WHERE last_used_at < ?;";
    
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return 0;
    }

    sqlite3_bind_int64(stmt, 1, threshold);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return 0;
    }

    return sqlite3_changes(db_);
}

// ========== 导入导出 ==========

bool FrequencyManager::exportToFile(const std::string& filePath) const {
    if (!initialized_) {
        return false;
    }

    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    // 写入头部
    file << "# SuYan User Word Frequency Export\n";
    file << "# Format: word<TAB>pinyin<TAB>frequency\n";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, 
        "SELECT word, pinyin, frequency FROM user_word_frequency ORDER BY frequency DESC;",
        -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        return false;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* word = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char* pinyin = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        int frequency = sqlite3_column_int(stmt, 2);
        
        file << word << "\t" << pinyin << "\t" << frequency << "\n";
    }

    sqlite3_finalize(stmt);
    file.close();
    return true;
}

int FrequencyManager::importFromFile(const std::string& filePath, bool merge) {
    if (!initialized_) {
        return -1;
    }

    std::ifstream file(filePath);
    if (!file.is_open()) {
        return -1;
    }

    // 如果不是合并模式，先清空
    if (!merge) {
        clearAll();
    }

    // 开始事务
    char* errMsg = nullptr;
    sqlite3_exec(db_, "BEGIN TRANSACTION;", nullptr, nullptr, &errMsg);

    int importCount = 0;
    std::string line;
    
    while (std::getline(file, line)) {
        // 跳过注释和空行
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // 解析行：word<TAB>pinyin<TAB>frequency
        std::istringstream iss(line);
        std::string word, pinyin;
        int frequency;

        if (std::getline(iss, word, '\t') && 
            std::getline(iss, pinyin, '\t') && 
            (iss >> frequency)) {
            
            if (merge) {
                // 合并模式：取较大的词频
                int existingFreq = getFrequency(word, pinyin);
                if (frequency > existingFreq) {
                    setFrequency(word, pinyin, frequency);
                }
            } else {
                setFrequency(word, pinyin, frequency);
            }
            importCount++;
        }
    }

    // 提交事务
    sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, &errMsg);

    file.close();
    return importCount;
}

} // namespace suyan
