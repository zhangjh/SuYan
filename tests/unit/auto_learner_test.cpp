// tests/unit/auto_learner_test.cpp
// 自动学词模块单元测试

#include <gtest/gtest.h>

#include <memory>
#include <thread>
#include <chrono>

#include "core/learning/auto_learner.h"
#include "core/learning/auto_learner_impl.h"
#include "core/storage/sqlite_storage.h"

using namespace ime::learning;
using namespace ime::storage;

class AutoLearnerTest : public ::testing::Test {
protected:
    void SetUp() override {
        storage_ = std::make_unique<SqliteStorage>(":memory:");
        ASSERT_TRUE(storage_->Initialize());

        learner_ = std::make_unique<AutoLearnerImpl>(storage_.get());
        ASSERT_TRUE(learner_->Initialize());
    }

    void TearDown() override {
        learner_->Shutdown();
        storage_->Close();
    }

    // 模拟连续输入单字
    void SimulateConsecutiveInput(const std::vector<std::pair<std::string, std::string>>& inputs) {
        for (const auto& [text, pinyin] : inputs) {
            learner_->RecordInput(text, pinyin);
            // 短暂延迟以模拟真实输入
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    std::unique_ptr<SqliteStorage> storage_;
    std::unique_ptr<AutoLearnerImpl> learner_;
};

// 测试：初始化
TEST_F(AutoLearnerTest, Initialize) {
    EXPECT_TRUE(learner_->IsInitialized());
    EXPECT_TRUE(learner_->IsEnabled());
}

// 测试：记录单次输入
TEST_F(AutoLearnerTest, RecordSingleInput) {
    auto detected = learner_->RecordInput("你", "ni");
    
    // 单次输入不应检测到词组
    EXPECT_TRUE(detected.empty());
    
    auto history = learner_->GetHistory();
    ASSERT_EQ(history.size(), 1);
    EXPECT_EQ(history[0].text, "你");
    EXPECT_EQ(history[0].pinyin, "ni");
}

// 测试：连续输入检测词组
TEST_F(AutoLearnerTest, DetectPhraseFromConsecutiveInput) {
    // 设置较低的阈值以便测试
    auto config = learner_->GetConfig();
    config.minOccurrences = 1;  // 出现 1 次就检测
    learner_->SetConfig(config);

    // 输入 "你好"
    learner_->RecordInput("你", "ni");
    auto detected = learner_->RecordInput("好", "hao");

    // 应该检测到 "你好" 词组
    ASSERT_FALSE(detected.empty());
    
    bool found = false;
    for (const auto& candidate : detected) {
        if (candidate.text == "你好") {
            found = true;
            EXPECT_EQ(candidate.pinyin, "ni hao");
            break;
        }
    }
    EXPECT_TRUE(found);
}

// 测试：词频阈值控制
TEST_F(AutoLearnerTest, FrequencyThreshold) {
    // 使用默认配置（minOccurrences = 2）
    auto config = learner_->GetConfig();
    EXPECT_EQ(config.minOccurrences, 2);

    // 第一次输入 "你好"
    learner_->RecordInput("你", "ni");
    auto detected1 = learner_->RecordInput("好", "hao");
    
    // 第一次不应达到阈值
    bool foundFirst = false;
    for (const auto& c : detected1) {
        if (c.text == "你好" && c.occurrences >= 2) {
            foundFirst = true;
        }
    }
    EXPECT_FALSE(foundFirst);

    // 清空历史，再次输入
    learner_->ClearHistory();
    
    // 第二次输入 "你好"
    learner_->RecordInput("你", "ni");
    auto detected2 = learner_->RecordInput("好", "hao");

    // 第二次应该达到阈值
    bool foundSecond = false;
    for (const auto& c : detected2) {
        if (c.text == "你好" && c.occurrences >= 2) {
            foundSecond = true;
        }
    }
    EXPECT_TRUE(foundSecond);
}

// 测试：确认学习
TEST_F(AutoLearnerTest, ConfirmLearn) {
    auto config = learner_->GetConfig();
    config.minOccurrences = 1;
    learner_->SetConfig(config);

    // 输入并检测
    learner_->RecordInput("你", "ni");
    learner_->RecordInput("好", "hao");

    // 确认学习
    EXPECT_TRUE(learner_->ConfirmLearn("你好", "ni hao"));

    // 验证已添加到用户词库
    int freq = storage_->GetWordFrequency("你好", "ni hao");
    EXPECT_GT(freq, 0);
}

// 测试：拒绝学习
TEST_F(AutoLearnerTest, RejectLearn) {
    auto config = learner_->GetConfig();
    config.minOccurrences = 1;
    learner_->SetConfig(config);

    // 输入并检测
    learner_->RecordInput("你", "ni");
    learner_->RecordInput("好", "hao");

    // 拒绝学习
    learner_->RejectLearn("你好", "ni hao");

    // 再次输入相同内容
    learner_->ClearHistory();
    learner_->RecordInput("你", "ni");
    auto detected = learner_->RecordInput("好", "hao");

    // 不应再检测到被拒绝的词组
    bool found = false;
    for (const auto& c : detected) {
        if (c.text == "你好") {
            found = true;
        }
    }
    EXPECT_FALSE(found);
}

// 测试：禁用自动学词
TEST_F(AutoLearnerTest, DisableAutoLearn) {
    learner_->SetEnabled(false);
    EXPECT_FALSE(learner_->IsEnabled());

    // 输入不应被记录
    auto detected = learner_->RecordInput("你", "ni");
    EXPECT_TRUE(detected.empty());

    auto history = learner_->GetHistory();
    EXPECT_TRUE(history.empty());
}

// 测试：清空历史
TEST_F(AutoLearnerTest, ClearHistory) {
    learner_->RecordInput("你", "ni");
    learner_->RecordInput("好", "hao");

    EXPECT_EQ(learner_->GetHistory().size(), 2);

    learner_->ClearHistory();

    EXPECT_TRUE(learner_->GetHistory().empty());
}

// 测试：历史记录大小限制
TEST_F(AutoLearnerTest, HistorySizeLimit) {
    auto config = learner_->GetConfig();
    config.historySize = 5;
    learner_->SetConfig(config);

    // 输入超过限制的数量
    for (int i = 0; i < 10; ++i) {
        learner_->RecordInput("字" + std::to_string(i), "zi");
    }

    auto history = learner_->GetHistory();
    EXPECT_LE(history.size(), 5);
}

// 测试：自动处理候选
TEST_F(AutoLearnerTest, ProcessCandidates) {
    auto config = learner_->GetConfig();
    config.minOccurrences = 2;
    learner_->SetConfig(config);

    // 输入两次 "你好"
    learner_->RecordInput("你", "ni");
    learner_->RecordInput("好", "hao");
    learner_->ClearHistory();
    learner_->RecordInput("你", "ni");
    learner_->RecordInput("好", "hao");

    // 处理候选
    auto learned = learner_->ProcessCandidates();

    // 应该自动学习了 "你好"
    bool found = false;
    for (const auto& c : learned) {
        if (c.text == "你好") {
            found = true;
        }
    }
    EXPECT_TRUE(found);

    // 验证已添加到用户词库
    int freq = storage_->GetWordFrequency("你好", "ni hao");
    EXPECT_GT(freq, 0);
}

// 测试：已存在的词不重复学习
TEST_F(AutoLearnerTest, NoRelearningExistingWords) {
    auto config = learner_->GetConfig();
    config.minOccurrences = 1;
    learner_->SetConfig(config);

    // 先手动添加词到用户词库
    storage_->IncrementWordFrequency("你好", "ni hao");

    // 输入 "你好"
    learner_->RecordInput("你", "ni");
    auto detected = learner_->RecordInput("好", "hao");

    // 不应检测到已存在的词
    bool found = false;
    for (const auto& c : detected) {
        if (c.text == "你好") {
            found = true;
        }
    }
    EXPECT_FALSE(found);
}

// 测试：词长限制
TEST_F(AutoLearnerTest, WordLengthLimit) {
    auto config = learner_->GetConfig();
    config.minOccurrences = 1;
    config.minWordLength = 2;
    config.maxWordLength = 3;
    learner_->SetConfig(config);

    // 输入 4 个字
    learner_->RecordInput("中", "zhong");
    learner_->RecordInput("华", "hua");
    learner_->RecordInput("人", "ren");
    auto detected = learner_->RecordInput("民", "min");

    // 应该检测到 2-3 字的词组，但不应有 4 字的
    bool found4Chars = false;
    for (const auto& c : detected) {
        size_t charCount = 0;
        for (size_t i = 0; i < c.text.length();) {
            unsigned char ch = c.text[i];
            if ((ch & 0xF0) == 0xE0) {
                i += 3;  // 中文字符
            } else {
                i += 1;
            }
            charCount++;
        }
        if (charCount > 3) {
            found4Chars = true;
        }
    }
    EXPECT_FALSE(found4Chars);
}

// 测试：配置持久化
TEST_F(AutoLearnerTest, ConfigPersistence) {
    auto config = learner_->GetConfig();
    config.minOccurrences = 5;
    config.maxInputInterval = 5000;
    config.enabled = false;
    learner_->SetConfig(config);

    // 创建新的学习器实例
    auto newLearner = std::make_unique<AutoLearnerImpl>(storage_.get());
    ASSERT_TRUE(newLearner->Initialize());

    auto loadedConfig = newLearner->GetConfig();
    EXPECT_EQ(loadedConfig.minOccurrences, 5);
    EXPECT_EQ(loadedConfig.maxInputInterval, 5000);
    EXPECT_FALSE(loadedConfig.enabled);
}
