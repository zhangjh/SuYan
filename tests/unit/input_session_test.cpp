// tests/unit/input_session_test.cpp
// 输入会话单元测试

#include <gtest/gtest.h>

#include "core/input/input_session.h"
#include "core/frequency/frequency_manager_impl.h"
#include "core/storage/sqlite_storage.h"

using namespace ime::input;
using namespace ime::storage;
// 注意：不使用 using namespace ime::frequency 以避免 CandidateWord 歧义

class InputSessionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 使用内存数据库进行测试
        storage_ = std::make_unique<SqliteStorage>(":memory:");
        ASSERT_TRUE(storage_->Initialize());

        frequencyManager_ = std::make_unique<ime::frequency::FrequencyManagerImpl>(storage_.get());
        ASSERT_TRUE(frequencyManager_->Initialize());

        session_ = std::make_unique<InputSession>(storage_.get(), 
                                                   frequencyManager_.get());

        // 设置模拟的候选词查询回调
        session_->SetCandidateQueryCallback([](const std::string& pinyin) {
            std::vector<CandidateWord> candidates;
            
            // 模拟一些候选词
            if (pinyin == "ni") {
                candidates.push_back({"你", "ni", 1000});
                candidates.push_back({"尼", "ni", 800});
                candidates.push_back({"泥", "ni", 600});
                candidates.push_back({"逆", "ni", 400});
                candidates.push_back({"腻", "ni", 200});
            } else if (pinyin == "hao") {
                candidates.push_back({"好", "hao", 1000});
                candidates.push_back({"号", "hao", 800});
                candidates.push_back({"豪", "hao", 600});
            } else if (pinyin == "nihao") {
                candidates.push_back({"你好", "nihao", 2000});
                candidates.push_back({"泥号", "nihao", 100});
            }
            
            return candidates;
        });
    }

    void TearDown() override {
        session_.reset();
        frequencyManager_->Shutdown();
        frequencyManager_.reset();
        storage_->Close();
        storage_.reset();
    }

    std::unique_ptr<SqliteStorage> storage_;
    std::unique_ptr<ime::frequency::FrequencyManagerImpl> frequencyManager_;
    std::unique_ptr<InputSession> session_;
};

// 测试：初始状态
TEST_F(InputSessionTest, InitialState) {
    EXPECT_FALSE(session_->IsComposing());
    EXPECT_TRUE(session_->GetPreedit().empty());
    EXPECT_TRUE(session_->GetCandidates().empty());
    EXPECT_EQ(session_->GetInputMode(), InputMode::Chinese);
}

// 测试：字母输入
TEST_F(InputSessionTest, LetterInput) {
    auto result = session_->ProcessChar('n');
    
    EXPECT_TRUE(result.consumed);
    EXPECT_TRUE(result.needsUpdate);
    EXPECT_TRUE(session_->IsComposing());
    EXPECT_EQ(session_->GetPreedit(), "n");
}

// 测试：拼音输入产生候选词
TEST_F(InputSessionTest, PinyinProducesCandidates) {
    session_->ProcessChar('n');
    session_->ProcessChar('i');
    
    EXPECT_EQ(session_->GetPreedit(), "ni");
    EXPECT_FALSE(session_->GetCandidates().empty());
    
    // 检查候选词
    const auto& candidates = session_->GetCandidates();
    EXPECT_EQ(candidates[0].text, "你");
}

// 测试：数字键选择候选词
TEST_F(InputSessionTest, DigitSelectsCandidate) {
    session_->ProcessChar('n');
    session_->ProcessChar('i');
    
    // 选择第一个候选词
    auto result = session_->ProcessKey(KeyEvent::FromChar('1'));
    
    EXPECT_TRUE(result.consumed);
    EXPECT_TRUE(result.needsCommit);
    EXPECT_EQ(result.commitText, "你");
    EXPECT_FALSE(session_->IsComposing());
}

// 测试：空格键选择首选
TEST_F(InputSessionTest, SpaceSelectsFirst) {
    session_->ProcessChar('n');
    session_->ProcessChar('i');
    
    auto result = session_->ProcessKey(KeyEvent::FromSpecial(KeyType::Space));
    
    EXPECT_TRUE(result.consumed);
    EXPECT_TRUE(result.needsCommit);
    EXPECT_EQ(result.commitText, "你");
}

// 测试：回车键提交原文
TEST_F(InputSessionTest, EnterCommitsRaw) {
    session_->ProcessChar('n');
    session_->ProcessChar('i');
    
    auto result = session_->ProcessKey(KeyEvent::FromSpecial(KeyType::Enter));
    
    EXPECT_TRUE(result.consumed);
    EXPECT_TRUE(result.needsCommit);
    EXPECT_EQ(result.commitText, "ni");
}

// 测试：Esc 键取消输入
TEST_F(InputSessionTest, EscapeCancels) {
    session_->ProcessChar('n');
    session_->ProcessChar('i');
    
    EXPECT_TRUE(session_->IsComposing());
    
    auto result = session_->ProcessKey(KeyEvent::FromSpecial(KeyType::Escape));
    
    EXPECT_TRUE(result.consumed);
    EXPECT_TRUE(result.needsHide);
    EXPECT_FALSE(session_->IsComposing());
    EXPECT_TRUE(session_->GetPreedit().empty());
}

// 测试：退格键删除
TEST_F(InputSessionTest, BackspaceDeletes) {
    session_->ProcessChar('n');
    session_->ProcessChar('i');
    
    EXPECT_EQ(session_->GetPreedit(), "ni");
    
    auto result = session_->ProcessKey(KeyEvent::FromSpecial(KeyType::Backspace));
    
    EXPECT_TRUE(result.consumed);
    EXPECT_TRUE(result.needsUpdate);
    EXPECT_EQ(session_->GetPreedit(), "n");
}

// 测试：退格到空时取消
TEST_F(InputSessionTest, BackspaceToEmptyCancels) {
    session_->ProcessChar('n');
    
    auto result = session_->ProcessKey(KeyEvent::FromSpecial(KeyType::Backspace));
    
    EXPECT_TRUE(result.consumed);
    EXPECT_TRUE(result.needsHide);
    EXPECT_FALSE(session_->IsComposing());
}

// 测试：选择候选词后更新词频
TEST_F(InputSessionTest, SelectionUpdatesFrequency) {
    // 输入并选择
    session_->ProcessChar('n');
    session_->ProcessChar('i');
    session_->ProcessKey(KeyEvent::FromChar('2'));  // 选择 "尼"
    
    // 检查词频是否更新
    int freq = frequencyManager_->GetUserFrequency("尼", "ni");
    EXPECT_GT(freq, 0);
}

// 测试：中英文模式切换
TEST_F(InputSessionTest, ModeToggle) {
    EXPECT_EQ(session_->GetInputMode(), InputMode::Chinese);
    
    session_->ToggleInputMode();
    EXPECT_EQ(session_->GetInputMode(), InputMode::English);
    
    session_->ToggleInputMode();
    EXPECT_EQ(session_->GetInputMode(), InputMode::Chinese);
}

// 测试：英文模式下按键透传
TEST_F(InputSessionTest, EnglishModePassThrough) {
    session_->SetInputMode(InputMode::English);
    
    auto result = session_->ProcessChar('a');
    
    EXPECT_FALSE(result.consumed);
    EXPECT_FALSE(session_->IsComposing());
}

// 测试：Shift 键切换模式
TEST_F(InputSessionTest, ShiftToggleMode) {
    EXPECT_EQ(session_->GetInputMode(), InputMode::Chinese);
    
    auto result = session_->ProcessKey(KeyEvent::FromSpecial(KeyType::Shift));
    
    EXPECT_TRUE(result.consumed);
    EXPECT_EQ(session_->GetInputMode(), InputMode::English);
}

// 测试：非输入状态下数字键透传
TEST_F(InputSessionTest, DigitPassThroughWhenNotComposing) {
    auto result = session_->ProcessKey(KeyEvent::FromChar('1'));
    
    EXPECT_FALSE(result.consumed);
}

// 测试：非输入状态下空格键透传
TEST_F(InputSessionTest, SpacePassThroughWhenNotComposing) {
    auto result = session_->ProcessKey(KeyEvent::FromSpecial(KeyType::Space));
    
    EXPECT_FALSE(result.consumed);
}

// 测试：选择超出范围的候选词
TEST_F(InputSessionTest, SelectOutOfRangeCandidate) {
    session_->ProcessChar('n');
    session_->ProcessChar('i');
    
    // 尝试选择不存在的候选词
    auto result = session_->SelectCandidate(9);
    
    EXPECT_FALSE(result.consumed);
    EXPECT_TRUE(session_->IsComposing());  // 仍在输入状态
}

// 测试：大写字母触发临时英文模式
TEST_F(InputSessionTest, UppercaseTriggersTempEnglish) {
    auto result = session_->ProcessChar('A');
    
    // 大写字母应该触发临时英文模式并透传
    EXPECT_FALSE(result.consumed);
    EXPECT_EQ(session_->GetInputMode(), InputMode::TempEnglish);
}

// 测试：模式状态持久化 - 保存
TEST_F(InputSessionTest, ModePersistenceSave) {
    // 切换到英文模式
    session_->SetInputMode(InputMode::English);
    
    // 检查存储中的值
    std::string savedMode = storage_->GetConfig("input.default_mode", "chinese");
    EXPECT_EQ(savedMode, "english");
}

// 测试：模式状态持久化 - 加载
TEST_F(InputSessionTest, ModePersistenceLoad) {
    // 先设置存储中的值
    storage_->SetConfig("input.default_mode", "english");
    
    // 创建新的 session 并加载模式
    auto newSession = std::make_unique<InputSession>(storage_.get(), 
                                                      frequencyManager_.get());
    newSession->LoadInputModeFromStorage();
    
    EXPECT_EQ(newSession->GetInputMode(), InputMode::English);
}

// 测试：临时英文模式不持久化
TEST_F(InputSessionTest, TempEnglishModeNotPersisted) {
    // 先设置为中文模式
    session_->SetInputMode(InputMode::Chinese);
    
    // 触发临时英文模式
    session_->ProcessChar('A');
    EXPECT_EQ(session_->GetInputMode(), InputMode::TempEnglish);
    
    // 检查存储中的值仍然是 chinese
    std::string savedMode = storage_->GetConfig("input.default_mode", "");
    EXPECT_EQ(savedMode, "chinese");
}

// 测试：Shift 键切换模式并持久化
TEST_F(InputSessionTest, ShiftToggleModeAndPersist) {
    EXPECT_EQ(session_->GetInputMode(), InputMode::Chinese);
    
    // 按 Shift 切换到英文模式
    session_->ProcessKey(KeyEvent::FromSpecial(KeyType::Shift));
    EXPECT_EQ(session_->GetInputMode(), InputMode::English);
    
    // 检查存储
    std::string savedMode = storage_->GetConfig("input.default_mode", "");
    EXPECT_EQ(savedMode, "english");
    
    // 再按 Shift 切换回中文模式
    session_->ProcessKey(KeyEvent::FromSpecial(KeyType::Shift));
    EXPECT_EQ(session_->GetInputMode(), InputMode::Chinese);
    
    // 检查存储
    savedMode = storage_->GetConfig("input.default_mode", "");
    EXPECT_EQ(savedMode, "chinese");
}

// 测试：英文模式下 Shift 键切换回中文
TEST_F(InputSessionTest, ShiftInEnglishModeTogglesToChinese) {
    session_->SetInputMode(InputMode::English);
    
    auto result = session_->ProcessKey(KeyEvent::FromSpecial(KeyType::Shift));
    
    EXPECT_TRUE(result.consumed);
    EXPECT_EQ(session_->GetInputMode(), InputMode::Chinese);
}

// 测试：临时英文模式下继续输入
TEST_F(InputSessionTest, TempEnglishModeContinuesInput) {
    // 触发临时英文模式
    auto result = session_->ProcessChar('H');
    EXPECT_FALSE(result.consumed);
    EXPECT_EQ(session_->GetInputMode(), InputMode::TempEnglish);
    
    // 继续输入小写字母，应该继续透传
    result = session_->ProcessChar('e');
    EXPECT_FALSE(result.consumed);
    
    result = session_->ProcessChar('l');
    EXPECT_FALSE(result.consumed);
    
    result = session_->ProcessChar('l');
    EXPECT_FALSE(result.consumed);
    
    result = session_->ProcessChar('o');
    EXPECT_FALSE(result.consumed);
}

// 测试：临时英文模式下空格键恢复中文模式
TEST_F(InputSessionTest, TempEnglishModeSpaceRestoresChinese) {
    // 触发临时英文模式
    session_->ProcessChar('H');
    EXPECT_EQ(session_->GetInputMode(), InputMode::TempEnglish);
    
    // 按空格键应该透传并恢复中文模式
    auto result = session_->ProcessKey(KeyEvent::FromSpecial(KeyType::Space));
    EXPECT_FALSE(result.consumed);  // 透传
    EXPECT_EQ(session_->GetInputMode(), InputMode::Chinese);
}

// 测试：临时英文模式下回车键恢复中文模式
TEST_F(InputSessionTest, TempEnglishModeEnterRestoresChinese) {
    // 触发临时英文模式
    session_->ProcessChar('H');
    EXPECT_EQ(session_->GetInputMode(), InputMode::TempEnglish);
    
    // 按回车键应该透传并恢复中文模式
    auto result = session_->ProcessKey(KeyEvent::FromSpecial(KeyType::Enter));
    EXPECT_FALSE(result.consumed);  // 透传
    EXPECT_EQ(session_->GetInputMode(), InputMode::Chinese);
}

// 测试：临时英文模式下 Escape 键恢复中文模式
TEST_F(InputSessionTest, TempEnglishModeEscapeRestoresChinese) {
    // 触发临时英文模式
    session_->ProcessChar('H');
    EXPECT_EQ(session_->GetInputMode(), InputMode::TempEnglish);
    
    // 按 Escape 键应该透传并恢复中文模式
    auto result = session_->ProcessKey(KeyEvent::FromSpecial(KeyType::Escape));
    EXPECT_FALSE(result.consumed);  // 透传
    EXPECT_EQ(session_->GetInputMode(), InputMode::Chinese);
}

// 测试：临时英文模式下 Shift 键恢复中文模式
TEST_F(InputSessionTest, TempEnglishModeShiftRestoresChinese) {
    // 触发临时英文模式
    session_->ProcessChar('H');
    EXPECT_EQ(session_->GetInputMode(), InputMode::TempEnglish);
    
    // 按 Shift 键应该消费并恢复中文模式
    auto result = session_->ProcessKey(KeyEvent::FromSpecial(KeyType::Shift));
    EXPECT_TRUE(result.consumed);  // 消费
    EXPECT_EQ(session_->GetInputMode(), InputMode::Chinese);
}

// 测试：整句输入
TEST_F(InputSessionTest, SentenceInput) {
    session_->ProcessChar('n');
    session_->ProcessChar('i');
    session_->ProcessChar('h');
    session_->ProcessChar('a');
    session_->ProcessChar('o');
    
    EXPECT_EQ(session_->GetPreedit(), "nihao");
    
    const auto& candidates = session_->GetCandidates();
    ASSERT_FALSE(candidates.empty());
    EXPECT_EQ(candidates[0].text, "你好");
}

// ==================== 分页功能测试 ====================

class InputSessionPaginationTest : public ::testing::Test {
protected:
    void SetUp() override {
        storage_ = std::make_unique<SqliteStorage>(":memory:");
        ASSERT_TRUE(storage_->Initialize());

        frequencyManager_ = std::make_unique<ime::frequency::FrequencyManagerImpl>(storage_.get());
        ASSERT_TRUE(frequencyManager_->Initialize());

        session_ = std::make_unique<InputSession>(storage_.get(), 
                                                   frequencyManager_.get());
        session_->SetPageSize(5);  // 设置较小的页面大小便于测试

        // 设置模拟的候选词查询回调，返回大量候选词用于分页测试
        session_->SetCandidateQueryCallback([](const std::string& pinyin) {
            std::vector<CandidateWord> candidates;
            
            if (pinyin == "shi") {
                // 返回 15 个候选词，可以分成 3 页
                candidates.push_back({"是", "shi", 1000});
                candidates.push_back({"时", "shi", 900});
                candidates.push_back({"事", "shi", 800});
                candidates.push_back({"市", "shi", 700});
                candidates.push_back({"式", "shi", 600});
                candidates.push_back({"世", "shi", 500});
                candidates.push_back({"室", "shi", 400});
                candidates.push_back({"师", "shi", 300});
                candidates.push_back({"史", "shi", 200});
                candidates.push_back({"使", "shi", 100});
                candidates.push_back({"始", "shi", 90});
                candidates.push_back({"士", "shi", 80});
                candidates.push_back({"示", "shi", 70});
                candidates.push_back({"视", "shi", 60});
                candidates.push_back({"试", "shi", 50});
            } else if (pinyin == "xyz") {
                // 无效拼音，返回空
            }
            
            return candidates;
        });
    }

    void TearDown() override {
        session_.reset();
        frequencyManager_->Shutdown();
        frequencyManager_.reset();
        storage_->Close();
        storage_.reset();
    }

    std::unique_ptr<SqliteStorage> storage_;
    std::unique_ptr<ime::frequency::FrequencyManagerImpl> frequencyManager_;
    std::unique_ptr<InputSession> session_;
};

// 测试：分页初始状态
TEST_F(InputSessionPaginationTest, InitialPageState) {
    session_->ProcessChar('s');
    session_->ProcessChar('h');
    session_->ProcessChar('i');
    
    const auto& state = session_->GetState();
    EXPECT_EQ(state.currentPage, 0);
    EXPECT_EQ(state.totalPages, 3);  // 15 个候选词，每页 5 个，共 3 页
    EXPECT_EQ(state.candidates.size(), 5);  // 第一页 5 个
    EXPECT_EQ(state.candidates[0].text, "是");
}

// 测试：Page Down 翻页
TEST_F(InputSessionPaginationTest, PageDownNavigation) {
    session_->ProcessChar('s');
    session_->ProcessChar('h');
    session_->ProcessChar('i');
    
    // 翻到第二页
    auto result = session_->ProcessKey(KeyEvent::FromSpecial(KeyType::PageDown));
    EXPECT_TRUE(result.consumed);
    EXPECT_TRUE(result.needsUpdate);
    
    const auto& state = session_->GetState();
    EXPECT_EQ(state.currentPage, 1);
    EXPECT_EQ(state.candidates.size(), 5);
    EXPECT_EQ(state.candidates[0].text, "世");  // 第二页第一个
}

// 测试：Page Up 翻页
TEST_F(InputSessionPaginationTest, PageUpNavigation) {
    session_->ProcessChar('s');
    session_->ProcessChar('h');
    session_->ProcessChar('i');
    
    // 先翻到第二页
    session_->ProcessKey(KeyEvent::FromSpecial(KeyType::PageDown));
    EXPECT_EQ(session_->GetState().currentPage, 1);
    
    // 翻回第一页
    auto result = session_->ProcessKey(KeyEvent::FromSpecial(KeyType::PageUp));
    EXPECT_TRUE(result.consumed);
    EXPECT_TRUE(result.needsUpdate);
    
    const auto& state = session_->GetState();
    EXPECT_EQ(state.currentPage, 0);
    EXPECT_EQ(state.candidates[0].text, "是");
}

// 测试：等号键翻页（下一页）
TEST_F(InputSessionPaginationTest, EqualKeyPageDown) {
    session_->ProcessChar('s');
    session_->ProcessChar('h');
    session_->ProcessChar('i');
    
    auto result = session_->ProcessKey(KeyEvent::FromChar('='));
    EXPECT_TRUE(result.consumed);
    EXPECT_TRUE(result.needsUpdate);
    
    EXPECT_EQ(session_->GetState().currentPage, 1);
}

// 测试：减号键翻页（上一页）
TEST_F(InputSessionPaginationTest, MinusKeyPageUp) {
    session_->ProcessChar('s');
    session_->ProcessChar('h');
    session_->ProcessChar('i');
    
    // 先翻到第二页
    session_->ProcessKey(KeyEvent::FromChar('='));
    EXPECT_EQ(session_->GetState().currentPage, 1);
    
    // 用减号键翻回
    auto result = session_->ProcessKey(KeyEvent::FromChar('-'));
    EXPECT_TRUE(result.consumed);
    EXPECT_TRUE(result.needsUpdate);
    
    EXPECT_EQ(session_->GetState().currentPage, 0);
}

// 测试：翻页边界 - 第一页不能再往前翻
TEST_F(InputSessionPaginationTest, PageUpBoundary) {
    session_->ProcessChar('s');
    session_->ProcessChar('h');
    session_->ProcessChar('i');
    
    // 在第一页尝试往前翻
    auto result = session_->ProcessKey(KeyEvent::FromSpecial(KeyType::PageUp));
    EXPECT_TRUE(result.consumed);  // 按键被消费
    EXPECT_FALSE(result.needsUpdate);  // 但不需要更新（已经是第一页）
    
    EXPECT_EQ(session_->GetState().currentPage, 0);
}

// 测试：翻页边界 - 最后一页不能再往后翻
TEST_F(InputSessionPaginationTest, PageDownBoundary) {
    session_->ProcessChar('s');
    session_->ProcessChar('h');
    session_->ProcessChar('i');
    
    // 翻到最后一页
    session_->ProcessKey(KeyEvent::FromSpecial(KeyType::PageDown));  // 第 2 页
    session_->ProcessKey(KeyEvent::FromSpecial(KeyType::PageDown));  // 第 3 页
    EXPECT_EQ(session_->GetState().currentPage, 2);
    
    // 尝试继续往后翻
    auto result = session_->ProcessKey(KeyEvent::FromSpecial(KeyType::PageDown));
    EXPECT_TRUE(result.consumed);  // 按键被消费
    EXPECT_FALSE(result.needsUpdate);  // 但不需要更新（已经是最后一页）
    
    EXPECT_EQ(session_->GetState().currentPage, 2);
}

// 测试：最后一页候选词数量
TEST_F(InputSessionPaginationTest, LastPageCandidateCount) {
    session_->ProcessChar('s');
    session_->ProcessChar('h');
    session_->ProcessChar('i');
    
    // 翻到最后一页
    session_->ProcessKey(KeyEvent::FromSpecial(KeyType::PageDown));
    session_->ProcessKey(KeyEvent::FromSpecial(KeyType::PageDown));
    
    const auto& state = session_->GetState();
    EXPECT_EQ(state.currentPage, 2);
    EXPECT_EQ(state.candidates.size(), 5);  // 最后一页也是 5 个
    EXPECT_EQ(state.candidates[0].text, "始");  // 第三页第一个（索引 10）
}

// 测试：非输入状态下翻页键透传
TEST_F(InputSessionPaginationTest, PageKeysPassThroughWhenNotComposing) {
    auto result = session_->ProcessKey(KeyEvent::FromSpecial(KeyType::PageDown));
    EXPECT_FALSE(result.consumed);
    
    result = session_->ProcessKey(KeyEvent::FromSpecial(KeyType::PageUp));
    EXPECT_FALSE(result.consumed);
}

// 测试：只有一页时翻页键透传
TEST_F(InputSessionPaginationTest, PageKeysPassThroughWhenSinglePage) {
    // 设置只返回少量候选词的回调
    session_->SetCandidateQueryCallback([](const std::string& pinyin) {
        std::vector<CandidateWord> candidates;
        if (pinyin == "a") {
            candidates.push_back({"啊", "a", 100});
            candidates.push_back({"阿", "a", 90});
        }
        return candidates;
    });
    
    session_->ProcessChar('a');
    
    EXPECT_EQ(session_->GetState().totalPages, 1);
    
    auto result = session_->ProcessKey(KeyEvent::FromSpecial(KeyType::PageDown));
    EXPECT_FALSE(result.consumed);  // 只有一页，翻页键透传
}

// ==================== 空候选词处理测试 ====================

// 测试：无效拼音返回空候选词
TEST_F(InputSessionPaginationTest, InvalidPinyinEmptyCandidates) {
    session_->ProcessChar('x');
    session_->ProcessChar('y');
    session_->ProcessChar('z');
    
    const auto& state = session_->GetState();
    EXPECT_TRUE(state.candidates.empty());
    EXPECT_EQ(state.totalPages, 0);
    EXPECT_TRUE(session_->IsComposing());  // 仍在输入状态
}

// 测试：空候选词时需要隐藏候选窗口
TEST_F(InputSessionPaginationTest, EmptyCandidatesNeedsHide) {
    auto result = session_->ProcessChar('x');
    EXPECT_TRUE(result.consumed);
    EXPECT_TRUE(result.needsHide);  // 无匹配时需要隐藏候选窗口
    EXPECT_TRUE(result.needsUpdate);  // 但仍需要更新预编辑文本
    EXPECT_TRUE(session_->IsComposing());  // 仍在输入状态
    EXPECT_TRUE(session_->GetCandidates().empty());
}

// 测试：退格后候选词为空时需要隐藏候选窗口
TEST_F(InputSessionPaginationTest, BackspaceToEmptyCandidatesNeedsHide) {
    // 先输入有效拼音
    session_->ProcessChar('s');
    session_->ProcessChar('h');
    session_->ProcessChar('i');
    EXPECT_FALSE(session_->GetCandidates().empty());
    
    // 添加无效字符使候选词变空
    session_->ProcessChar('x');
    session_->ProcessChar('x');
    EXPECT_TRUE(session_->GetCandidates().empty());
    
    // 退格删除一个字符，候选词仍然为空
    auto result = session_->ProcessKey(KeyEvent::FromSpecial(KeyType::Backspace));
    EXPECT_TRUE(result.consumed);
    EXPECT_TRUE(result.needsHide);  // 候选词为空，需要隐藏
    EXPECT_TRUE(result.needsUpdate);  // 但仍需要更新预编辑文本
}

// 测试：空候选词时空格键提交原文
TEST_F(InputSessionPaginationTest, SpaceCommitsRawWhenNoCandidates) {
    session_->ProcessChar('x');
    session_->ProcessChar('y');
    session_->ProcessChar('z');
    
    EXPECT_TRUE(session_->GetCandidates().empty());
    
    auto result = session_->ProcessKey(KeyEvent::FromSpecial(KeyType::Space));
    EXPECT_TRUE(result.consumed);
    EXPECT_TRUE(result.needsCommit);
    EXPECT_EQ(result.commitText, "xyz");  // 提交原始拼音
}

// 测试：空候选词时数字键透传
TEST_F(InputSessionPaginationTest, DigitPassThroughWhenNoCandidates) {
    session_->ProcessChar('x');
    session_->ProcessChar('y');
    session_->ProcessChar('z');
    
    EXPECT_TRUE(session_->GetCandidates().empty());
    
    auto result = session_->ProcessKey(KeyEvent::FromChar('1'));
    EXPECT_FALSE(result.consumed);  // 没有候选词，数字键透传
}
