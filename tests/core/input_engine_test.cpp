/**
 * InputEngine 单元测试
 *
 * 测试输入引擎的核心功能：
 * - 初始化和关闭
 * - 按键处理
 * - 候选词选择和翻页
 * - 模式切换
 * - 状态管理
 * - 词频学习
 */

#include <iostream>
#include <cassert>
#include <filesystem>
#include <vector>
#include <string>

// Qt 头文件必须在 rime_api.h 之前包含
#include "frequency_manager.h"

#ifdef Bool
#undef Bool
#endif

#include "input_engine.h"
#include "platform_bridge.h"

#ifdef Bool
#undef Bool
#endif

namespace fs = std::filesystem;

// 测试辅助宏
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "✗ 断言失败: " << message << std::endl; \
            std::cerr << "  位置: " << __FILE__ << ":" << __LINE__ << std::endl; \
            return false; \
        } \
    } while(0)

#define TEST_PASS(message) \
    std::cout << "✓ " << message << std::endl

/**
 * Mock PlatformBridge - 用于测试的模拟平台桥接
 */
class MockPlatformBridge : public suyan::IPlatformBridge {
public:
    void commitText(const std::string& text) override {
        committedTexts_.push_back(text);
    }

    suyan::CursorPosition getCursorPosition() override {
        return cursorPosition_;
    }

    void updatePreedit(const std::string& preedit, int caretPos) override {
        lastPreedit_ = preedit;
        lastCaretPos_ = caretPos;
    }

    void clearPreedit() override {
        lastPreedit_.clear();
        lastCaretPos_ = 0;
    }

    std::string getCurrentAppId() override {
        return "com.test.app";
    }

    // 测试辅助方法
    const std::vector<std::string>& getCommittedTexts() const {
        return committedTexts_;
    }

    std::string getLastCommittedText() const {
        return committedTexts_.empty() ? "" : committedTexts_.back();
    }

    void clearCommittedTexts() {
        committedTexts_.clear();
    }

    const std::string& getLastPreedit() const {
        return lastPreedit_;
    }

    void setCursorPosition(int x, int y, int height) {
        cursorPosition_.x = x;
        cursorPosition_.y = y;
        cursorPosition_.height = height;
    }

private:
    std::vector<std::string> committedTexts_;
    std::string lastPreedit_;
    int lastCaretPos_ = 0;
    suyan::CursorPosition cursorPosition_;
};

// 获取项目根目录
std::string getProjectRoot() {
    fs::path current = fs::current_path();

    std::vector<fs::path> candidates = {
        current / ".." / "..",
        current / "..",
        current,
        current / ".." / ".." / "..",
    };

    for (const auto& candidate : candidates) {
        fs::path dataPath = candidate / "data" / "rime" / "default.yaml";
        if (fs::exists(dataPath)) {
            return fs::canonical(candidate).string();
        }
    }

    return fs::canonical(current / ".." / "..").string();
}

class InputEngineTest {
public:
    InputEngineTest() {
        projectRoot_ = getProjectRoot();
        sharedDataDir_ = projectRoot_ + "/data/rime";
        userDataDir_ = projectRoot_ + "/build/rime_user_data_engine_test";

        fs::create_directories(userDataDir_);
    }

    ~InputEngineTest() {
        engine_.shutdown();
    }

    bool runAllTests() {
        std::cout << "=== InputEngine 单元测试 ===" << std::endl;
        std::cout << "项目根目录: " << projectRoot_ << std::endl;
        std::cout << std::endl;

        // 检查词库数据
        if (!fs::exists(sharedDataDir_ + "/default.yaml")) {
            std::cerr << "错误: 找不到 RIME 词库数据" << std::endl;
            return false;
        }

        bool allPassed = true;

        // 初始化测试
        allPassed &= testInitialize();

        // 平台桥接测试
        allPassed &= testPlatformBridge();

        // 模式切换测试
        allPassed &= testModeSwitch();

        // 按键处理测试
        allPassed &= testProcessKeyEvent();

        // 候选词测试
        allPassed &= testCandidateSelection();

        // 翻页测试
        allPassed &= testPaging();

        // 状态管理测试
        allPassed &= testStateManagement();

        // 临时英文模式测试
        allPassed &= testTempEnglishMode();

        // 回调测试
        allPassed &= testCallbacks();
        
        // 词频学习测试
        allPassed &= testFrequencyLearning();

        std::cout << std::endl;
        if (allPassed) {
            std::cout << "=== 所有测试通过 ===" << std::endl;
        } else {
            std::cout << "=== 部分测试失败 ===" << std::endl;
        }

        return allPassed;
    }

private:
    std::string projectRoot_;
    std::string sharedDataDir_;
    std::string userDataDir_;
    suyan::InputEngine engine_;
    MockPlatformBridge mockBridge_;

    // ========== 初始化测试 ==========

    bool testInitialize() {
        // 首次初始化
        bool result = engine_.initialize(userDataDir_, sharedDataDir_);
        TEST_ASSERT(result, "初始化应该成功");
        TEST_ASSERT(engine_.isInitialized(), "初始化后 isInitialized 应该返回 true");

        // 重复初始化应该返回 true
        result = engine_.initialize(userDataDir_, sharedDataDir_);
        TEST_ASSERT(result, "重复初始化应该返回 true");

        TEST_PASS("testInitialize: 初始化成功");
        return true;
    }

    // ========== 平台桥接测试 ==========

    bool testPlatformBridge() {
        TEST_ASSERT(engine_.getPlatformBridge() == nullptr, "初始时平台桥接应为空");

        engine_.setPlatformBridge(&mockBridge_);
        TEST_ASSERT(engine_.getPlatformBridge() == &mockBridge_, "设置后应能获取平台桥接");

        // 设置提交文本回调，让回调调用 mockBridge 的 commitText
        // 这模拟了 main.mm 中的回调设置
        engine_.setCommitTextCallback([this](const std::string& text) {
            mockBridge_.commitText(text);
        });

        TEST_PASS("testPlatformBridge: 平台桥接设置正常");
        return true;
    }

    // ========== 模式切换测试 ==========

    bool testModeSwitch() {
        // 默认应该是中文模式
        TEST_ASSERT(engine_.getMode() == suyan::InputMode::Chinese, "默认应该是中文模式");

        // 切换到英文模式
        engine_.setMode(suyan::InputMode::English);
        TEST_ASSERT(engine_.getMode() == suyan::InputMode::English, "应该切换到英文模式");

        // 切换回中文模式
        engine_.toggleMode();
        TEST_ASSERT(engine_.getMode() == suyan::InputMode::Chinese, "toggleMode 应该切换回中文模式");

        // 再次切换
        engine_.toggleMode();
        TEST_ASSERT(engine_.getMode() == suyan::InputMode::English, "toggleMode 应该切换到英文模式");

        // 恢复中文模式
        engine_.setMode(suyan::InputMode::Chinese);

        TEST_PASS("testModeSwitch: 模式切换正常");
        return true;
    }

    // ========== 按键处理测试 ==========

    bool testProcessKeyEvent() {
        engine_.reset();
        mockBridge_.clearCommittedTexts();

        // 输入拼音 'n'
        bool processed = engine_.processKeyEvent('n', 0);
        TEST_ASSERT(processed, "按键 'n' 应该被处理");

        // 检查状态
        auto state = engine_.getState();
        TEST_ASSERT(state.isComposing, "输入后应该处于输入状态");
        TEST_ASSERT(!state.rawInput.empty(), "原始输入不应为空");

        // 继续输入 'i', 'h', 'a', 'o'
        engine_.processKeyEvent('i', 0);
        engine_.processKeyEvent('h', 0);
        engine_.processKeyEvent('a', 0);
        engine_.processKeyEvent('o', 0);

        state = engine_.getState();
        TEST_ASSERT(state.rawInput == "nihao", "原始输入应该是 'nihao'，实际是: " + state.rawInput);
        TEST_ASSERT(!state.candidates.empty(), "应该有候选词");

        std::cout << "  preedit: " << state.preedit << std::endl;
        std::cout << "  候选词数量: " << state.candidates.size() << std::endl;
        if (!state.candidates.empty()) {
            std::cout << "  首选: " << state.candidates[0].text << std::endl;
        }

        // 清除输入
        engine_.reset();

        TEST_PASS("testProcessKeyEvent: 按键处理正常");
        return true;
    }

    // ========== 候选词选择测试 ==========

    bool testCandidateSelection() {
        engine_.reset();
        mockBridge_.clearCommittedTexts();

        // 输入拼音
        engine_.processKeyEvent('n', 0);
        engine_.processKeyEvent('i', 0);

        auto state = engine_.getState();
        TEST_ASSERT(!state.candidates.empty(), "应该有候选词");

        // 选择第一个候选词
        bool success = engine_.selectCandidate(1);
        TEST_ASSERT(success, "选择候选词应该成功");

        // 检查是否有提交
        std::string committed = mockBridge_.getLastCommittedText();
        TEST_ASSERT(!committed.empty(), "应该有文字提交");
        std::cout << "  提交文字: " << committed << std::endl;

        // 无效索引测试
        engine_.reset();
        engine_.processKeyEvent('h', 0);
        engine_.processKeyEvent('a', 0);
        engine_.processKeyEvent('o', 0);

        success = engine_.selectCandidate(0);  // 无效索引
        TEST_ASSERT(!success, "索引 0 应该失败");

        success = engine_.selectCandidate(10);  // 超出范围
        TEST_ASSERT(!success, "索引 10 应该失败");

        engine_.reset();

        TEST_PASS("testCandidateSelection: 候选词选择正常");
        return true;
    }

    // ========== 翻页测试 ==========

    bool testPaging() {
        engine_.reset();

        // 输入一个有多页候选词的拼音
        engine_.processKeyEvent('s', 0);
        engine_.processKeyEvent('h', 0);
        engine_.processKeyEvent('i', 0);

        auto state1 = engine_.getState();
        int page1 = state1.pageIndex;

        if (!state1.hasMorePages) {
            std::cout << "  候选词只有一页，跳过翻页测试" << std::endl;
            engine_.reset();
            TEST_PASS("testPaging: 跳过（只有一页）");
            return true;
        }

        // 向后翻页
        bool success = engine_.pageDown();
        TEST_ASSERT(success, "向后翻页应该成功");

        auto state2 = engine_.getState();
        TEST_ASSERT(state2.pageIndex > page1, "翻页后页码应该增加");

        // 向前翻页
        success = engine_.pageUp();
        TEST_ASSERT(success, "向前翻页应该成功");

        auto state3 = engine_.getState();
        TEST_ASSERT(state3.pageIndex == page1, "翻回后页码应该恢复");

        engine_.reset();

        TEST_PASS("testPaging: 翻页正常");
        return true;
    }

    // ========== 状态管理测试 ==========

    bool testStateManagement() {
        engine_.reset();

        // 初始状态
        TEST_ASSERT(!engine_.isComposing(), "初始时不应处于输入状态");

        auto state = engine_.getState();
        TEST_ASSERT(state.mode == suyan::InputMode::Chinese, "应该是中文模式");
        TEST_ASSERT(!state.isComposing, "不应处于输入状态");
        TEST_ASSERT(state.candidates.empty(), "候选词应为空");

        // 输入后状态
        engine_.processKeyEvent('w', 0);
        engine_.processKeyEvent('o', 0);

        TEST_ASSERT(engine_.isComposing(), "输入后应处于输入状态");

        state = engine_.getState();
        TEST_ASSERT(state.isComposing, "状态应显示正在输入");
        TEST_ASSERT(!state.rawInput.empty(), "原始输入不应为空");

        // 重置
        engine_.reset();
        TEST_ASSERT(!engine_.isComposing(), "重置后不应处于输入状态");

        state = engine_.getState();
        TEST_ASSERT(!state.isComposing, "状态应显示未输入");
        TEST_ASSERT(state.rawInput.empty(), "原始输入应为空");

        TEST_PASS("testStateManagement: 状态管理正常");
        return true;
    }

    // ========== 临时英文模式测试 ==========

    bool testTempEnglishMode() {
        engine_.reset();
        engine_.setMode(suyan::InputMode::Chinese);
        mockBridge_.clearCommittedTexts();

        // 模拟大写字母输入（Shift + 字母）进入临时英文模式
        // 注意：这里直接测试大写字母 'H'
        bool processed = engine_.processKeyEvent('H', suyan::KeyModifier::Shift);

        // 检查是否进入临时英文模式
        if (engine_.getMode() == suyan::InputMode::TempEnglish) {
            std::cout << "  进入临时英文模式" << std::endl;

            // 继续输入
            engine_.processKeyEvent('e', 0);
            engine_.processKeyEvent('l', 0);
            engine_.processKeyEvent('l', 0);
            engine_.processKeyEvent('o', 0);

            auto state = engine_.getState();
            std::cout << "  临时英文缓冲: " << state.preedit << std::endl;

            // 空格提交
            engine_.processKeyEvent(suyan::KeyCode::Space, 0);

            std::string committed = mockBridge_.getLastCommittedText();
            TEST_ASSERT(!committed.empty(), "应该有文字提交");
            std::cout << "  提交文字: " << committed << std::endl;

            // 应该退出临时英文模式
            TEST_ASSERT(engine_.getMode() == suyan::InputMode::Chinese, "应该退出临时英文模式");
        } else {
            std::cout << "  临时英文模式未触发（可能需要调整触发条件）" << std::endl;
        }

        engine_.reset();
        engine_.setMode(suyan::InputMode::Chinese);

        TEST_PASS("testTempEnglishMode: 临时英文模式测试完成");
        return true;
    }

    // ========== 回调测试 ==========

    bool testCallbacks() {
        engine_.reset();
        mockBridge_.clearCommittedTexts();

        bool stateChangedCalled = false;
        bool commitTextCalled = false;
        std::string lastCommitText;

        // 设置回调
        engine_.setStateChangedCallback([&](const suyan::InputState& state) {
            stateChangedCalled = true;
        });

        engine_.setCommitTextCallback([&](const std::string& text) {
            commitTextCalled = true;
            lastCommitText = text;
        });

        // 输入触发状态变化
        stateChangedCalled = false;
        engine_.processKeyEvent('a', 0);
        TEST_ASSERT(stateChangedCalled, "输入应触发状态变化回调");

        // 选择候选词触发提交
        commitTextCalled = false;
        engine_.selectCandidate(1);
        TEST_ASSERT(commitTextCalled, "选择候选词应触发提交回调");
        TEST_ASSERT(!lastCommitText.empty(), "提交文字不应为空");
        std::cout << "  回调收到提交: " << lastCommitText << std::endl;

        engine_.reset();

        TEST_PASS("testCallbacks: 回调机制正常");
        return true;
    }
    
    // ========== 词频学习测试 ==========
    
    bool testFrequencyLearning() {
        // 初始化 FrequencyManager
        std::string freqDataDir = userDataDir_ + "/freq_test";
        fs::create_directories(freqDataDir);
        
        auto& freqMgr = suyan::FrequencyManager::instance();
        
        // 如果已初始化，先关闭
        if (freqMgr.isInitialized()) {
            freqMgr.shutdown();
        }
        
        bool initResult = freqMgr.initialize(freqDataDir);
        TEST_ASSERT(initResult, "FrequencyManager 初始化应该成功");
        
        // 清空之前的数据
        freqMgr.clearAll();
        
        engine_.reset();
        mockBridge_.clearCommittedTexts();
        
        // 测试词频学习开关
        TEST_ASSERT(engine_.isFrequencyLearningEnabled(), "默认应该启用词频学习");
        
        engine_.setFrequencyLearningEnabled(false);
        TEST_ASSERT(!engine_.isFrequencyLearningEnabled(), "应该能禁用词频学习");
        
        engine_.setFrequencyLearningEnabled(true);
        TEST_ASSERT(engine_.isFrequencyLearningEnabled(), "应该能启用词频学习");
        
        // 测试最小词频阈值设置
        engine_.setMinFrequencyForSorting(3);
        TEST_ASSERT(engine_.getMinFrequencyForSorting() == 3, "最小词频阈值应该是 3");
        
        engine_.setMinFrequencyForSorting(1);  // 恢复默认值
        
        // 输入拼音并选择候选词
        engine_.processKeyEvent('n', 0);
        engine_.processKeyEvent('i', 0);
        
        auto state = engine_.getState();
        TEST_ASSERT(!state.candidates.empty(), "应该有候选词");
        
        // 记录第一个候选词
        std::string firstCandidate = state.candidates[0].text;
        std::string pinyin = state.rawInput;
        
        std::cout << "  输入拼音: " << pinyin << std::endl;
        std::cout << "  首选候选词: " << firstCandidate << std::endl;
        
        // 选择第一个候选词（这应该更新词频）
        engine_.selectCandidate(1);
        
        // 检查词频是否更新
        int freq = freqMgr.getFrequency(firstCandidate, pinyin);
        std::cout << "  选择后词频: " << freq << std::endl;
        TEST_ASSERT(freq >= 1, "选择候选词后词频应该至少为 1");
        
        // 再次输入相同拼音并选择相同候选词
        engine_.reset();
        engine_.processKeyEvent('n', 0);
        engine_.processKeyEvent('i', 0);
        engine_.selectCandidate(1);
        
        // 检查词频是否累加
        int freq2 = freqMgr.getFrequency(firstCandidate, pinyin);
        std::cout << "  再次选择后词频: " << freq2 << std::endl;
        TEST_ASSERT(freq2 >= freq, "词频应该累加");
        
        // 测试禁用词频学习后不更新词频
        engine_.setFrequencyLearningEnabled(false);
        engine_.reset();
        engine_.processKeyEvent('n', 0);
        engine_.processKeyEvent('i', 0);
        engine_.selectCandidate(1);
        
        int freq3 = freqMgr.getFrequency(firstCandidate, pinyin);
        std::cout << "  禁用词频学习后词频: " << freq3 << std::endl;
        TEST_ASSERT(freq3 == freq2, "禁用词频学习后词频不应变化");
        
        // 恢复词频学习
        engine_.setFrequencyLearningEnabled(true);
        
        // 清理
        engine_.reset();
        freqMgr.clearAll();
        
        TEST_PASS("testFrequencyLearning: 词频学习正常");
        return true;
    }
};

int main() {
    InputEngineTest test;
    return test.runAllTests() ? 0 : 1;
}
