// tests/unit/input_state_test.cpp
// 输入状态管理单元测试

#include <gtest/gtest.h>

#include "core/input/input_state.h"

using namespace ime::input;

class InputStateTest : public ::testing::Test {
protected:
    void SetUp() override {
        stateManager_ = std::make_unique<InputStateManager>();
        changeCount_ = 0;
        lastChangeType_ = StateChangeType::None;

        // 添加监听器
        stateManager_->AddListener([this](const StateChangeEvent& event) {
            changeCount_++;
            lastChangeType_ = event.type;
            lastEvent_ = event;
        });
    }

    std::unique_ptr<InputStateManager> stateManager_;
    int changeCount_;
    StateChangeType lastChangeType_;
    StateChangeEvent lastEvent_;
};

// 测试：初始状态
TEST_F(InputStateTest, InitialState) {
    EXPECT_TRUE(stateManager_->GetPreedit().empty());
    EXPECT_TRUE(stateManager_->GetCandidates().empty());
    EXPECT_EQ(stateManager_->GetCurrentPage(), 0);
    EXPECT_EQ(stateManager_->GetTotalPages(), 0);
    EXPECT_EQ(stateManager_->GetMode(), InputMode::Chinese);
    EXPECT_FALSE(stateManager_->IsComposing());
    EXPECT_FALSE(stateManager_->HasCandidates());
}

// 测试：设置预编辑文本
TEST_F(InputStateTest, SetPreedit) {
    stateManager_->SetPreedit("ni");

    EXPECT_EQ(stateManager_->GetPreedit(), "ni");
    EXPECT_TRUE(stateManager_->IsComposing());
    EXPECT_EQ(changeCount_, 1);
    EXPECT_EQ(lastChangeType_, StateChangeType::PreeditChanged);
}

// 测试：追加字符
TEST_F(InputStateTest, AppendToPreedit) {
    stateManager_->AppendToPreedit('n');
    stateManager_->AppendToPreedit('i');

    EXPECT_EQ(stateManager_->GetPreedit(), "ni");
    EXPECT_EQ(changeCount_, 2);
}

// 测试：删除字符
TEST_F(InputStateTest, PopFromPreedit) {
    stateManager_->SetPreedit("ni");
    changeCount_ = 0;

    bool hasMore = stateManager_->PopFromPreedit();

    EXPECT_TRUE(hasMore);
    EXPECT_EQ(stateManager_->GetPreedit(), "n");
    EXPECT_EQ(changeCount_, 1);

    hasMore = stateManager_->PopFromPreedit();

    EXPECT_FALSE(hasMore);
    EXPECT_TRUE(stateManager_->GetPreedit().empty());
}

// 测试：空字符串删除
TEST_F(InputStateTest, PopFromEmptyPreedit) {
    bool hasMore = stateManager_->PopFromPreedit();

    EXPECT_FALSE(hasMore);
    EXPECT_EQ(changeCount_, 0);  // 不应触发事件
}

// 测试：设置候选词
TEST_F(InputStateTest, SetCandidates) {
    std::vector<CandidateWord> candidates = {
        {"你", "ni", 100},
        {"尼", "ni", 80}
    };

    stateManager_->SetCandidates(candidates);

    EXPECT_EQ(stateManager_->GetCandidates().size(), 2);
    EXPECT_TRUE(stateManager_->HasCandidates());
    EXPECT_EQ(lastChangeType_, StateChangeType::CandidatesChanged);
}

// 测试：分页
TEST_F(InputStateTest, Pagination) {
    std::vector<CandidateWord> candidates;
    for (int i = 0; i < 25; ++i) {
        candidates.push_back({"词" + std::to_string(i), "ci", 100 - i});
    }

    stateManager_->SetAllCandidates(candidates, 9);

    EXPECT_EQ(stateManager_->GetTotalPages(), 3);
    EXPECT_EQ(stateManager_->GetCurrentPage(), 0);
    EXPECT_EQ(stateManager_->GetCandidates().size(), 9);
}

// 测试：翻页
TEST_F(InputStateTest, PageNavigation) {
    std::vector<CandidateWord> candidates;
    for (int i = 0; i < 25; ++i) {
        candidates.push_back({"词" + std::to_string(i), "ci", 100 - i});
    }

    stateManager_->SetAllCandidates(candidates, 9);
    changeCount_ = 0;

    // 下一页
    bool success = stateManager_->NextPage();
    EXPECT_TRUE(success);
    EXPECT_EQ(stateManager_->GetCurrentPage(), 1);
    EXPECT_EQ(lastChangeType_, StateChangeType::PageChanged);

    // 再下一页
    success = stateManager_->NextPage();
    EXPECT_TRUE(success);
    EXPECT_EQ(stateManager_->GetCurrentPage(), 2);

    // 已经是最后一页，不能再翻
    success = stateManager_->NextPage();
    EXPECT_FALSE(success);
    EXPECT_EQ(stateManager_->GetCurrentPage(), 2);

    // 上一页
    success = stateManager_->PreviousPage();
    EXPECT_TRUE(success);
    EXPECT_EQ(stateManager_->GetCurrentPage(), 1);

    // 回到第一页
    success = stateManager_->PreviousPage();
    EXPECT_TRUE(success);
    EXPECT_EQ(stateManager_->GetCurrentPage(), 0);

    // 已经是第一页，不能再翻
    success = stateManager_->PreviousPage();
    EXPECT_FALSE(success);
    EXPECT_EQ(stateManager_->GetCurrentPage(), 0);
}

// 测试：模式切换
TEST_F(InputStateTest, ModeToggle) {
    EXPECT_EQ(stateManager_->GetMode(), InputMode::Chinese);

    stateManager_->ToggleMode();
    EXPECT_EQ(stateManager_->GetMode(), InputMode::English);
    EXPECT_EQ(lastChangeType_, StateChangeType::ModeChanged);

    stateManager_->ToggleMode();
    EXPECT_EQ(stateManager_->GetMode(), InputMode::Chinese);
}

// 测试：设置模式
TEST_F(InputStateTest, SetMode) {
    stateManager_->SetMode(InputMode::English);
    EXPECT_EQ(stateManager_->GetMode(), InputMode::English);

    // 设置相同模式不应触发事件
    changeCount_ = 0;
    stateManager_->SetMode(InputMode::English);
    EXPECT_EQ(changeCount_, 0);
}

// 测试：重置
TEST_F(InputStateTest, Reset) {
    stateManager_->SetPreedit("ni");
    stateManager_->SetCandidates({{"你", "ni", 100}});
    changeCount_ = 0;

    stateManager_->Reset();

    EXPECT_TRUE(stateManager_->GetPreedit().empty());
    EXPECT_TRUE(stateManager_->GetCandidates().empty());
    EXPECT_FALSE(stateManager_->IsComposing());
    EXPECT_EQ(lastChangeType_, StateChangeType::Reset);
}

// 测试：提交并重置
TEST_F(InputStateTest, CommitAndReset) {
    stateManager_->SetPreedit("ni");
    stateManager_->SetCandidates({{"你", "ni", 100}});
    changeCount_ = 0;

    stateManager_->CommitAndReset("你");

    EXPECT_TRUE(stateManager_->GetPreedit().empty());
    EXPECT_TRUE(stateManager_->GetCandidates().empty());
    EXPECT_EQ(lastChangeType_, StateChangeType::Committed);
    EXPECT_EQ(lastEvent_.committedText, "你");
}

// 测试：取消并重置
TEST_F(InputStateTest, CancelAndReset) {
    stateManager_->SetPreedit("ni");
    stateManager_->SetCandidates({{"你", "ni", 100}});
    changeCount_ = 0;

    stateManager_->CancelAndReset();

    EXPECT_TRUE(stateManager_->GetPreedit().empty());
    EXPECT_TRUE(stateManager_->GetCandidates().empty());
    EXPECT_EQ(lastChangeType_, StateChangeType::Cancelled);
}

// 测试：临时英文模式提交后恢复
TEST_F(InputStateTest, TempEnglishModeResetOnCommit) {
    stateManager_->SetMode(InputMode::TempEnglish);
    stateManager_->SetPreedit("Hello");

    stateManager_->CommitAndReset("Hello");

    EXPECT_EQ(stateManager_->GetMode(), InputMode::Chinese);
}

// 测试：临时英文模式取消后恢复
TEST_F(InputStateTest, TempEnglishModeResetOnCancel) {
    stateManager_->SetMode(InputMode::TempEnglish);
    stateManager_->SetPreedit("Hello");

    stateManager_->CancelAndReset();

    EXPECT_EQ(stateManager_->GetMode(), InputMode::Chinese);
}

// 测试：清除监听器
TEST_F(InputStateTest, ClearListeners) {
    stateManager_->ClearListeners();
    changeCount_ = 0;

    stateManager_->SetPreedit("ni");

    EXPECT_EQ(changeCount_, 0);  // 监听器已清除，不应收到通知
}

// 测试：多个监听器
TEST_F(InputStateTest, MultipleListeners) {
    int secondListenerCount = 0;
    stateManager_->AddListener([&secondListenerCount](const StateChangeEvent&) {
        secondListenerCount++;
    });

    stateManager_->SetPreedit("ni");

    EXPECT_EQ(changeCount_, 1);
    EXPECT_EQ(secondListenerCount, 1);
}

// 测试：设置页面大小
TEST_F(InputStateTest, SetPageSize) {
    stateManager_->SetPageSize(5);
    EXPECT_EQ(stateManager_->GetPageSize(), 5);

    std::vector<CandidateWord> candidates;
    for (int i = 0; i < 12; ++i) {
        candidates.push_back({"词" + std::to_string(i), "ci", 100 - i});
    }

    stateManager_->SetAllCandidates(candidates, 5);

    EXPECT_EQ(stateManager_->GetTotalPages(), 3);  // 12 / 5 = 3 pages
    EXPECT_EQ(stateManager_->GetCandidates().size(), 5);
}
