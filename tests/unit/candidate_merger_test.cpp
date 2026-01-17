// tests/unit/candidate_merger_test.cpp
// 候选词合并器单元测试

#include <gtest/gtest.h>

#include "core/input/candidate_merger.h"

using namespace ime::input;

class CandidateMergerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 使用默认配置
        config_ = MergeConfig::Default();
    }

    MergeConfig config_;
};

// 测试：空输入
TEST_F(CandidateMergerTest, EmptyInputs) {
    std::vector<CandidateWord> userWords;
    std::vector<CandidateWord> rimeCandidates;

    auto result = CandidateMerger::MergeStatic(userWords, rimeCandidates, config_);

    EXPECT_TRUE(result.empty());
}

// 测试：只有 librime 候选词
TEST_F(CandidateMergerTest, OnlyRimeCandidates) {
    std::vector<CandidateWord> userWords;
    std::vector<CandidateWord> rimeCandidates = {
        {"你", "ni", 1000},
        {"尼", "ni", 800},
        {"泥", "ni", 600}
    };

    auto result = CandidateMerger::MergeStatic(userWords, rimeCandidates, config_);

    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0].text, "你");
    EXPECT_EQ(result[1].text, "尼");
    EXPECT_EQ(result[2].text, "泥");
}

// 测试：只有用户高频词
TEST_F(CandidateMergerTest, OnlyUserWords) {
    std::vector<CandidateWord> userWords = {
        {"你好", "nihao", 100},
        {"你们", "nimen", 50}
    };
    userWords[0].isUserWord = true;
    userWords[1].isUserWord = true;

    std::vector<CandidateWord> rimeCandidates;

    auto result = CandidateMerger::MergeStatic(userWords, rimeCandidates, config_);

    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].text, "你好");
    EXPECT_EQ(result[1].text, "你们");
    EXPECT_TRUE(result[0].isUserWord);
    EXPECT_TRUE(result[1].isUserWord);
}

// 测试：用户高频词优先
TEST_F(CandidateMergerTest, UserWordsFirst) {
    std::vector<CandidateWord> userWords = {
        {"妮", "ni", 100},  // 用户高频词
        {"霓", "ni", 50}
    };

    std::vector<CandidateWord> rimeCandidates = {
        {"你", "ni", 1000},
        {"尼", "ni", 800},
        {"泥", "ni", 600},
        {"妮", "ni", 400}  // 与用户词重复
    };

    config_.userWordsFirst = true;
    auto result = CandidateMerger::MergeStatic(userWords, rimeCandidates, config_);

    // 用户词应该在前面
    ASSERT_GE(result.size(), 2);
    EXPECT_EQ(result[0].text, "妮");  // 用户高频词
    EXPECT_EQ(result[1].text, "霓");  // 用户高频词
    EXPECT_TRUE(result[0].isUserWord);
    EXPECT_TRUE(result[1].isUserWord);

    // 后面是 librime 候选词（去重后）
    EXPECT_EQ(result[2].text, "你");
    EXPECT_EQ(result[3].text, "尼");
    EXPECT_EQ(result[4].text, "泥");
    // "妮" 不应该重复出现
    for (size_t i = 2; i < result.size(); ++i) {
        EXPECT_NE(result[i].text, "妮");
    }
}

// 测试：去重功能
TEST_F(CandidateMergerTest, Deduplication) {
    std::vector<CandidateWord> userWords = {
        {"你", "ni", 100}  // 与 librime 候选词重复
    };

    std::vector<CandidateWord> rimeCandidates = {
        {"你", "ni", 1000},
        {"尼", "ni", 800}
    };

    auto result = CandidateMerger::MergeStatic(userWords, rimeCandidates, config_);

    // "你" 只应该出现一次
    int count = 0;
    for (const auto& c : result) {
        if (c.text == "你") {
            count++;
        }
    }
    EXPECT_EQ(count, 1);
}

// 测试：词频阈值过滤
TEST_F(CandidateMergerTest, FrequencyThreshold) {
    std::vector<CandidateWord> userWords = {
        {"高频", "gaopin", 100},  // 高于阈值
        {"低频", "dipin", 1}      // 低于阈值（默认阈值为 3）
    };

    std::vector<CandidateWord> rimeCandidates;

    config_.minUserFrequency = 3;
    auto result = CandidateMerger::MergeStatic(userWords, rimeCandidates, config_);

    // 只有高频词应该被包含
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].text, "高频");
}

// 测试：用户词数量限制
TEST_F(CandidateMergerTest, MaxUserWordsLimit) {
    std::vector<CandidateWord> userWords;
    for (int i = 0; i < 10; ++i) {
        userWords.push_back({"词" + std::to_string(i), "ci", 100 - i});
    }

    std::vector<CandidateWord> rimeCandidates;

    config_.maxUserWords = 5;
    auto result = CandidateMerger::MergeStatic(userWords, rimeCandidates, config_);

    // 最多只有 5 个用户词
    EXPECT_LE(result.size(), 5);
}

// 测试：页面大小限制
TEST_F(CandidateMergerTest, PageSizeLimit) {
    std::vector<CandidateWord> userWords = {
        {"用户词1", "yonghuci", 100},
        {"用户词2", "yonghuci", 90}
    };

    std::vector<CandidateWord> rimeCandidates;
    for (int i = 0; i < 20; ++i) {
        rimeCandidates.push_back({"候选" + std::to_string(i), "houxuan", 1000 - i});
    }

    config_.pageSize = 9;
    auto result = CandidateMerger::MergeStatic(userWords, rimeCandidates, config_);

    // 结果不应超过页面大小
    EXPECT_LE(static_cast<int>(result.size()), config_.pageSize);
}

// 测试：索引更新
TEST_F(CandidateMergerTest, IndexUpdate) {
    std::vector<CandidateWord> candidates = {
        {"一", "yi", 100},
        {"二", "er", 90},
        {"三", "san", 80}
    };

    CandidateUtils::UpdateIndices(candidates);

    EXPECT_EQ(candidates[0].index, 1);
    EXPECT_EQ(candidates[1].index, 2);
    EXPECT_EQ(candidates[2].index, 3);
}

// 测试：分页功能
TEST_F(CandidateMergerTest, Pagination) {
    std::vector<CandidateWord> candidates;
    for (int i = 0; i < 25; ++i) {
        candidates.push_back({"词" + std::to_string(i), "ci", 100 - i});
    }

    // 第一页
    auto page0 = CandidateUtils::GetPage(candidates, 0, 9);
    EXPECT_EQ(page0.size(), 9);
    EXPECT_EQ(page0[0].text, "词0");
    EXPECT_EQ(page0[0].index, 1);

    // 第二页
    auto page1 = CandidateUtils::GetPage(candidates, 1, 9);
    EXPECT_EQ(page1.size(), 9);
    EXPECT_EQ(page1[0].text, "词9");
    EXPECT_EQ(page1[0].index, 1);

    // 第三页（不满）
    auto page2 = CandidateUtils::GetPage(candidates, 2, 9);
    EXPECT_EQ(page2.size(), 7);
    EXPECT_EQ(page2[0].text, "词18");

    // 超出范围的页
    auto page3 = CandidateUtils::GetPage(candidates, 3, 9);
    EXPECT_TRUE(page3.empty());
}

// 测试：总页数计算
TEST_F(CandidateMergerTest, TotalPages) {
    EXPECT_EQ(CandidateUtils::GetTotalPages(0, 9), 0);
    EXPECT_EQ(CandidateUtils::GetTotalPages(1, 9), 1);
    EXPECT_EQ(CandidateUtils::GetTotalPages(9, 9), 1);
    EXPECT_EQ(CandidateUtils::GetTotalPages(10, 9), 2);
    EXPECT_EQ(CandidateUtils::GetTotalPages(18, 9), 2);
    EXPECT_EQ(CandidateUtils::GetTotalPages(19, 9), 3);
    EXPECT_EQ(CandidateUtils::GetTotalPages(25, 9), 3);
}

// 测试：去重工具函数
TEST_F(CandidateMergerTest, RemoveDuplicatesUtil) {
    std::vector<CandidateWord> candidates = {
        {"你", "ni", 100},
        {"好", "hao", 90},
        {"你", "ni", 80},  // 重复
        {"世界", "shijie", 70},
        {"好", "hao", 60}  // 重复
    };

    auto result = CandidateUtils::RemoveDuplicates(candidates);

    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0].text, "你");
    EXPECT_EQ(result[1].text, "好");
    EXPECT_EQ(result[2].text, "世界");
}
