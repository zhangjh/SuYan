// tests/unit/frequency_test.cpp
// 用户词频管理器单元测试

#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <memory>

#include "core/frequency/frequency_manager_impl.h"
#include "core/storage/sqlite_storage.h"

namespace ime {
namespace frequency {
namespace test {

// 测试夹具
class FrequencyManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 使用临时数据库文件
        testDbPath_ = "test_frequency_data.db";
        // 确保测试前删除旧的数据库文件
        std::remove(testDbPath_.c_str());

        storage_ = std::make_unique<storage::SqliteStorage>(testDbPath_);
        ASSERT_TRUE(storage_->Initialize());

        manager_ = std::make_unique<FrequencyManagerImpl>(storage_.get());
        ASSERT_TRUE(manager_->Initialize());
    }

    void TearDown() override {
        manager_->Shutdown();
        manager_.reset();
        storage_->Close();
        storage_.reset();
        // 清理测试数据库文件
        std::remove(testDbPath_.c_str());
        std::remove((testDbPath_ + "-wal").c_str());
        std::remove((testDbPath_ + "-shm").c_str());
    }

    std::string testDbPath_;
    std::unique_ptr<storage::SqliteStorage> storage_;
    std::unique_ptr<FrequencyManagerImpl> manager_;
};

// ==================== 初始化测试 ====================

TEST_F(FrequencyManagerTest, InitializeSucceeds) {
    EXPECT_TRUE(manager_->IsInitialized());
}

TEST_F(FrequencyManagerTest, DoubleInitializeIsIdempotent) {
    EXPECT_TRUE(manager_->Initialize());
    EXPECT_TRUE(manager_->IsInitialized());
}

TEST_F(FrequencyManagerTest, ShutdownAndReinitialize) {
    manager_->Shutdown();
    EXPECT_FALSE(manager_->IsInitialized());

    EXPECT_TRUE(manager_->Initialize());
    EXPECT_TRUE(manager_->IsInitialized());
}

// ==================== 词频记录测试 ====================

TEST_F(FrequencyManagerTest, RecordWordSelectionIncreasesFrequency) {
    int freq1 = manager_->RecordWordSelection("你好", "nihao");
    EXPECT_EQ(freq1, 1);

    int freq2 = manager_->RecordWordSelection("你好", "nihao");
    EXPECT_EQ(freq2, 2);

    int freq3 = manager_->RecordWordSelection("你好", "nihao");
    EXPECT_EQ(freq3, 3);
}

TEST_F(FrequencyManagerTest, RecordWordSelectionsMultiple) {
    std::vector<std::pair<std::string, std::string>> words = {
        {"中国", "zhongguo"},
        {"人民", "renmin"},
        {"中国", "zhongguo"},  // 重复
    };

    manager_->RecordWordSelections(words);

    EXPECT_EQ(manager_->GetUserFrequency("中国", "zhongguo"), 2);
    EXPECT_EQ(manager_->GetUserFrequency("人民", "renmin"), 1);
}

TEST_F(FrequencyManagerTest, GetUserFrequencyNonExistent) {
    EXPECT_EQ(manager_->GetUserFrequency("不存在", "bucunzai"), 0);
}

// ==================== 高频词查询测试 ====================

TEST_F(FrequencyManagerTest, GetTopUserWords) {
    // 创建不同频率的词
    for (int i = 0; i < 5; ++i) {
        manager_->RecordWordSelection("词" + std::to_string(i), "ci");
    }
    for (int i = 0; i < 4; ++i) {
        manager_->RecordWordSelection("词4", "ci");  // 词4 总共 5 次
    }
    for (int i = 0; i < 3; ++i) {
        manager_->RecordWordSelection("词3", "ci");  // 词3 总共 4 次
    }

    auto top = manager_->GetTopUserWords("ci", 3);
    EXPECT_EQ(top.size(), 3);

    // 应该按频率降序排列
    EXPECT_EQ(top[0].text, "词4");
    EXPECT_EQ(top[0].userFrequency, 5);
    EXPECT_EQ(top[1].text, "词3");
    EXPECT_EQ(top[1].userFrequency, 4);
}

TEST_F(FrequencyManagerTest, GetTopUserWordsEmpty) {
    auto top = manager_->GetTopUserWords("nonexistent", 10);
    EXPECT_TRUE(top.empty());
}

// ==================== 候选词排序测试 ====================

TEST_F(FrequencyManagerTest, SortCandidatesWithUserFrequency) {
    // 先记录一些用户词频
    for (int i = 0; i < 10; ++i) {
        manager_->RecordWordSelection("高频词", "test");
    }
    for (int i = 0; i < 5; ++i) {
        manager_->RecordWordSelection("中频词", "test");
    }
    manager_->RecordWordSelection("低频词", "test");

    // 创建候选词列表（初始顺序与词频无关）
    std::vector<CandidateWord> candidates = {
        {.text = "低频词",
         .pinyin = "test",
         .baseFrequency = 1000,
         .userFrequency = 0,
         .combinedScore = 0,
         .source = "base"},
        {.text = "高频词",
         .pinyin = "test",
         .baseFrequency = 500,
         .userFrequency = 0,
         .combinedScore = 0,
         .source = "base"},
        {.text = "中频词",
         .pinyin = "test",
         .baseFrequency = 800,
         .userFrequency = 0,
         .combinedScore = 0,
         .source = "base"},
    };

    manager_->SortCandidates(candidates, "test");

    // 排序后，高频词应该在前面（用户词频权重较高）
    EXPECT_EQ(candidates[0].text, "高频词");
    EXPECT_EQ(candidates[0].userFrequency, 10);
}

TEST_F(FrequencyManagerTest, SortCandidatesWithoutUserFrequency) {
    // 没有用户词频时，应该按基础词频排序
    std::vector<CandidateWord> candidates = {
        {.text = "词A",
         .pinyin = "test",
         .baseFrequency = 100,
         .userFrequency = 0,
         .combinedScore = 0,
         .source = "base"},
        {.text = "词B",
         .pinyin = "test",
         .baseFrequency = 1000,
         .userFrequency = 0,
         .combinedScore = 0,
         .source = "base"},
        {.text = "词C",
         .pinyin = "test",
         .baseFrequency = 500,
         .userFrequency = 0,
         .combinedScore = 0,
         .source = "base"},
    };

    manager_->SortCandidates(candidates, "test");

    // 按基础词频降序
    EXPECT_EQ(candidates[0].text, "词B");
    EXPECT_EQ(candidates[1].text, "词C");
    EXPECT_EQ(candidates[2].text, "词A");
}

// ==================== 配置测试 ====================

TEST_F(FrequencyManagerTest, GetDefaultConfig) {
    auto config = manager_->GetConfig();
    EXPECT_DOUBLE_EQ(config.userFrequencyWeight, 0.6);
    EXPECT_DOUBLE_EQ(config.baseFrequencyWeight, 0.3);
    EXPECT_DOUBLE_EQ(config.recencyWeight, 0.1);
}

TEST_F(FrequencyManagerTest, SetAndGetConfig) {
    FrequencyConfig newConfig;
    newConfig.userFrequencyWeight = 0.7;
    newConfig.baseFrequencyWeight = 0.2;
    newConfig.recencyWeight = 0.1;
    newConfig.recencyDecayDays = 14;
    newConfig.maxUserFrequency = 50000;

    manager_->SetConfig(newConfig);

    auto retrieved = manager_->GetConfig();
    EXPECT_DOUBLE_EQ(retrieved.userFrequencyWeight, 0.7);
    EXPECT_DOUBLE_EQ(retrieved.baseFrequencyWeight, 0.2);
    EXPECT_EQ(retrieved.recencyDecayDays, 14);
    EXPECT_EQ(retrieved.maxUserFrequency, 50000);
}

// ==================== 数据管理测试 ====================

TEST_F(FrequencyManagerTest, ClearAllUserFrequencies) {
    manager_->RecordWordSelection("词1", "ci1");
    manager_->RecordWordSelection("词2", "ci2");
    manager_->RecordWordSelection("词3", "ci3");

    EXPECT_EQ(manager_->GetUserFrequency("词1", "ci1"), 1);

    EXPECT_TRUE(manager_->ClearAllUserFrequencies());

    EXPECT_EQ(manager_->GetUserFrequency("词1", "ci1"), 0);
    EXPECT_EQ(manager_->GetUserFrequency("词2", "ci2"), 0);
    EXPECT_EQ(manager_->GetUserFrequency("词3", "ci3"), 0);
}

TEST_F(FrequencyManagerTest, ExportUserFrequencies) {
    manager_->RecordWordSelection("导出词1", "daochu1");
    manager_->RecordWordSelection("导出词1", "daochu1");
    manager_->RecordWordSelection("导出词2", "daochu2");

    std::vector<std::tuple<std::string, std::string, int>> exported;

    manager_->ExportUserFrequencies(
        [&exported](const std::string& word, const std::string& pinyin,
                    int frequency) {
            exported.emplace_back(word, pinyin, frequency);
        });

    EXPECT_EQ(exported.size(), 2);

    // 检查导出的数据
    bool found1 = false, found2 = false;
    for (const auto& [word, pinyin, freq] : exported) {
        if (word == "导出词1" && pinyin == "daochu1" && freq == 2) {
            found1 = true;
        }
        if (word == "导出词2" && pinyin == "daochu2" && freq == 1) {
            found2 = true;
        }
    }
    EXPECT_TRUE(found1);
    EXPECT_TRUE(found2);
}

TEST_F(FrequencyManagerTest, ImportUserFrequency) {
    EXPECT_TRUE(manager_->ImportUserFrequency("导入词", "daoru", 5));
    EXPECT_EQ(manager_->GetUserFrequency("导入词", "daoru"), 5);
}

// ==================== 综合得分计算测试 ====================

TEST_F(FrequencyManagerTest, CalculateCombinedScoreWithHighUserFrequency) {
    CandidateWord candidate;
    candidate.text = "测试词";
    candidate.pinyin = "ceshici";
    candidate.baseFrequency = 1000;
    candidate.userFrequency = 100;
    candidate.combinedScore = 0;

    manager_->CalculateCombinedScore(candidate);

    // 高用户词频应该产生较高的综合得分
    EXPECT_GT(candidate.combinedScore, 0);
}

TEST_F(FrequencyManagerTest, CalculateCombinedScoreWithZeroUserFrequency) {
    CandidateWord candidate;
    candidate.text = "测试词";
    candidate.pinyin = "ceshici";
    candidate.baseFrequency = 1000;
    candidate.userFrequency = 0;
    candidate.combinedScore = 0;

    manager_->CalculateCombinedScore(candidate);

    // 即使没有用户词频，基础词频也应该产生得分
    EXPECT_GT(candidate.combinedScore, 0);
}

TEST_F(FrequencyManagerTest, UserFrequencyAffectsSorting) {
    // 这个测试验证 Requirements 5.2：用户词频影响排序

    // 创建两个候选词，基础词频相同
    std::vector<CandidateWord> candidates = {
        {.text = "无用户词频",
         .pinyin = "test",
         .baseFrequency = 1000,
         .userFrequency = 0,
         .combinedScore = 0,
         .source = "base"},
        {.text = "有用户词频",
         .pinyin = "test",
         .baseFrequency = 1000,
         .userFrequency = 0,
         .combinedScore = 0,
         .source = "base"},
    };

    // 为其中一个词记录用户词频
    for (int i = 0; i < 20; ++i) {
        manager_->RecordWordSelection("有用户词频", "test");
    }

    manager_->SortCandidates(candidates, "test");

    // 有用户词频的词应该排在前面
    EXPECT_EQ(candidates[0].text, "有用户词频");
    EXPECT_EQ(candidates[0].userFrequency, 20);
}

}  // namespace test
}  // namespace frequency
}  // namespace ime
