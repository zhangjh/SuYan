// tests/unit/weasel_integration_test.cpp
// Weasel 集成层单元测试

#include <gtest/gtest.h>

// 注意：这些测试只在 Windows 平台上运行
// 在其他平台上，测试会被跳过

#ifdef PLATFORM_WINDOWS

#include "platform/windows/weasel_integration.h"
#include "platform/windows/weasel_handler_ext.h"
#include "platform/windows/weasel_tray_ext.h"

using namespace ime::platform::windows;

class WeaselIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 测试前确保未初始化
        WeaselIntegration::Instance().Shutdown();
    }

    void TearDown() override {
        // 测试后清理
        WeaselIntegration::Instance().Shutdown();
    }
};

// 测试初始化和关闭
TEST_F(WeaselIntegrationTest, InitializeAndShutdown) {
    WeaselIntegrationConfig config = WeaselIntegrationConfig::Default();
    
    // 初始化
    EXPECT_TRUE(WeaselIntegration::Instance().Initialize(config));
    EXPECT_TRUE(WeaselIntegration::Instance().IsInitialized());
    
    // 关闭
    WeaselIntegration::Instance().Shutdown();
    EXPECT_FALSE(WeaselIntegration::Instance().IsInitialized());
}

// 测试重复初始化
TEST_F(WeaselIntegrationTest, DoubleInitialize) {
    WeaselIntegrationConfig config = WeaselIntegrationConfig::Default();
    
    EXPECT_TRUE(WeaselIntegration::Instance().Initialize(config));
    EXPECT_TRUE(WeaselIntegration::Instance().Initialize(config));  // 应该返回 true
    EXPECT_TRUE(WeaselIntegration::Instance().IsInitialized());
}

// 测试输入模式切换
TEST_F(WeaselIntegrationTest, InputModeToggle) {
    WeaselIntegrationConfig config = WeaselIntegrationConfig::Default();
    ASSERT_TRUE(WeaselIntegration::Instance().Initialize(config));
    
    // 默认应该是中文模式
    EXPECT_EQ(WeaselIntegration::Instance().GetInputMode(), 
              ime::input::InputMode::Chinese);
    
    // 切换到英文模式
    WeaselIntegration::Instance().ToggleInputMode();
    EXPECT_EQ(WeaselIntegration::Instance().GetInputMode(), 
              ime::input::InputMode::English);
    
    // 再次切换回中文模式
    WeaselIntegration::Instance().ToggleInputMode();
    EXPECT_EQ(WeaselIntegration::Instance().GetInputMode(), 
              ime::input::InputMode::Chinese);
}

// 测试候选词合并（空输入）
TEST_F(WeaselIntegrationTest, MergeCandidatesEmpty) {
    WeaselIntegrationConfig config = WeaselIntegrationConfig::Default();
    ASSERT_TRUE(WeaselIntegration::Instance().Initialize(config));
    
    std::vector<std::string> empty;
    auto result = WeaselIntegration::Instance().MergeCandidates(empty, "ni");
    
    // 空输入应该返回空结果（或只有用户高频词）
    // 由于没有用户词频数据，应该返回空
    EXPECT_TRUE(result.empty());
}

// 测试候选词合并（有输入）
TEST_F(WeaselIntegrationTest, MergeCandidatesWithInput) {
    WeaselIntegrationConfig config = WeaselIntegrationConfig::Default();
    ASSERT_TRUE(WeaselIntegration::Instance().Initialize(config));
    
    std::vector<std::string> candidates = {"你", "尼", "泥", "逆", "腻"};
    auto result = WeaselIntegration::Instance().MergeCandidates(candidates, "ni");
    
    // 应该返回合并后的结果
    EXPECT_FALSE(result.empty());
    EXPECT_GE(result.size(), candidates.size());
}

// 测试词频记录
TEST_F(WeaselIntegrationTest, RecordWordSelection) {
    WeaselIntegrationConfig config = WeaselIntegrationConfig::Default();
    ASSERT_TRUE(WeaselIntegration::Instance().Initialize(config));
    
    // 记录选择
    WeaselIntegration::Instance().RecordWordSelection("你好", "nihao");
    
    // 获取用户高频词
    auto topWords = WeaselIntegration::Instance().GetUserTopWords("nihao", 5);
    
    // 应该包含刚才记录的词
    bool found = false;
    for (const auto& word : topWords) {
        if (word == "你好") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

// 测试配置读写
TEST_F(WeaselIntegrationTest, ConfigReadWrite) {
    WeaselIntegrationConfig config = WeaselIntegrationConfig::Default();
    ASSERT_TRUE(WeaselIntegration::Instance().Initialize(config));
    
    // 写入配置
    EXPECT_TRUE(WeaselIntegration::Instance().SetConfig("test.key", "test_value"));
    
    // 读取配置
    auto value = WeaselIntegration::Instance().GetConfig("test.key");
    EXPECT_EQ(value, "test_value");
    
    // 读取不存在的配置
    auto defaultValue = WeaselIntegration::Instance().GetConfig("nonexistent", "default");
    EXPECT_EQ(defaultValue, "default");
}

// 测试 C 接口
TEST_F(WeaselIntegrationTest, CInterface) {
    // 初始化
    EXPECT_EQ(ImeIntegration_Initialize(nullptr, nullptr), 0);
    
    // 获取输入模式
    EXPECT_EQ(ImeIntegration_GetInputMode(), 0);  // 中文模式
    
    // 切换模式
    ImeIntegration_ToggleInputMode();
    EXPECT_EQ(ImeIntegration_GetInputMode(), 1);  // 英文模式
    
    // 关闭
    ImeIntegration_Shutdown();
}

// 测试 WeaselHandlerExtension
TEST_F(WeaselIntegrationTest, HandlerExtension) {
    auto& ext = WeaselHandlerExtension::Instance();
    
    // 初始化
    ext.OnInitialize(nullptr, nullptr);
    EXPECT_TRUE(ext.IsInitialized());
    
    // 测试输入模式
    EXPECT_EQ(ext.GetInputMode(), 0);  // 中文模式
    
    ext.ToggleInputMode();
    EXPECT_EQ(ext.GetInputMode(), 1);  // 英文模式
    
    // 清理
    ext.OnFinalize();
    EXPECT_FALSE(ext.IsInitialized());
}

// 测试 WeaselTrayExtension
TEST_F(WeaselIntegrationTest, TrayExtension) {
    // 先初始化集成层
    WeaselIntegrationConfig config = WeaselIntegrationConfig::Default();
    ASSERT_TRUE(WeaselIntegration::Instance().Initialize(config));

    auto& tray = WeaselTrayExtension::Instance();
    
    // 初始化
    tray.Initialize();
    EXPECT_TRUE(tray.IsInitialized());
    
    // 测试模式切换
    EXPECT_EQ(tray.GetCurrentMode(), TrayInputMode::Chinese);
    
    tray.SetCurrentMode(TrayInputMode::English);
    EXPECT_EQ(tray.GetCurrentMode(), TrayInputMode::English);
    
    tray.ToggleMode();
    EXPECT_EQ(tray.GetCurrentMode(), TrayInputMode::Chinese);
    
    // 测试提示文本
    auto tooltip = tray.GetTooltipText();
    EXPECT_FALSE(tooltip.empty());
    EXPECT_NE(tooltip.find(L"中文"), std::wstring::npos);
    
    // 测试菜单项
    auto menuItems = tray.GetExtendedMenuItems();
    EXPECT_FALSE(menuItems.empty());
    
    // 测试菜单命令处理
    EXPECT_TRUE(tray.HandleMenuCommand(ID_TRAY_EXT_CHINESE_MODE));
    EXPECT_EQ(tray.GetCurrentMode(), TrayInputMode::Chinese);
    
    EXPECT_TRUE(tray.HandleMenuCommand(ID_TRAY_EXT_ENGLISH_MODE));
    EXPECT_EQ(tray.GetCurrentMode(), TrayInputMode::English);
    
    // 清理
    tray.Shutdown();
    EXPECT_FALSE(tray.IsInitialized());
}

// 测试双击切换
TEST_F(WeaselIntegrationTest, TrayDoubleClick) {
    WeaselIntegrationConfig config = WeaselIntegrationConfig::Default();
    ASSERT_TRUE(WeaselIntegration::Instance().Initialize(config));

    auto& tray = WeaselTrayExtension::Instance();
    TrayExtConfig trayConfig = TrayExtConfig::Default();
    trayConfig.enableQuickSwitch = true;
    trayConfig.doubleClickInterval = 500;
    tray.Initialize(trayConfig);
    
    // 初始状态
    tray.SetCurrentMode(TrayInputMode::Chinese);
    
    // 模拟双击
    DWORD time1 = 1000;
    DWORD time2 = 1200;  // 200ms 后
    
    EXPECT_FALSE(tray.HandleClick(time1));  // 第一次点击
    EXPECT_TRUE(tray.HandleClick(time2));   // 第二次点击（双击）
    
    // 应该切换到英文模式
    EXPECT_EQ(tray.GetCurrentMode(), TrayInputMode::English);
    
    tray.Shutdown();
}

#else  // !PLATFORM_WINDOWS

// 非 Windows 平台的占位测试
TEST(WeaselIntegrationTest, SkippedOnNonWindows) {
    GTEST_SKIP() << "Weasel integration tests only run on Windows";
}

#endif  // PLATFORM_WINDOWS
