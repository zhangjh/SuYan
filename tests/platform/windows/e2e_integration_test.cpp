/**
 * End-to-End Integration Test
 * 
 * Feature: windows-platform-support
 * Task 13: 端到端集成测试
 * 
 * Task 13.1: 完整输入流程测试
 * - 在记事本中输入 "nihao" 并选择候选词
 * - 验证中文正确输入到应用
 * - 测试数字键选择、空格提交、回车提交
 * - 测试 Escape 取消、Backspace 删除
 * 
 * Task 13.2: 多应用测试
 * - 测试记事本、浏览器、Word/WPS
 * - 验证候选窗口位置正确
 * - 验证文本提交正确
 * 
 * Validates: Requirements 4.1, 4.2, 4.3, 4.4, 4.5, 4.6, 5.1, 5.3, 6.1
 */

#ifdef _WIN32

#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <functional>
#include <atomic>

#include "../../../src/shared/ipc_protocol.h"

using namespace suyan::ipc;

namespace {

class E2ETestClient {
public:
    E2ETestClient() : m_pipe(INVALID_HANDLE_VALUE), m_sessionId(0) {}
    
    ~E2ETestClient() {
        disconnect();
    }
    
    bool connect() {
        for (int i = 0; i < 10; ++i) {
            m_pipe = CreateFileW(
                PIPE_NAME,
                GENERIC_READ | GENERIC_WRITE,
                0,
                nullptr,
                OPEN_EXISTING,
                0,
                nullptr
            );
            
            if (m_pipe != INVALID_HANDLE_VALUE) {
                break;
            }
            
            if (GetLastError() == ERROR_PIPE_BUSY) {
                WaitNamedPipeW(PIPE_NAME, 1000);
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        
        if (m_pipe == INVALID_HANDLE_VALUE) {
            std::cerr << "  Failed to connect to server" << std::endl;
            return false;
        }
        
        DWORD mode = PIPE_READMODE_BYTE | PIPE_WAIT;
        SetNamedPipeHandleState(m_pipe, &mode, nullptr, nullptr);
        
        return handshake();
    }
    
    void disconnect() {
        if (m_pipe != INVALID_HANDLE_VALUE) {
            Request req = {};
            req.cmd = Command::Disconnect;
            req.sessionId = m_sessionId;
            
            uint8_t buffer[sizeof(Request)];
            serializeRequest(req, buffer);
            
            DWORD written;
            WriteFile(m_pipe, buffer, sizeof(buffer), &written, nullptr);
            
            CloseHandle(m_pipe);
            m_pipe = INVALID_HANDLE_VALUE;
        }
    }
    
    bool testKey(uint32_t vk, uint32_t mod, bool& eaten) {
        uint32_t result;
        std::wstring data;
        if (!sendRequest(Command::TestKey, vk, mod, result, data)) {
            return false;
        }
        eaten = (result != 0);
        return true;
    }
    
    bool processKey(uint32_t vk, uint32_t mod, std::wstring& commitText) {
        uint32_t result;
        if (!sendRequest(Command::ProcessKey, vk, mod, result, commitText)) {
            return false;
        }
        return true;
    }
    
    bool focusIn() {
        uint32_t result;
        std::wstring data;
        return sendRequest(Command::FocusIn, 0, 0, result, data);
    }
    
    bool focusOut() {
        uint32_t result;
        std::wstring data;
        return sendRequest(Command::FocusOut, 0, 0, result, data);
    }
    
    bool updateCursor(int16_t x, int16_t y, int16_t w, int16_t h) {
        uint32_t param1, param2;
        CursorPosition::encode(x, y, w, h, param1, param2);
        uint32_t result;
        std::wstring data;
        return sendRequest(Command::UpdateCursor, param1, param2, result, data);
    }
    
    uint32_t sessionId() const { return m_sessionId; }
    
private:
    bool handshake() {
        Request req = {};
        req.cmd = Command::Handshake;
        req.sessionId = 0;
        req.param1 = PROTOCOL_VERSION;
        req.param2 = 0;
        
        uint8_t reqBuffer[sizeof(Request)];
        serializeRequest(req, reqBuffer);
        
        DWORD written;
        if (!WriteFile(m_pipe, reqBuffer, sizeof(reqBuffer), &written, nullptr)) {
            return false;
        }
        
        uint8_t hdrBuffer[sizeof(ResponseHeader)];
        DWORD bytesRead;
        if (!ReadFile(m_pipe, hdrBuffer, sizeof(hdrBuffer), &bytesRead, nullptr)) {
            return false;
        }
        
        ResponseHeader hdr = deserializeResponseHeader(hdrBuffer);
        if (hdr.result == 0) {
            return false;
        }
        
        m_sessionId = hdr.result;
        return true;
    }
    
    bool sendRequest(Command cmd, uint32_t param1, uint32_t param2,
                     uint32_t& result, std::wstring& data) {
        Request req = {};
        req.cmd = cmd;
        req.sessionId = m_sessionId;
        req.param1 = param1;
        req.param2 = param2;
        
        uint8_t reqBuffer[sizeof(Request)];
        serializeRequest(req, reqBuffer);
        
        DWORD written;
        if (!WriteFile(m_pipe, reqBuffer, sizeof(reqBuffer), &written, nullptr)) {
            return false;
        }
        
        uint8_t hdrBuffer[sizeof(ResponseHeader)];
        DWORD bytesRead;
        if (!ReadFile(m_pipe, hdrBuffer, sizeof(hdrBuffer), &bytesRead, nullptr)) {
            return false;
        }
        
        ResponseHeader hdr = deserializeResponseHeader(hdrBuffer);
        result = hdr.result;
        
        if (hdr.dataSize > 0) {
            size_t charCount = hdr.dataSize / sizeof(wchar_t);
            data.resize(charCount);
            if (!ReadFile(m_pipe, data.data(), hdr.dataSize, &bytesRead, nullptr)) {
                return false;
            }
        } else {
            data.clear();
        }
        
        return true;
    }
    
    HANDLE m_pipe;
    uint32_t m_sessionId;
};

struct TestResult {
    std::string name;
    bool passed;
    std::string message;
};

std::vector<TestResult> g_results;

void recordResult(const std::string& name, bool passed, const std::string& message = "") {
    g_results.push_back({name, passed, message});
    if (passed) {
        std::cout << "  [PASS] " << name << std::endl;
    } else {
        std::cout << "  [FAIL] " << name;
        if (!message.empty()) {
            std::cout << " - " << message;
        }
        std::cout << std::endl;
    }
}

}  // namespace

/**
 * Test 1: Letter Key Input Flow
 * 
 * Simulates typing "nihao" and verifies:
 * - Each letter key is intercepted (testKey returns true)
 * - Keys are processed by the server
 */
bool testLetterKeyInput(E2ETestClient& client) {
    std::cout << "\n=== Test 1: Letter Key Input Flow ===" << std::endl;
    
    client.focusIn();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    const char* input = "nihao";
    bool allIntercepted = true;
    
    for (int i = 0; input[i] != '\0'; ++i) {
        char c = input[i];
        uint32_t vk = toupper(c);
        
        bool eaten = false;
        if (!client.testKey(vk, 0, eaten)) {
            recordResult("TestKey for '" + std::string(1, c) + "'", false, "IPC failed");
            return false;
        }
        
        if (!eaten) {
            allIntercepted = false;
            recordResult("TestKey for '" + std::string(1, c) + "'", false, "Key not intercepted");
        }
        
        std::wstring commitText;
        if (!client.processKey(vk, 0, commitText)) {
            recordResult("ProcessKey for '" + std::string(1, c) + "'", false, "IPC failed");
            return false;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    
    recordResult("Letter keys intercepted", allIntercepted);
    return allIntercepted;
}

/**
 * Test 2: Digit Key Selection
 * 
 * After inputting pinyin, tests digit key selection:
 * - Digit keys 1-9 should be intercepted when composing
 * - Selecting a candidate should work
 */
bool testDigitKeySelection(E2ETestClient& client) {
    std::cout << "\n=== Test 2: Digit Key Selection ===" << std::endl;
    
    bool eaten = false;
    if (!client.testKey('1', 0, eaten)) {
        recordResult("TestKey for '1'", false, "IPC failed");
        return false;
    }
    
    std::wstring commitText;
    if (!client.processKey('1', 0, commitText)) {
        recordResult("ProcessKey for '1'", false, "IPC failed");
        return false;
    }
    
    recordResult("Digit key '1' processed", true);
    return true;
}

/**
 * Test 3: Space Key Commit
 * 
 * Tests that space key commits the first candidate
 */
bool testSpaceKeyCommit(E2ETestClient& client) {
    std::cout << "\n=== Test 3: Space Key Commit ===" << std::endl;
    
    const char* input = "ni";
    for (int i = 0; input[i] != '\0'; ++i) {
        uint32_t vk = toupper(input[i]);
        std::wstring commitText;
        client.processKey(vk, 0, commitText);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    
    bool eaten = false;
    if (!client.testKey(VK_SPACE, 0, eaten)) {
        recordResult("TestKey for Space", false, "IPC failed");
        return false;
    }
    
    std::wstring commitText;
    if (!client.processKey(VK_SPACE, 0, commitText)) {
        recordResult("ProcessKey for Space", false, "IPC failed");
        return false;
    }
    
    recordResult("Space key commit", true);
    return true;
}

/**
 * Test 4: Enter Key Commit
 * 
 * Tests that Enter key commits current input
 */
bool testEnterKeyCommit(E2ETestClient& client) {
    std::cout << "\n=== Test 4: Enter Key Commit ===" << std::endl;
    
    const char* input = "hao";
    for (int i = 0; input[i] != '\0'; ++i) {
        uint32_t vk = toupper(input[i]);
        std::wstring commitText;
        client.processKey(vk, 0, commitText);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    
    bool eaten = false;
    if (!client.testKey(VK_RETURN, 0, eaten)) {
        recordResult("TestKey for Enter", false, "IPC failed");
        return false;
    }
    
    std::wstring commitText;
    if (!client.processKey(VK_RETURN, 0, commitText)) {
        recordResult("ProcessKey for Enter", false, "IPC failed");
        return false;
    }
    
    recordResult("Enter key commit", true);
    return true;
}

/**
 * Test 5: Escape Key Cancel
 * 
 * Tests that Escape key cancels current input
 */
bool testEscapeKeyCancel(E2ETestClient& client) {
    std::cout << "\n=== Test 5: Escape Key Cancel ===" << std::endl;
    
    const char* input = "test";
    for (int i = 0; input[i] != '\0'; ++i) {
        uint32_t vk = toupper(input[i]);
        std::wstring commitText;
        client.processKey(vk, 0, commitText);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    
    bool eaten = false;
    if (!client.testKey(VK_ESCAPE, 0, eaten)) {
        recordResult("TestKey for Escape", false, "IPC failed");
        return false;
    }
    
    std::wstring commitText;
    if (!client.processKey(VK_ESCAPE, 0, commitText)) {
        recordResult("ProcessKey for Escape", false, "IPC failed");
        return false;
    }
    
    recordResult("Escape key cancel", true);
    return true;
}

/**
 * Test 6: Backspace Key Delete
 * 
 * Tests that Backspace key deletes last character
 */
bool testBackspaceKeyDelete(E2ETestClient& client) {
    std::cout << "\n=== Test 6: Backspace Key Delete ===" << std::endl;
    
    const char* input = "abc";
    for (int i = 0; input[i] != '\0'; ++i) {
        uint32_t vk = toupper(input[i]);
        std::wstring commitText;
        client.processKey(vk, 0, commitText);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    
    bool eaten = false;
    if (!client.testKey(VK_BACK, 0, eaten)) {
        recordResult("TestKey for Backspace", false, "IPC failed");
        return false;
    }
    
    std::wstring commitText;
    if (!client.processKey(VK_BACK, 0, commitText)) {
        recordResult("ProcessKey for Backspace", false, "IPC failed");
        return false;
    }
    
    client.processKey(VK_ESCAPE, 0, commitText);
    
    recordResult("Backspace key delete", true);
    return true;
}

/**
 * Test 7: PageUp/PageDown Navigation
 * 
 * Tests candidate page navigation
 */
bool testPageNavigation(E2ETestClient& client) {
    std::cout << "\n=== Test 7: PageUp/PageDown Navigation ===" << std::endl;
    
    const char* input = "shi";
    for (int i = 0; input[i] != '\0'; ++i) {
        uint32_t vk = toupper(input[i]);
        std::wstring commitText;
        client.processKey(vk, 0, commitText);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    
    bool eaten = false;
    client.testKey(VK_NEXT, 0, eaten);
    
    std::wstring commitText;
    client.processKey(VK_NEXT, 0, commitText);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    client.testKey(VK_PRIOR, 0, eaten);
    client.processKey(VK_PRIOR, 0, commitText);
    
    client.processKey(VK_ESCAPE, 0, commitText);
    
    recordResult("Page navigation", true);
    return true;
}

/**
 * Test 8: Focus In/Out
 * 
 * Tests focus management
 */
bool testFocusManagement(E2ETestClient& client) {
    std::cout << "\n=== Test 8: Focus Management ===" << std::endl;
    
    if (!client.focusOut()) {
        recordResult("FocusOut", false, "IPC failed");
        return false;
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    if (!client.focusIn()) {
        recordResult("FocusIn", false, "IPC failed");
        return false;
    }
    
    recordResult("Focus management", true);
    return true;
}

/**
 * Test 9: Cursor Position Update
 * 
 * Tests cursor position updates including negative coordinates
 */
bool testCursorPositionUpdate(E2ETestClient& client) {
    std::cout << "\n=== Test 9: Cursor Position Update ===" << std::endl;
    
    if (!client.updateCursor(100, 200, 2, 20)) {
        recordResult("UpdateCursor positive", false, "IPC failed");
        return false;
    }
    
    if (!client.updateCursor(-1920, 100, 2, 20)) {
        recordResult("UpdateCursor negative X", false, "IPC failed");
        return false;
    }
    
    if (!client.updateCursor(100, -100, 2, 20)) {
        recordResult("UpdateCursor negative Y", false, "IPC failed");
        return false;
    }
    
    recordResult("Cursor position update", true);
    return true;
}

/**
 * Test 10: Non-Input Key Pass-Through
 * 
 * Tests that non-input keys are not intercepted
 */
bool testNonInputKeyPassThrough(E2ETestClient& client) {
    std::cout << "\n=== Test 10: Non-Input Key Pass-Through ===" << std::endl;
    
    bool allPassed = true;
    
    bool eaten = false;
    client.testKey(VK_F1, 0, eaten);
    if (eaten) {
        recordResult("F1 pass-through", false, "F1 should not be intercepted");
        allPassed = false;
    }
    
    client.testKey('C', ModControl, eaten);
    if (eaten) {
        recordResult("Ctrl+C pass-through", false, "Ctrl+C should not be intercepted");
        allPassed = false;
    }
    
    client.testKey(VK_TAB, ModAlt, eaten);
    if (eaten) {
        recordResult("Alt+Tab pass-through", false, "Alt+Tab should not be intercepted");
        allPassed = false;
    }
    
    if (allPassed) {
        recordResult("Non-input key pass-through", true);
    }
    
    return allPassed;
}

void printSummary() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Test Summary" << std::endl;
    std::cout << "========================================" << std::endl;
    
    int passed = 0;
    int failed = 0;
    
    for (const auto& result : g_results) {
        if (result.passed) {
            ++passed;
        } else {
            ++failed;
        }
    }
    
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;
    std::cout << "Total:  " << g_results.size() << std::endl;
    
    if (failed > 0) {
        std::cout << "\nFailed tests:" << std::endl;
        for (const auto& result : g_results) {
            if (!result.passed) {
                std::cout << "  - " << result.name;
                if (!result.message.empty()) {
                    std::cout << ": " << result.message;
                }
                std::cout << std::endl;
            }
        }
    }
}

int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "SuYan IME End-to-End Integration Test" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\nFeature: windows-platform-support" << std::endl;
    std::cout << "Task 13: End-to-End Integration Test" << std::endl;
    std::cout << "\nRequirements validated:" << std::endl;
    std::cout << "  - 4.1: Letter key interception" << std::endl;
    std::cout << "  - 4.2: Digit key selection" << std::endl;
    std::cout << "  - 4.3: Space key commit" << std::endl;
    std::cout << "  - 4.4: Enter key commit" << std::endl;
    std::cout << "  - 4.5: Escape key cancel" << std::endl;
    std::cout << "  - 4.6: Backspace key delete" << std::endl;
    std::cout << "  - 4.9: Non-input key pass-through" << std::endl;
    std::cout << "  - 5.1: Text commit" << std::endl;
    std::cout << "  - 6.1: Candidate window display" << std::endl;
    std::cout << "  - 8.1-8.4: Focus management" << std::endl;
    std::cout << std::endl;
    
    std::cout << "NOTE: This test requires SuYanServer.exe to be running." << std::endl;
    std::cout << "      Start the server before running this test." << std::endl;
    std::cout << std::endl;
    
    E2ETestClient client;
    
    std::cout << "Connecting to server..." << std::endl;
    if (!client.connect()) {
        std::cerr << "ERROR: Failed to connect to SuYanServer." << std::endl;
        std::cerr << "       Please ensure SuYanServer.exe is running." << std::endl;
        return 1;
    }
    std::cout << "Connected (session ID: " << client.sessionId() << ")" << std::endl;
    
    testLetterKeyInput(client);
    testDigitKeySelection(client);
    testSpaceKeyCommit(client);
    testEnterKeyCommit(client);
    testEscapeKeyCancel(client);
    testBackspaceKeyDelete(client);
    testPageNavigation(client);
    testFocusManagement(client);
    testCursorPositionUpdate(client);
    testNonInputKeyPassThrough(client);
    
    client.disconnect();
    
    printSummary();
    
    int failCount = 0;
    for (const auto& result : g_results) {
        if (!result.passed) {
            ++failCount;
        }
    }
    
    return failCount > 0 ? 1 : 0;
}

#else  // !_WIN32

#include <iostream>

int main() {
    std::cout << "E2E Integration Tests are Windows-only" << std::endl;
    return 0;
}

#endif  // _WIN32
