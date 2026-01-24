/**
 * RimeWrapper 单元测试
 *
 * 测试 RimeWrapper 对 librime API 的封装功能。
 * 需要 rime-ice 词库数据才能完整测试。
 */

#include <iostream>
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include "rime_wrapper.h"

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

// 获取项目根目录
std::string getProjectRoot() {
    // 从构建目录向上查找项目根目录
    fs::path current = fs::current_path();
    
    // 尝试多个可能的路径
    std::vector<fs::path> candidates = {
        current / ".." / "..",           // build/bin -> root
        current / "..",                   // build -> root
        current,                          // 当前目录就是 root
        current / ".." / ".." / "..",    // 更深的构建目录
    };
    
    for (const auto& candidate : candidates) {
        fs::path dataPath = candidate / "data" / "rime" / "default.yaml";
        if (fs::exists(dataPath)) {
            return fs::canonical(candidate).string();
        }
    }
    
    // 如果找不到，返回当前目录的上两级（假设在 build/bin 中运行）
    return fs::canonical(current / ".." / "..").string();
}

class RimeWrapperTest {
public:
    RimeWrapperTest() {
        projectRoot_ = getProjectRoot();
        sharedDataDir_ = projectRoot_ + "/data/rime";
        userDataDir_ = projectRoot_ + "/build/rime_user_data";
        
        // 创建用户数据目录
        fs::create_directories(userDataDir_);
    }
    
    ~RimeWrapperTest() {
        // 清理：销毁会话并关闭 RIME
        auto& rime = suyan::RimeWrapper::instance();
        if (sessionId_ != 0) {
            rime.destroySession(sessionId_);
        }
        rime.finalize();
    }
    
    bool runAllTests() {
        std::cout << "=== RimeWrapper 单元测试 ===" << std::endl;
        std::cout << "项目根目录: " << projectRoot_ << std::endl;
        std::cout << "共享数据目录: " << sharedDataDir_ << std::endl;
        std::cout << "用户数据目录: " << userDataDir_ << std::endl;
        std::cout << std::endl;
        
        // 检查词库数据是否存在
        if (!fs::exists(sharedDataDir_ + "/default.yaml")) {
            std::cerr << "错误: 找不到 RIME 词库数据" << std::endl;
            std::cerr << "请确保 data/rime/ 目录包含 rime-ice 词库文件" << std::endl;
            return false;
        }
        
        bool allPassed = true;
        
        // 基础测试（初始化）
        allPassed &= testGetInstance();
        allPassed &= testInitialize();
        allPassed &= testGetVersion();
        
        // 等待部署完成（必须在创建会话之前）
        std::cout << "\n等待 RIME 部署完成..." << std::endl;
        auto& rime = suyan::RimeWrapper::instance();
        rime.joinMaintenanceThread();
        TEST_PASS("部署完成");
        std::cout << std::endl;
        
        // 会话测试（部署完成后）
        allPassed &= testCreateSession();
        allPassed &= testFindSession();
        
        // 输入处理测试
        allPassed &= testProcessKey();
        allPassed &= testSimulateKeySequence();
        allPassed &= testGetComposition();
        allPassed &= testGetCandidateMenu();
        allPassed &= testSelectCandidate();
        allPassed &= testChangePage();
        allPassed &= testClearComposition();
        allPassed &= testCommitComposition();
        
        // 状态测试
        allPassed &= testGetState();
        allPassed &= testGetSetOption();
        
        // 方案测试
        allPassed &= testGetSchemaList();
        allPassed &= testGetCurrentSchemaId();
        
        // 会话销毁测试
        allPassed &= testDestroySession();
        
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
    RimeSessionId sessionId_ = 0;
    
    // ========== 基础测试 ==========
    
    bool testGetInstance() {
        auto& rime1 = suyan::RimeWrapper::instance();
        auto& rime2 = suyan::RimeWrapper::instance();
        TEST_ASSERT(&rime1 == &rime2, "单例实例应该相同");
        TEST_PASS("testGetInstance: 单例模式正常");
        return true;
    }
    
    bool testInitialize() {
        auto& rime = suyan::RimeWrapper::instance();
        
        // 首次初始化
        bool result = rime.initialize(userDataDir_, sharedDataDir_, "RimeWrapperTest");
        TEST_ASSERT(result, "初始化应该成功");
        TEST_ASSERT(rime.isInitialized(), "初始化后 isInitialized 应该返回 true");
        
        // 重复初始化应该返回 true（幂等）
        result = rime.initialize(userDataDir_, sharedDataDir_, "RimeWrapperTest");
        TEST_ASSERT(result, "重复初始化应该返回 true");
        
        // 启动维护任务
        rime.startMaintenance(true);
        
        TEST_PASS("testInitialize: 初始化成功");
        return true;
    }
    
    bool testGetVersion() {
        auto& rime = suyan::RimeWrapper::instance();
        std::string version = rime.getVersion();
        TEST_ASSERT(!version.empty(), "版本号不应为空");
        std::cout << "  librime 版本: " << version << std::endl;
        TEST_PASS("testGetVersion: 获取版本成功");
        return true;
    }
    
    bool testCreateSession() {
        auto& rime = suyan::RimeWrapper::instance();
        sessionId_ = rime.createSession();
        TEST_ASSERT(sessionId_ != 0, "创建会话应该返回非零 ID");
        std::cout << "  会话 ID: " << sessionId_ << std::endl;
        TEST_PASS("testCreateSession: 创建会话成功");
        return true;
    }
    
    bool testFindSession() {
        auto& rime = suyan::RimeWrapper::instance();
        TEST_ASSERT(rime.findSession(sessionId_), "应该能找到已创建的会话");
        TEST_ASSERT(!rime.findSession(99999), "不应该找到不存在的会话");
        TEST_PASS("testFindSession: 查找会话正常");
        return true;
    }
    
    // ========== 输入处理测试 ==========
    
    bool testProcessKey() {
        auto& rime = suyan::RimeWrapper::instance();
        
        // 清除之前的输入
        rime.clearComposition(sessionId_);
        
        // 输入 'n' (ASCII 110)
        bool processed = rime.processKey(sessionId_, 'n', 0);
        TEST_ASSERT(processed, "按键 'n' 应该被处理");
        
        // 检查原始输入
        std::string input = rime.getRawInput(sessionId_);
        TEST_ASSERT(input == "n", "原始输入应该是 'n'");
        
        // 清除
        rime.clearComposition(sessionId_);
        
        TEST_PASS("testProcessKey: 按键处理正常");
        return true;
    }
    
    bool testSimulateKeySequence() {
        auto& rime = suyan::RimeWrapper::instance();
        
        // 清除之前的输入
        rime.clearComposition(sessionId_);
        
        // 模拟输入 "nihao"
        bool result = rime.simulateKeySequence(sessionId_, "nihao");
        TEST_ASSERT(result, "模拟按键序列应该成功");
        
        // 检查原始输入
        std::string input = rime.getRawInput(sessionId_);
        TEST_ASSERT(input == "nihao", "原始输入应该是 'nihao'，实际是: " + input);
        
        TEST_PASS("testSimulateKeySequence: 模拟按键序列正常");
        return true;
    }
    
    bool testGetComposition() {
        auto& rime = suyan::RimeWrapper::instance();
        
        // 此时应该有 "nihao" 的输入
        auto comp = rime.getComposition(sessionId_);
        TEST_ASSERT(!comp.preedit.empty(), "preedit 不应为空");
        std::cout << "  preedit: " << comp.preedit << std::endl;
        std::cout << "  cursorPos: " << comp.cursorPos << std::endl;
        
        TEST_PASS("testGetComposition: 获取组合信息正常");
        return true;
    }
    
    bool testGetCandidateMenu() {
        auto& rime = suyan::RimeWrapper::instance();
        
        // 获取候选词菜单
        auto menu = rime.getCandidateMenu(sessionId_);
        TEST_ASSERT(!menu.candidates.empty(), "候选词列表不应为空");
        
        std::cout << "  候选词数量: " << menu.candidates.size() << std::endl;
        std::cout << "  页大小: " << menu.pageSize << std::endl;
        std::cout << "  当前页: " << menu.pageIndex << std::endl;
        std::cout << "  是否最后一页: " << (menu.isLastPage ? "是" : "否") << std::endl;
        
        // 打印前几个候选词
        int count = std::min(5, (int)menu.candidates.size());
        for (int i = 0; i < count; ++i) {
            std::cout << "  [" << (i + 1) << "] " << menu.candidates[i].text;
            if (!menu.candidates[i].comment.empty()) {
                std::cout << " (" << menu.candidates[i].comment << ")";
            }
            std::cout << std::endl;
        }
        
        TEST_PASS("testGetCandidateMenu: 获取候选词菜单正常");
        return true;
    }
    
    bool testSelectCandidate() {
        auto& rime = suyan::RimeWrapper::instance();
        
        // 选择第一个候选词
        bool result = rime.selectCandidateOnCurrentPage(sessionId_, 0);
        TEST_ASSERT(result, "选择候选词应该成功");
        
        // 获取提交的文本
        std::string commitText = rime.getCommitText(sessionId_);
        TEST_ASSERT(!commitText.empty(), "提交文本不应为空");
        std::cout << "  提交文本: " << commitText << std::endl;
        
        TEST_PASS("testSelectCandidate: 选择候选词正常");
        return true;
    }
    
    bool testChangePage() {
        auto& rime = suyan::RimeWrapper::instance();
        
        // 重新输入以测试翻页
        rime.clearComposition(sessionId_);
        rime.simulateKeySequence(sessionId_, "shi");
        
        auto menu1 = rime.getCandidateMenu(sessionId_);
        if (menu1.isLastPage) {
            std::cout << "  候选词只有一页，跳过翻页测试" << std::endl;
            rime.clearComposition(sessionId_);
            TEST_PASS("testChangePage: 跳过（只有一页）");
            return true;
        }
        
        // 向后翻页
        bool result = rime.changePage(sessionId_, false);
        TEST_ASSERT(result, "向后翻页应该成功");
        
        auto menu2 = rime.getCandidateMenu(sessionId_);
        TEST_ASSERT(menu2.pageIndex > menu1.pageIndex, "翻页后页码应该增加");
        
        // 向前翻页
        result = rime.changePage(sessionId_, true);
        TEST_ASSERT(result, "向前翻页应该成功");
        
        auto menu3 = rime.getCandidateMenu(sessionId_);
        TEST_ASSERT(menu3.pageIndex == menu1.pageIndex, "翻回后页码应该恢复");
        
        rime.clearComposition(sessionId_);
        
        TEST_PASS("testChangePage: 翻页正常");
        return true;
    }
    
    bool testClearComposition() {
        auto& rime = suyan::RimeWrapper::instance();
        
        // 输入一些内容
        rime.simulateKeySequence(sessionId_, "test");
        std::string input1 = rime.getRawInput(sessionId_);
        TEST_ASSERT(!input1.empty(), "输入后原始输入不应为空");
        
        // 清除
        rime.clearComposition(sessionId_);
        
        std::string input2 = rime.getRawInput(sessionId_);
        TEST_ASSERT(input2.empty(), "清除后原始输入应为空");
        
        TEST_PASS("testClearComposition: 清除输入正常");
        return true;
    }
    
    bool testCommitComposition() {
        auto& rime = suyan::RimeWrapper::instance();
        
        // 输入拼音
        rime.simulateKeySequence(sessionId_, "hao");
        
        // 提交
        bool result = rime.commitComposition(sessionId_);
        TEST_ASSERT(result, "提交应该成功");
        
        // 获取提交文本
        std::string commitText = rime.getCommitText(sessionId_);
        TEST_ASSERT(!commitText.empty(), "提交文本不应为空");
        std::cout << "  提交文本: " << commitText << std::endl;
        
        TEST_PASS("testCommitComposition: 提交输入正常");
        return true;
    }
    
    // ========== 状态测试 ==========
    
    bool testGetState() {
        auto& rime = suyan::RimeWrapper::instance();
        
        auto state = rime.getState(sessionId_);
        std::cout << "  方案 ID: " << state.schemaId << std::endl;
        std::cout << "  方案名称: " << state.schemaName << std::endl;
        std::cout << "  是否正在输入: " << (state.isComposing ? "是" : "否") << std::endl;
        std::cout << "  是否 ASCII 模式: " << (state.isAsciiMode ? "是" : "否") << std::endl;
        
        TEST_PASS("testGetState: 获取状态正常");
        return true;
    }
    
    bool testGetSetOption() {
        auto& rime = suyan::RimeWrapper::instance();
        
        // 获取 ascii_mode 选项
        bool asciiMode = rime.getOption(sessionId_, "ascii_mode");
        std::cout << "  当前 ascii_mode: " << (asciiMode ? "true" : "false") << std::endl;
        
        // 切换选项
        rime.setOption(sessionId_, "ascii_mode", !asciiMode);
        bool newAsciiMode = rime.getOption(sessionId_, "ascii_mode");
        TEST_ASSERT(newAsciiMode != asciiMode, "选项应该被切换");
        
        // 恢复原值
        rime.setOption(sessionId_, "ascii_mode", asciiMode);
        
        TEST_PASS("testGetSetOption: 获取/设置选项正常");
        return true;
    }
    
    // ========== 方案测试 ==========
    
    bool testGetSchemaList() {
        auto& rime = suyan::RimeWrapper::instance();
        
        auto schemas = rime.getSchemaList();
        TEST_ASSERT(!schemas.empty(), "方案列表不应为空");
        
        std::cout << "  可用方案:" << std::endl;
        for (const auto& [id, name] : schemas) {
            std::cout << "    - " << id << ": " << name << std::endl;
        }
        
        TEST_PASS("testGetSchemaList: 获取方案列表正常");
        return true;
    }
    
    bool testGetCurrentSchemaId() {
        auto& rime = suyan::RimeWrapper::instance();
        
        std::string schemaId = rime.getCurrentSchemaId(sessionId_);
        TEST_ASSERT(!schemaId.empty(), "当前方案 ID 不应为空");
        std::cout << "  当前方案 ID: " << schemaId << std::endl;
        
        TEST_PASS("testGetCurrentSchemaId: 获取当前方案 ID 正常");
        return true;
    }
    
    // ========== 会话销毁测试 ==========
    
    bool testDestroySession() {
        auto& rime = suyan::RimeWrapper::instance();
        
        // 创建一个临时会话
        RimeSessionId tempSession = rime.createSession();
        TEST_ASSERT(tempSession != 0, "创建临时会话应该成功");
        TEST_ASSERT(rime.findSession(tempSession), "应该能找到临时会话");
        
        // 销毁
        rime.destroySession(tempSession);
        TEST_ASSERT(!rime.findSession(tempSession), "销毁后不应该找到会话");
        
        TEST_PASS("testDestroySession: 销毁会话正常");
        return true;
    }
};

int main() {
    RimeWrapperTest test;
    return test.runAllTests() ? 0 : 1;
}
