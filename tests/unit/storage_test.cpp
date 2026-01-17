// tests/unit/storage_test.cpp
// SQLite 存储层单元测试

#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>

#include "core/storage/sqlite_storage.h"

namespace ime {
namespace storage {
namespace test {

// 测试夹具
class SqliteStorageTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 使用临时数据库文件
        testDbPath_ = "test_ime_data.db";
        // 确保测试前删除旧的数据库文件
        std::remove(testDbPath_.c_str());

        storage_ = std::make_unique<SqliteStorage>(testDbPath_);
        ASSERT_TRUE(storage_->Initialize());
    }

    void TearDown() override {
        storage_->Close();
        storage_.reset();
        // 清理测试数据库文件
        std::remove(testDbPath_.c_str());
        // 清理 WAL 文件
        std::remove((testDbPath_ + "-wal").c_str());
        std::remove((testDbPath_ + "-shm").c_str());
    }

    std::string testDbPath_;
    std::unique_ptr<SqliteStorage> storage_;
};

// ==================== 初始化测试 ====================

TEST_F(SqliteStorageTest, InitializeCreatesDatabase) {
    EXPECT_TRUE(storage_->IsInitialized());
    EXPECT_TRUE(std::filesystem::exists(testDbPath_));
}

TEST_F(SqliteStorageTest, DoubleInitializeIsIdempotent) {
    EXPECT_TRUE(storage_->Initialize());
    EXPECT_TRUE(storage_->IsInitialized());
}

TEST_F(SqliteStorageTest, CloseAndReinitialize) {
    storage_->Close();
    EXPECT_FALSE(storage_->IsInitialized());

    EXPECT_TRUE(storage_->Initialize());
    EXPECT_TRUE(storage_->IsInitialized());
}

// ==================== 词库元数据测试 ====================

TEST_F(SqliteStorageTest, SaveAndGetDictionaryMeta) {
    LocalDictionaryMeta meta;
    meta.id = "test_dict_001";
    meta.name = "测试词库";
    meta.type = "base";
    meta.localVersion = "1.0.0";
    meta.cloudVersion = "1.0.1";
    meta.wordCount = 50000;
    meta.filePath = "/path/to/dict.yaml";
    meta.checksum = "abc123";
    meta.priority = 10;
    meta.isEnabled = true;

    EXPECT_TRUE(storage_->SaveDictionaryMeta(meta));

    auto retrieved = storage_->GetDictionaryMeta("test_dict_001");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->id, meta.id);
    EXPECT_EQ(retrieved->name, meta.name);
    EXPECT_EQ(retrieved->type, meta.type);
    EXPECT_EQ(retrieved->localVersion, meta.localVersion);
    EXPECT_EQ(retrieved->cloudVersion, meta.cloudVersion);
    EXPECT_EQ(retrieved->wordCount, meta.wordCount);
    EXPECT_EQ(retrieved->filePath, meta.filePath);
    EXPECT_EQ(retrieved->checksum, meta.checksum);
    EXPECT_EQ(retrieved->priority, meta.priority);
    EXPECT_EQ(retrieved->isEnabled, meta.isEnabled);
}

TEST_F(SqliteStorageTest, GetNonExistentDictionaryReturnsNullopt) {
    auto result = storage_->GetDictionaryMeta("non_existent");
    EXPECT_FALSE(result.has_value());
}

TEST_F(SqliteStorageTest, UpdateDictionaryMeta) {
    LocalDictionaryMeta meta;
    meta.id = "test_dict_002";
    meta.name = "测试词库2";
    meta.type = "extended";
    meta.localVersion = "1.0.0";
    meta.wordCount = 10000;
    meta.filePath = "/path/to/dict2.yaml";
    meta.checksum = "def456";
    meta.priority = 5;
    meta.isEnabled = true;

    EXPECT_TRUE(storage_->SaveDictionaryMeta(meta));

    // 更新元数据
    meta.name = "更新后的词库";
    meta.localVersion = "2.0.0";
    meta.wordCount = 20000;
    EXPECT_TRUE(storage_->SaveDictionaryMeta(meta));

    auto retrieved = storage_->GetDictionaryMeta("test_dict_002");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->name, "更新后的词库");
    EXPECT_EQ(retrieved->localVersion, "2.0.0");
    EXPECT_EQ(retrieved->wordCount, 20000);
}

TEST_F(SqliteStorageTest, GetAllDictionaries) {
    // 插入多个词库
    for (int i = 0; i < 3; ++i) {
        LocalDictionaryMeta meta;
        meta.id = "dict_" + std::to_string(i);
        meta.name = "词库" + std::to_string(i);
        meta.type = "base";
        meta.localVersion = "1.0.0";
        meta.wordCount = 1000 * (i + 1);
        meta.filePath = "/path/to/dict" + std::to_string(i) + ".yaml";
        meta.checksum = "checksum" + std::to_string(i);
        meta.priority = i;
        meta.isEnabled = true;
        EXPECT_TRUE(storage_->SaveDictionaryMeta(meta));
    }

    auto all = storage_->GetAllDictionaries();
    EXPECT_EQ(all.size(), 3);
    // 应该按优先级降序排列
    EXPECT_EQ(all[0].id, "dict_2");
    EXPECT_EQ(all[1].id, "dict_1");
    EXPECT_EQ(all[2].id, "dict_0");
}

TEST_F(SqliteStorageTest, GetEnabledDictionaries) {
    // 插入词库，部分启用
    for (int i = 0; i < 4; ++i) {
        LocalDictionaryMeta meta;
        meta.id = "dict_enabled_" + std::to_string(i);
        meta.name = "词库" + std::to_string(i);
        meta.type = "base";
        meta.localVersion = "1.0.0";
        meta.wordCount = 1000;
        meta.filePath = "/path/to/dict.yaml";
        meta.checksum = "checksum";
        meta.priority = i;
        meta.isEnabled = (i % 2 == 0);  // 偶数启用
        EXPECT_TRUE(storage_->SaveDictionaryMeta(meta));
    }

    auto enabled = storage_->GetEnabledDictionaries();
    EXPECT_EQ(enabled.size(), 2);
    for (const auto& dict : enabled) {
        EXPECT_TRUE(dict.isEnabled);
    }
}

TEST_F(SqliteStorageTest, UpdateDictionaryVersion) {
    LocalDictionaryMeta meta;
    meta.id = "dict_version_test";
    meta.name = "版本测试词库";
    meta.type = "base";
    meta.localVersion = "1.0.0";
    meta.wordCount = 1000;
    meta.filePath = "/path/to/dict.yaml";
    meta.checksum = "checksum";
    meta.priority = 0;
    meta.isEnabled = true;
    EXPECT_TRUE(storage_->SaveDictionaryMeta(meta));

    EXPECT_TRUE(storage_->UpdateDictionaryVersion("dict_version_test", "2.0.0", "2.1.0"));

    auto retrieved = storage_->GetDictionaryMeta("dict_version_test");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->localVersion, "2.0.0");
    EXPECT_EQ(retrieved->cloudVersion, "2.1.0");
}

TEST_F(SqliteStorageTest, SetDictionaryEnabled) {
    LocalDictionaryMeta meta;
    meta.id = "dict_enable_test";
    meta.name = "启用测试词库";
    meta.type = "base";
    meta.localVersion = "1.0.0";
    meta.wordCount = 1000;
    meta.filePath = "/path/to/dict.yaml";
    meta.checksum = "checksum";
    meta.priority = 0;
    meta.isEnabled = true;
    EXPECT_TRUE(storage_->SaveDictionaryMeta(meta));

    EXPECT_TRUE(storage_->SetDictionaryEnabled("dict_enable_test", false));

    auto retrieved = storage_->GetDictionaryMeta("dict_enable_test");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_FALSE(retrieved->isEnabled);
}

TEST_F(SqliteStorageTest, DeleteDictionaryMeta) {
    LocalDictionaryMeta meta;
    meta.id = "dict_delete_test";
    meta.name = "删除测试词库";
    meta.type = "base";
    meta.localVersion = "1.0.0";
    meta.wordCount = 1000;
    meta.filePath = "/path/to/dict.yaml";
    meta.checksum = "checksum";
    meta.priority = 0;
    meta.isEnabled = true;
    EXPECT_TRUE(storage_->SaveDictionaryMeta(meta));

    EXPECT_TRUE(storage_->DeleteDictionaryMeta("dict_delete_test"));

    auto retrieved = storage_->GetDictionaryMeta("dict_delete_test");
    EXPECT_FALSE(retrieved.has_value());
}

// ==================== 词频测试 ====================

TEST_F(SqliteStorageTest, IncrementWordFrequency) {
    EXPECT_TRUE(storage_->IncrementWordFrequency("你好", "nihao"));
    EXPECT_EQ(storage_->GetWordFrequency("你好", "nihao"), 1);

    EXPECT_TRUE(storage_->IncrementWordFrequency("你好", "nihao"));
    EXPECT_EQ(storage_->GetWordFrequency("你好", "nihao"), 2);

    EXPECT_TRUE(storage_->IncrementWordFrequency("你好", "nihao"));
    EXPECT_EQ(storage_->GetWordFrequency("你好", "nihao"), 3);
}

TEST_F(SqliteStorageTest, GetWordFrequencyNonExistent) {
    EXPECT_EQ(storage_->GetWordFrequency("不存在", "bucunzai"), 0);
}

TEST_F(SqliteStorageTest, GetTopFrequencyWords) {
    // 插入多个词频记录
    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j <= i; ++j) {
            storage_->IncrementWordFrequency("词" + std::to_string(i), "ci");
        }
    }

    auto top = storage_->GetTopFrequencyWords("ci", 3);
    EXPECT_EQ(top.size(), 3);
    // 应该按频率降序排列
    EXPECT_EQ(top[0].word, "词4");
    EXPECT_EQ(top[0].frequency, 5);
    EXPECT_EQ(top[1].word, "词3");
    EXPECT_EQ(top[1].frequency, 4);
    EXPECT_EQ(top[2].word, "词2");
    EXPECT_EQ(top[2].frequency, 3);
}

TEST_F(SqliteStorageTest, DeleteWordFrequency) {
    storage_->IncrementWordFrequency("删除测试", "shanchuceshi");
    EXPECT_EQ(storage_->GetWordFrequency("删除测试", "shanchuceshi"), 1);

    EXPECT_TRUE(storage_->DeleteWordFrequency("删除测试", "shanchuceshi"));
    EXPECT_EQ(storage_->GetWordFrequency("删除测试", "shanchuceshi"), 0);
}

TEST_F(SqliteStorageTest, ClearAllWordFrequencies) {
    storage_->IncrementWordFrequency("词1", "ci1");
    storage_->IncrementWordFrequency("词2", "ci2");
    storage_->IncrementWordFrequency("词3", "ci3");

    auto all = storage_->GetAllWordFrequencies();
    EXPECT_EQ(all.size(), 3);

    EXPECT_TRUE(storage_->ClearAllWordFrequencies());

    all = storage_->GetAllWordFrequencies();
    EXPECT_EQ(all.size(), 0);
}

// ==================== 配置测试 ====================

TEST_F(SqliteStorageTest, DefaultConfigsExist) {
    // 检查默认配置是否存在
    EXPECT_EQ(storage_->GetConfig("cloud.enabled"), "true");
    EXPECT_EQ(storage_->GetConfig("cloud.check_interval"), "86400");
    EXPECT_EQ(storage_->GetConfig("input.default_mode"), "chinese");
    EXPECT_EQ(storage_->GetConfig("input.page_size"), "9");
}

TEST_F(SqliteStorageTest, SetAndGetConfig) {
    EXPECT_TRUE(storage_->SetConfig("test.key", "test_value"));
    EXPECT_EQ(storage_->GetConfig("test.key"), "test_value");
}

TEST_F(SqliteStorageTest, GetConfigWithDefault) {
    EXPECT_EQ(storage_->GetConfig("non.existent", "default"), "default");
}

TEST_F(SqliteStorageTest, UpdateConfig) {
    EXPECT_TRUE(storage_->SetConfig("update.key", "value1"));
    EXPECT_EQ(storage_->GetConfig("update.key"), "value1");

    EXPECT_TRUE(storage_->SetConfig("update.key", "value2"));
    EXPECT_EQ(storage_->GetConfig("update.key"), "value2");
}

TEST_F(SqliteStorageTest, DeleteConfig) {
    EXPECT_TRUE(storage_->SetConfig("delete.key", "value"));
    EXPECT_EQ(storage_->GetConfig("delete.key"), "value");

    EXPECT_TRUE(storage_->DeleteConfig("delete.key"));
    EXPECT_EQ(storage_->GetConfig("delete.key", "default"), "default");
}

TEST_F(SqliteStorageTest, GetAllConfigs) {
    auto configs = storage_->GetAllConfigs();
    EXPECT_GE(configs.size(), 6);  // 至少有 6 个默认配置
}

// ==================== 下载任务测试 ====================

TEST_F(SqliteStorageTest, SaveAndGetDownloadTask) {
    DownloadTask task;
    task.dictionaryId = "dict_download_001";
    task.version = "1.0.0";
    task.downloadUrl = "https://example.com/dict.zip";
    task.totalSize = 1024000;
    task.downloadedSize = 0;
    task.tempFilePath = "/tmp/dict_download.tmp";
    task.status = DownloadStatus::Pending;
    task.errorMessage = "";

    EXPECT_TRUE(storage_->SaveDownloadTask(task));

    auto retrieved = storage_->GetDownloadTask("dict_download_001");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->dictionaryId, task.dictionaryId);
    EXPECT_EQ(retrieved->version, task.version);
    EXPECT_EQ(retrieved->downloadUrl, task.downloadUrl);
    EXPECT_EQ(retrieved->totalSize, task.totalSize);
    EXPECT_EQ(retrieved->status, DownloadStatus::Pending);
}

TEST_F(SqliteStorageTest, UpdateDownloadProgress) {
    DownloadTask task;
    task.dictionaryId = "dict_progress_001";
    task.version = "1.0.0";
    task.downloadUrl = "https://example.com/dict.zip";
    task.totalSize = 1024000;
    task.downloadedSize = 0;
    task.status = DownloadStatus::Pending;

    EXPECT_TRUE(storage_->SaveDownloadTask(task));

    EXPECT_TRUE(storage_->UpdateDownloadProgress(
        "dict_progress_001", 512000, DownloadStatus::Downloading));

    auto retrieved = storage_->GetDownloadTask("dict_progress_001");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->downloadedSize, 512000);
    EXPECT_EQ(retrieved->status, DownloadStatus::Downloading);
}

TEST_F(SqliteStorageTest, DeleteDownloadTask) {
    DownloadTask task;
    task.dictionaryId = "dict_delete_task";
    task.version = "1.0.0";
    task.downloadUrl = "https://example.com/dict.zip";
    task.totalSize = 1024000;
    task.status = DownloadStatus::Pending;

    EXPECT_TRUE(storage_->SaveDownloadTask(task));
    EXPECT_TRUE(storage_->GetDownloadTask("dict_delete_task").has_value());

    EXPECT_TRUE(storage_->DeleteDownloadTask("dict_delete_task"));
    EXPECT_FALSE(storage_->GetDownloadTask("dict_delete_task").has_value());
}

TEST_F(SqliteStorageTest, GetPendingDownloadTasks) {
    // 创建不同状态的下载任务
    std::vector<std::pair<std::string, DownloadStatus>> tasks = {
        {"pending_task", DownloadStatus::Pending},
        {"downloading_task", DownloadStatus::Downloading},
        {"paused_task", DownloadStatus::Paused},
        {"completed_task", DownloadStatus::Completed},
        {"failed_task", DownloadStatus::Failed},
    };

    for (const auto& [id, status] : tasks) {
        DownloadTask task;
        task.dictionaryId = id;
        task.version = "1.0.0";
        task.downloadUrl = "https://example.com/" + id + ".zip";
        task.totalSize = 1024000;
        task.status = status;
        EXPECT_TRUE(storage_->SaveDownloadTask(task));
    }

    auto pending = storage_->GetPendingDownloadTasks();
    EXPECT_EQ(pending.size(), 3);  // pending, downloading, paused

    for (const auto& task : pending) {
        EXPECT_TRUE(task.status == DownloadStatus::Pending ||
                    task.status == DownloadStatus::Downloading ||
                    task.status == DownloadStatus::Paused);
    }
}

}  // namespace test
}  // namespace storage
}  // namespace ime
