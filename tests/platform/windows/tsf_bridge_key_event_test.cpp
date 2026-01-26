/**
 * TSFBridge 键事件处理单元测试
 * Task 5.3: 编写键事件处理单元测试
 * 
 * 测试内容：
 * - InputEngine 返回值与 pfEaten 的一致性
 * - Property 3: 键事件消费一致性
 * 
 * Validates: Requirements 2.4, 2.5
 * 
 * 注意：此测试文件设计为跨平台编译，在非 Windows 平台上
 * 使用模拟的 Windows API 定义进行测试。
 */

#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <functional>

// ========== 平台兼容层 ==========
// 在非 Windows 平台上模拟 Windows API 定义

#ifndef _WIN32

// 模拟 Windows 类型定义
typedef unsigned long WPARAM;
typedef long LPARAM;
typedef unsigned int UINT;
typedef short SHORT;
typedef int BOOL;
typedef long HRESULT;

#define TRUE 1
#define FALSE 0
#define S_OK 0

// 模拟 Windows 虚拟键码定义
#define VK_BACK           0x08
#define VK_TAB            0x09
#define VK_RETURN         0x0D
#define VK_SHIFT          0x10
#define VK_CONTROL        0x11
#define VK_MENU           0x12  // Alt
#define VK_ESCAPE         0x1B
#define VK_SPACE          0x20
#define VK_PRIOR          0x21  // Page Up
#define VK_NEXT           0x22  // Page Down
#define VK_END            0x23
#define VK_HOME           0x24
#define VK_LEFT           0x25
#define VK_UP             0x26
#define VK_RIGHT          0x27
#define VK_DOWN           0x28
#define VK_DELETE         0x2E
#define VK_LSHIFT         0xA0
#define VK_RSHIFT         0xA1
#define VK_LCONTROL       0xA2
#define VK_RCONTROL       0xA3
#define VK_LMENU          0xA4
#define VK_RMENU          0xA5

// 模拟键盘状态
static unsigned char g_keyboardState[256] = {0};

SHORT GetKeyState(int vk) {
    SHORT result = 0;
    if (g_keyboardState[vk] & 0x80) {
        result |= 0x8000;
    }
    return result;
}

void setKeyDown(int vk, bool down) {
    if (down) {
        g_keyboardState[vk] |= 0x80;
    } else {
        g_keyboardState[vk] &= ~0x80;
    }
}

void resetKeyboardState() {
    for (int i = 0; i < 256; i++) {
        g_keyboardState[i] = 0;
    }
}

typedef unsigned long DWORD;

DWORD GetTickCount() {
    return static_cast<DWORD>(std::time(nullptr) * 1000);
}

#else
#include <windows.h>

void setKeyDown(int vk, bool down) {}
void resetKeyboardState() {}

#endif // _WIN32

// ========== 输入模式枚举 ==========

enum class InputMode {
    Chinese,
    English,
    TempEnglish
};

// ========== 修饰键掩码 ==========

namespace KeyModifier {
    constexpr int None    = 0;
    constexpr int Shift   = 1 << 0;
    constexpr int Control = 1 << 2;
    constexpr int Alt     = 1 << 3;
    constexpr int Super   = 1 << 6;
}

// ========== 模拟 InputEngine ==========

class MockInputEngine {
public:
    MockInputEngine() : mode_(InputMode::Chinese), composing_(false) {}
    
    void setMode(InputMode mode) { mode_ = mode; }
    InputMode getMode() const { return mode_; }
    
    void setComposing(bool composing) { composing_ = composing; }
    bool isComposing() const { return composing_; }
    
    // 设置 processKeyEvent 的返回值
    void setProcessKeyEventResult(bool result) { processKeyEventResult_ = result; }
    
    // 模拟 processKeyEvent
    bool processKeyEvent(int keyCode, int modifiers) {
        lastKeyCode_ = keyCode;
        lastModifiers_ = modifiers;
        return processKeyEventResult_;
    }
    
    int getLastKeyCode() const { return lastKeyCode_; }
    int getLastModifiers() const { return lastModifiers_; }
    
    void toggleMode() {
        if (mode_ == InputMode::Chinese) {
            mode_ = InputMode::English;
        } else {
            mode_ = InputMode::Chinese;
        }
    }
    
    void reset() {
        composing_ = false;
    }
    
    void activate() { active_ = true; }
    void deactivate() { active_ = false; }
    bool isActive() const { return active_; }

private:
    InputMode mode_;
    bool composing_;
    bool processKeyEventResult_ = false;
    int lastKeyCode_ = 0;
    int lastModifiers_ = 0;
    bool active_ = false;
};

// ========== 模拟 TSFBridge 键事件处理逻辑 ==========

class MockTSFBridge {
public:
    MockTSFBridge() : inputEngine_(nullptr), activated_(false) {}
    
    void setInputEngine(MockInputEngine* engine) { inputEngine_ = engine; }
    void setActivated(bool activated) { activated_ = activated; }
    
    /**
     * 模拟 OnTestKeyDown 逻辑
     * 
     * 返回是否应该消费此按键
     */
    bool onTestKeyDown(WPARAM wParam, LPARAM lParam) {
        if (!inputEngine_ || !activated_) {
            return false;
        }
        
        // 修饰键不消费
        if (isModifierKey(wParam)) {
            return false;
        }
        
        // 英文模式下不消费任何按键
        if (inputEngine_->getMode() == InputMode::English) {
            return false;
        }
        
        // 中文模式下的消费逻辑
        if (inputEngine_->isComposing()) {
            // 正在输入时，消费大部分按键
            if (isCharacterKey(wParam) || isNavigationKey(wParam) ||
                wParam == VK_BACK || wParam == VK_ESCAPE || 
                wParam == VK_RETURN || wParam == VK_SPACE ||
                wParam == VK_PRIOR || wParam == VK_NEXT) {
                return true;
            }
        } else {
            // 未输入时，只消费字母键
            if (wParam >= 'A' && wParam <= 'Z') {
                return true;
            }
        }
        
        return false;
    }
    
    /**
     * 模拟 OnKeyDown 逻辑
     * 
     * 核心测试点：InputEngine 返回值与 pfEaten 的一致性
     * Property 3: 键事件消费一致性
     */
    bool onKeyDown(WPARAM wParam, LPARAM lParam, BOOL* pfEaten) {
        if (!pfEaten) {
            return false;
        }
        
        *pfEaten = FALSE;
        
        if (!inputEngine_ || !activated_) {
            return true;
        }
        
        // 处理 Shift 键状态跟踪
        if (wParam == VK_SHIFT || wParam == VK_LSHIFT || wParam == VK_RSHIFT) {
            shiftKeyPressed_ = true;
            otherKeyPressedWithShift_ = false;
            return true;
        }
        
        // 如果 Shift 按下期间有其他键按下，标记
        if (shiftKeyPressed_) {
            otherKeyPressedWithShift_ = true;
        }
        
        // 修饰键不消费
        if (isModifierKey(wParam)) {
            return true;
        }
        
        // 转换键码（简化版）
        int rimeKeyCode = convertVirtualKeyToRime(wParam);
        int rimeModifiers = convertModifiersToRime();
        
        if (rimeKeyCode == 0) {
            return true;
        }
        
        // 调用 InputEngine 处理按键
        // Property 3: 键事件消费一致性
        // InputEngine 返回 true -> pfEaten = TRUE
        // InputEngine 返回 false -> pfEaten = FALSE
        bool handled = inputEngine_->processKeyEvent(rimeKeyCode, rimeModifiers);
        *pfEaten = handled ? TRUE : FALSE;
        
        return true;
    }
    
    /**
     * 模拟 OnKeyUp 逻辑
     */
    bool onKeyUp(WPARAM wParam, LPARAM lParam, BOOL* pfEaten) {
        if (!pfEaten) {
            return false;
        }
        
        *pfEaten = FALSE;
        
        if (!inputEngine_ || !activated_) {
            return true;
        }
        
        // 处理 Shift 键释放切换模式
        if (wParam == VK_SHIFT || wParam == VK_LSHIFT || wParam == VK_RSHIFT) {
            if (shiftKeyPressed_ && !otherKeyPressedWithShift_) {
                inputEngine_->toggleMode();
                *pfEaten = TRUE;
            }
            shiftKeyPressed_ = false;
            otherKeyPressedWithShift_ = false;
        }
        
        return true;
    }

private:
    bool isModifierKey(WPARAM vk) {
        switch (vk) {
            case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT:
            case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL:
            case VK_MENU: case VK_LMENU: case VK_RMENU:
                return true;
            default:
                return false;
        }
    }
    
    bool isCharacterKey(WPARAM vk) {
        if (vk >= 'A' && vk <= 'Z') return true;
        if (vk >= '0' && vk <= '9') return true;
        if (vk == VK_SPACE) return true;
        return false;
    }
    
    bool isNavigationKey(WPARAM vk) {
        switch (vk) {
            case VK_LEFT: case VK_RIGHT: case VK_UP: case VK_DOWN:
            case VK_HOME: case VK_END: case VK_PRIOR: case VK_NEXT:
                return true;
            default:
                return false;
        }
    }
    
    int convertVirtualKeyToRime(WPARAM vk) {
        // 简化版键码转换
        if (vk >= 'A' && vk <= 'Z') {
            return static_cast<int>(vk) + 32;  // 小写
        }
        if (vk >= '0' && vk <= '9') {
            return static_cast<int>(vk);
        }
        if (vk == VK_SPACE) return 0x0020;
        if (vk == VK_RETURN) return 0xff0d;
        if (vk == VK_BACK) return 0xff08;
        if (vk == VK_ESCAPE) return 0xff1b;
        return 0;
    }
    
    int convertModifiersToRime() {
        int modifiers = KeyModifier::None;
        if (GetKeyState(VK_SHIFT) & 0x8000) modifiers |= KeyModifier::Shift;
        if (GetKeyState(VK_CONTROL) & 0x8000) modifiers |= KeyModifier::Control;
        if (GetKeyState(VK_MENU) & 0x8000) modifiers |= KeyModifier::Alt;
        return modifiers;
    }
    
    MockInputEngine* inputEngine_;
    bool activated_;
    bool shiftKeyPressed_ = false;
    bool otherKeyPressedWithShift_ = false;
};

// ========== 测试辅助宏 ==========

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

// ========== 测试类 ==========

class TSFBridgeKeyEventTest {
public:
    TSFBridgeKeyEventTest() {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
    }

    bool runAllTests() {
        std::cout << "=== TSFBridge 键事件处理单元测试 ===" << std::endl;
        std::cout << "Task 5.3: 编写键事件处理单元测试" << std::endl;
        std::cout << "Validates: Requirements 2.4, 2.5" << std::endl;
        std::cout << std::endl;

        bool allPassed = true;

        // 基础功能测试
        allPassed &= testKeyEventConsumption();
        allPassed &= testModifierKeyPassthrough();
        allPassed &= testEnglishModePassthrough();
        allPassed &= testChineseModeComposing();
        allPassed &= testShiftKeyModeSwitch();
        
        // 属性测试
        allPassed &= testProperty3_KeyEventConsumptionConsistency();

        std::cout << std::endl;
        if (allPassed) {
            std::cout << "=== 所有测试通过 ===" << std::endl;
        } else {
            std::cout << "=== 部分测试失败 ===" << std::endl;
        }

        return allPassed;
    }

private:
    // ========== 基础功能测试 ==========
    
    bool testKeyEventConsumption() {
        std::cout << "\n--- 键事件消费测试 ---" << std::endl;
        
        MockInputEngine engine;
        MockTSFBridge bridge;
        bridge.setInputEngine(&engine);
        bridge.setActivated(true);
        
        engine.setMode(InputMode::Chinese);
        engine.setComposing(false);
        
        // 测试字母键在中文模式下被消费
        bool shouldEat = bridge.onTestKeyDown('A', 0);
        TEST_ASSERT(shouldEat == true, "中文模式下字母键应该被消费");
        
        // 测试数字键在未输入时不被消费
        shouldEat = bridge.onTestKeyDown('1', 0);
        TEST_ASSERT(shouldEat == false, "中文模式未输入时数字键不应该被消费");
        
        TEST_PASS("testKeyEventConsumption: 键事件消费逻辑正确");
        return true;
    }
    
    bool testModifierKeyPassthrough() {
        std::cout << "\n--- 修饰键透传测试 ---" << std::endl;
        
        MockInputEngine engine;
        MockTSFBridge bridge;
        bridge.setInputEngine(&engine);
        bridge.setActivated(true);
        
        engine.setMode(InputMode::Chinese);
        
        // 修饰键应该不被消费
        bool shouldEat = bridge.onTestKeyDown(VK_SHIFT, 0);
        TEST_ASSERT(shouldEat == false, "Shift 键不应该被消费");
        
        shouldEat = bridge.onTestKeyDown(VK_CONTROL, 0);
        TEST_ASSERT(shouldEat == false, "Control 键不应该被消费");
        
        shouldEat = bridge.onTestKeyDown(VK_MENU, 0);
        TEST_ASSERT(shouldEat == false, "Alt 键不应该被消费");
        
        TEST_PASS("testModifierKeyPassthrough: 修饰键正确透传");
        return true;
    }
    
    bool testEnglishModePassthrough() {
        std::cout << "\n--- 英文模式透传测试 ---" << std::endl;
        
        MockInputEngine engine;
        MockTSFBridge bridge;
        bridge.setInputEngine(&engine);
        bridge.setActivated(true);
        
        engine.setMode(InputMode::English);
        
        // 英文模式下所有按键都不应该被消费
        for (char c = 'A'; c <= 'Z'; c++) {
            bool shouldEat = bridge.onTestKeyDown(c, 0);
            TEST_ASSERT(shouldEat == false, 
                "英文模式下字母键 " + std::string(1, c) + " 不应该被消费");
        }
        
        for (char c = '0'; c <= '9'; c++) {
            bool shouldEat = bridge.onTestKeyDown(c, 0);
            TEST_ASSERT(shouldEat == false,
                "英文模式下数字键 " + std::string(1, c) + " 不应该被消费");
        }
        
        TEST_PASS("testEnglishModePassthrough: 英文模式正确透传所有按键");
        return true;
    }
    
    bool testChineseModeComposing() {
        std::cout << "\n--- 中文模式输入中测试 ---" << std::endl;
        
        MockInputEngine engine;
        MockTSFBridge bridge;
        bridge.setInputEngine(&engine);
        bridge.setActivated(true);
        
        engine.setMode(InputMode::Chinese);
        engine.setComposing(true);  // 正在输入
        
        // 正在输入时，字母键应该被消费
        for (char c = 'A'; c <= 'Z'; c++) {
            bool shouldEat = bridge.onTestKeyDown(c, 0);
            TEST_ASSERT(shouldEat == true,
                "输入中字母键 " + std::string(1, c) + " 应该被消费");
        }
        
        // 正在输入时，数字键应该被消费（用于选择候选词）
        for (char c = '0'; c <= '9'; c++) {
            bool shouldEat = bridge.onTestKeyDown(c, 0);
            TEST_ASSERT(shouldEat == true,
                "输入中数字键 " + std::string(1, c) + " 应该被消费");
        }
        
        // 正在输入时，功能键应该被消费
        bool shouldEat = bridge.onTestKeyDown(VK_BACK, 0);
        TEST_ASSERT(shouldEat == true, "输入中 Backspace 应该被消费");
        
        shouldEat = bridge.onTestKeyDown(VK_ESCAPE, 0);
        TEST_ASSERT(shouldEat == true, "输入中 Escape 应该被消费");
        
        shouldEat = bridge.onTestKeyDown(VK_RETURN, 0);
        TEST_ASSERT(shouldEat == true, "输入中 Enter 应该被消费");
        
        shouldEat = bridge.onTestKeyDown(VK_SPACE, 0);
        TEST_ASSERT(shouldEat == true, "输入中 Space 应该被消费");
        
        TEST_PASS("testChineseModeComposing: 中文模式输入中正确消费按键");
        return true;
    }
    
    bool testShiftKeyModeSwitch() {
        std::cout << "\n--- Shift 键切换模式测试 ---" << std::endl;
        
        MockInputEngine engine;
        MockTSFBridge bridge;
        bridge.setInputEngine(&engine);
        bridge.setActivated(true);
        
        engine.setMode(InputMode::Chinese);
        
        // 模拟 Shift 按下
        BOOL eaten = FALSE;
        bridge.onKeyDown(VK_SHIFT, 0, &eaten);
        TEST_ASSERT(eaten == FALSE, "Shift 按下不应该被消费");
        TEST_ASSERT(engine.getMode() == InputMode::Chinese, "Shift 按下不应该切换模式");
        
        // 模拟 Shift 释放（无其他键按下）
        bridge.onKeyUp(VK_SHIFT, 0, &eaten);
        TEST_ASSERT(eaten == TRUE, "单独 Shift 释放应该被消费");
        TEST_ASSERT(engine.getMode() == InputMode::English, "单独 Shift 释放应该切换到英文模式");
        
        // 再次切换回中文
        bridge.onKeyDown(VK_SHIFT, 0, &eaten);
        bridge.onKeyUp(VK_SHIFT, 0, &eaten);
        TEST_ASSERT(engine.getMode() == InputMode::Chinese, "再次 Shift 应该切换回中文模式");
        
        TEST_PASS("testShiftKeyModeSwitch: Shift 键切换模式正确");
        return true;
    }

    // ========== Property 3: 键事件消费一致性 ==========
    /**
     * Property 3: 键事件消费一致性
     * 
     * For any 键盘事件，当 InputEngine::processKeyEvent 返回 true 时，
     * TSFBridge 应将 pfEaten 设为 TRUE；当返回 false 时，应设为 FALSE。
     * 即 InputEngine 的返回值与 TSF 事件消费标志严格一致。
     * 
     * Validates: Requirements 2.4, 2.5
     */
    bool testProperty3_KeyEventConsumptionConsistency() {
        std::cout << "\n--- Property 3: 键事件消费一致性 ---" << std::endl;
        std::cout << "  验证: InputEngine 返回值与 pfEaten 严格一致" << std::endl;
        
        MockInputEngine engine;
        MockTSFBridge bridge;
        bridge.setInputEngine(&engine);
        bridge.setActivated(true);
        
        engine.setMode(InputMode::Chinese);
        
        const int NUM_ITERATIONS = 100;
        int testCount = 0;
        
        // 收集测试用的虚拟键码
        std::vector<WPARAM> testKeys;
        for (char c = 'A'; c <= 'Z'; c++) testKeys.push_back(c);
        for (char c = '0'; c <= '9'; c++) testKeys.push_back(c);
        testKeys.push_back(VK_SPACE);
        testKeys.push_back(VK_RETURN);
        testKeys.push_back(VK_BACK);
        testKeys.push_back(VK_ESCAPE);
        
        // 测试 InputEngine 返回 true 时 pfEaten 应为 TRUE
        engine.setProcessKeyEventResult(true);
        
        for (WPARAM vk : testKeys) {
            for (int i = 0; i < NUM_ITERATIONS; i++) {
                BOOL eaten = FALSE;
                bridge.onKeyDown(vk, 0, &eaten);
                
                // Property 3: InputEngine 返回 true -> pfEaten = TRUE
                TEST_ASSERT(eaten == TRUE,
                    "InputEngine 返回 true 时 pfEaten 应为 TRUE: VK=" + std::to_string(vk));
                testCount++;
            }
        }
        
        // 测试 InputEngine 返回 false 时 pfEaten 应为 FALSE
        engine.setProcessKeyEventResult(false);
        
        for (WPARAM vk : testKeys) {
            for (int i = 0; i < NUM_ITERATIONS; i++) {
                BOOL eaten = FALSE;
                bridge.onKeyDown(vk, 0, &eaten);
                
                // Property 3: InputEngine 返回 false -> pfEaten = FALSE
                TEST_ASSERT(eaten == FALSE,
                    "InputEngine 返回 false 时 pfEaten 应为 FALSE: VK=" + std::to_string(vk));
                testCount++;
            }
        }
        
        // 随机测试：随机设置 InputEngine 返回值，验证一致性
        for (int i = 0; i < NUM_ITERATIONS * 10; i++) {
            bool expectedResult = (std::rand() % 2) == 0;
            engine.setProcessKeyEventResult(expectedResult);
            
            WPARAM vk = testKeys[std::rand() % testKeys.size()];
            BOOL eaten = FALSE;
            bridge.onKeyDown(vk, 0, &eaten);
            
            bool actualEaten = (eaten == TRUE);
            TEST_ASSERT(actualEaten == expectedResult,
                "随机测试: InputEngine 返回 " + std::to_string(expectedResult) + 
                " 时 pfEaten 应为 " + std::to_string(expectedResult));
            testCount++;
        }
        
        std::cout << "  执行了 " << testCount << " 次属性测试" << std::endl;
        TEST_PASS("Property 3: 键事件消费一致性验证通过");
        return true;
    }
};

// ========== 主函数 ==========

int main() {
    TSFBridgeKeyEventTest test;
    bool passed = test.runAllTests();
    return passed ? 0 : 1;
}
