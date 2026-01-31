/**
 * IPC Integration Test
 * 
 * Feature: windows-platform-support
 * Task 5: Checkpoint - IPC 通信验证
 * 
 * Verifies:
 * - Server starts and accepts connections
 * - Client connects and performs handshake
 * - Multiple clients can connect and disconnect
 * - IPC messages are correctly transmitted
 */

#ifdef _WIN32

#include "../../../src/core/ipc_server.h"
#include "../../../src/shared/ipc_protocol.h"
#include "../../../src/shared/logger.h"
#include <QCoreApplication>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <set>

using namespace suyan;
using namespace suyan::ipc;

namespace {

std::atomic<int> g_testKeyCount{0};
std::atomic<int> g_processKeyCount{0};
std::atomic<int> g_focusInCount{0};
std::atomic<int> g_focusOutCount{0};
std::atomic<int> g_updateCursorCount{0};

uint32_t testHandler(uint32_t sessionId, uint32_t cmd, uint32_t param1, 
                     uint32_t param2, std::wstring& response) {
    Command command = static_cast<Command>(cmd);
    
    switch (command) {
        case Command::TestKey:
            ++g_testKeyCount;
            // Only intercept letter keys (A-Z virtual key codes are 0x41-0x5A)
            return (param1 >= 0x41 && param1 <= 0x5A) ? 1 : 0;
            
        case Command::ProcessKey:
            ++g_processKeyCount;
            response = L"测试";
            return 1;
            
        case Command::FocusIn:
            ++g_focusInCount;
            return 1;
            
        case Command::FocusOut:
            ++g_focusOutCount;
            return 1;
            
        case Command::UpdateCursor:
            ++g_updateCursorCount;
            return 1;
            
        default:
            return 0;
    }
}

void resetCounters() {
    g_testKeyCount = 0;
    g_processKeyCount = 0;
    g_focusInCount = 0;
    g_focusOutCount = 0;
    g_updateCursorCount = 0;
}

class TestClient {
public:
    TestClient() : m_pipe(INVALID_HANDLE_VALUE), m_sessionId(0) {}
    
    ~TestClient() {
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
            std::cerr << "  Failed to connect to pipe, error=" << GetLastError() << std::endl;
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
    
    bool sendTestKey(uint32_t vk, uint32_t mod, uint32_t& result) {
        return sendRequest(Command::TestKey, vk, mod, result);
    }
    
    bool sendProcessKey(uint32_t vk, uint32_t mod, uint32_t& result, std::wstring& data) {
        return sendRequestWithData(Command::ProcessKey, vk, mod, result, data);
    }
    
    bool sendFocusIn(uint32_t& result) {
        return sendRequest(Command::FocusIn, 0, 0, result);
    }
    
    bool sendFocusOut(uint32_t& result) {
        return sendRequest(Command::FocusOut, 0, 0, result);
    }
    
    bool sendUpdateCursor(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t& result) {
        uint32_t param1, param2;
        CursorPosition::encode(x, y, w, h, param1, param2);
        return sendRequest(Command::UpdateCursor, param1, param2, result);
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
            std::cerr << "  Handshake write failed, error=" << GetLastError() << std::endl;
            return false;
        }
        
        uint8_t hdrBuffer[sizeof(ResponseHeader)];
        DWORD bytesRead;
        if (!ReadFile(m_pipe, hdrBuffer, sizeof(hdrBuffer), &bytesRead, nullptr)) {
            std::cerr << "  Handshake read failed, error=" << GetLastError() << std::endl;
            return false;
        }
        
        ResponseHeader hdr = deserializeResponseHeader(hdrBuffer);
        if (hdr.result == 0) {
            std::cerr << "  Handshake rejected by server" << std::endl;
            return false;
        }
        
        m_sessionId = hdr.result;
        return true;
    }
    
    bool sendRequest(Command cmd, uint32_t param1, uint32_t param2, uint32_t& result) {
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
        return true;
    }
    
    bool sendRequestWithData(Command cmd, uint32_t param1, uint32_t param2, 
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
        }
        
        return true;
    }
    
    HANDLE m_pipe;
    uint32_t m_sessionId;
};

}  // namespace

bool testServerStartStop() {
    std::cout << "Test 1: Server Start/Stop... ";
    
    IPCServer server;
    server.setHandler(testHandler);
    
    if (!server.start(2)) {
        std::cout << "FAILED (start failed)" << std::endl;
        return false;
    }
    
    if (!server.isRunning()) {
        std::cout << "FAILED (not running after start)" << std::endl;
        return false;
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    server.stop();
    
    if (server.isRunning()) {
        std::cout << "FAILED (still running after stop)" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

bool testSingleClientConnection() {
    std::cout << "Test 2: Single Client Connection... ";
    
    IPCServer server;
    server.setHandler(testHandler);
    
    if (!server.start(2)) {
        std::cout << "FAILED (server start failed)" << std::endl;
        return false;
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    TestClient client;
    if (!client.connect()) {
        std::cout << "FAILED (client connect failed)" << std::endl;
        server.stop();
        return false;
    }
    
    if (client.sessionId() == 0) {
        std::cout << "FAILED (invalid session ID)" << std::endl;
        server.stop();
        return false;
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    if (server.clientCount() != 1) {
        std::cout << "FAILED (expected 1 client, got " << server.clientCount() << ")" << std::endl;
        server.stop();
        return false;
    }
    
    client.disconnect();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    server.stop();
    
    std::cout << "PASSED (sessionId=" << client.sessionId() << ")" << std::endl;
    return true;
}

bool testMultipleClients() {
    std::cout << "Test 3: Multiple Clients... ";
    
    IPCServer server;
    server.setHandler(testHandler);
    
    if (!server.start(4)) {
        std::cout << "FAILED (server start failed)" << std::endl;
        return false;
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::vector<std::unique_ptr<TestClient>> clients;
    const int NUM_CLIENTS = 3;
    
    for (int i = 0; i < NUM_CLIENTS; ++i) {
        auto client = std::make_unique<TestClient>();
        if (!client->connect()) {
            std::cout << "FAILED (client " << i << " connect failed)" << std::endl;
            server.stop();
            return false;
        }
        clients.push_back(std::move(client));
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    if (server.clientCount() != NUM_CLIENTS) {
        std::cout << "FAILED (expected " << NUM_CLIENTS << " clients, got " 
                  << server.clientCount() << ")" << std::endl;
        server.stop();
        return false;
    }
    
    std::set<uint32_t> sessionIds;
    for (const auto& client : clients) {
        sessionIds.insert(client->sessionId());
    }
    
    if (sessionIds.size() != NUM_CLIENTS) {
        std::cout << "FAILED (duplicate session IDs)" << std::endl;
        server.stop();
        return false;
    }
    
    clients[1]->disconnect();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    clients.clear();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    server.stop();
    
    std::cout << "PASSED" << std::endl;
    return true;
}

bool testKeyCommands() {
    std::cout << "Test 4: Key Commands... ";
    
    resetCounters();
    
    IPCServer server;
    server.setHandler(testHandler);
    
    if (!server.start(2)) {
        std::cout << "FAILED (server start failed)" << std::endl;
        return false;
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    TestClient client;
    if (!client.connect()) {
        std::cout << "FAILED (client connect failed)" << std::endl;
        server.stop();
        return false;
    }
    
    uint32_t result;
    
    if (!client.sendTestKey('A', 0, result) || result != 1) {
        std::cout << "FAILED (TestKey 'A' should return 1)" << std::endl;
        server.stop();
        return false;
    }
    
    if (!client.sendTestKey(VK_F1, 0, result) || result != 0) {
        std::cout << "FAILED (TestKey F1 should return 0)" << std::endl;
        server.stop();
        return false;
    }
    
    std::wstring commitText;
    if (!client.sendProcessKey('n', 0, result, commitText) || result != 1) {
        std::cout << "FAILED (ProcessKey failed)" << std::endl;
        server.stop();
        return false;
    }
    
    if (commitText != L"测试") {
        std::cout << "FAILED (ProcessKey returned wrong text)" << std::endl;
        server.stop();
        return false;
    }
    
    client.disconnect();
    server.stop();
    
    if (g_testKeyCount != 2 || g_processKeyCount != 1) {
        std::cout << "FAILED (wrong command counts)" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

bool testFocusCommands() {
    std::cout << "Test 5: Focus Commands... ";
    
    resetCounters();
    
    IPCServer server;
    server.setHandler(testHandler);
    
    if (!server.start(2)) {
        std::cout << "FAILED (server start failed)" << std::endl;
        return false;
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    TestClient client;
    if (!client.connect()) {
        std::cout << "FAILED (client connect failed)" << std::endl;
        server.stop();
        return false;
    }
    
    uint32_t result;
    
    if (!client.sendFocusIn(result) || result != 1) {
        std::cout << "FAILED (FocusIn failed)" << std::endl;
        server.stop();
        return false;
    }
    
    if (!client.sendFocusOut(result) || result != 1) {
        std::cout << "FAILED (FocusOut failed)" << std::endl;
        server.stop();
        return false;
    }
    
    client.disconnect();
    server.stop();
    
    if (g_focusInCount != 1 || g_focusOutCount != 1) {
        std::cout << "FAILED (wrong focus counts)" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

bool testCursorUpdate() {
    std::cout << "Test 6: Cursor Update... ";
    
    resetCounters();
    
    IPCServer server;
    server.setHandler(testHandler);
    
    if (!server.start(2)) {
        std::cout << "FAILED (server start failed)" << std::endl;
        return false;
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    TestClient client;
    if (!client.connect()) {
        std::cout << "FAILED (client connect failed)" << std::endl;
        server.stop();
        return false;
    }
    
    uint32_t result;
    
    if (!client.sendUpdateCursor(100, 200, 2, 20, result) || result != 1) {
        std::cout << "FAILED (UpdateCursor positive coords failed)" << std::endl;
        server.stop();
        return false;
    }
    
    if (!client.sendUpdateCursor(-1920, -100, 2, 20, result) || result != 1) {
        std::cout << "FAILED (UpdateCursor negative coords failed)" << std::endl;
        server.stop();
        return false;
    }
    
    client.disconnect();
    server.stop();
    
    if (g_updateCursorCount != 2) {
        std::cout << "FAILED (wrong cursor update count)" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

bool testSingleInstanceLock() {
    std::cout << "Test 7: Single Instance Lock... ";
    
    SingleInstanceLock lock1;
    SingleInstanceLock lock2;
    
    if (!lock1.tryAcquire()) {
        std::cout << "FAILED (first lock should succeed)" << std::endl;
        return false;
    }
    
    if (lock2.tryAcquire()) {
        std::cout << "FAILED (second lock should fail)" << std::endl;
        lock1.release();
        return false;
    }
    
    lock1.release();
    
    if (!lock2.tryAcquire()) {
        std::cout << "FAILED (lock should succeed after release)" << std::endl;
        return false;
    }
    
    lock2.release();
    
    std::cout << "PASSED" << std::endl;
    return true;
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    
    suyan::log::initialize(L"ipc_test");
    
    std::cout << "Starting IPC Integration Tests..." << std::endl;
    std::cout.flush();
    
    std::cout << "=== IPC Integration Tests ===" << std::endl;
    std::cout << "Feature: windows-platform-support" << std::endl;
    std::cout << "Task 5: Checkpoint - IPC Communication Verification" << std::endl;
    std::cout << std::endl;
    std::cout.flush();
    
    int passed = 0;
    int failed = 0;
    
    if (testServerStartStop()) ++passed; else ++failed;
    if (testSingleClientConnection()) ++passed; else ++failed;
    if (testMultipleClients()) ++passed; else ++failed;
    if (testKeyCommands()) ++passed; else ++failed;
    if (testFocusCommands()) ++passed; else ++failed;
    if (testCursorUpdate()) ++passed; else ++failed;
    if (testSingleInstanceLock()) ++passed; else ++failed;
    
    std::cout << std::endl;
    std::cout << "Results: " << passed << " passed, " << failed << " failed" << std::endl;
    
    suyan::log::shutdown();
    
    return failed > 0 ? 1 : 0;
}

#else  // !_WIN32

#include <iostream>

int main() {
    std::cout << "IPC Integration Tests are Windows-only" << std::endl;
    return 0;
}

#endif  // _WIN32
