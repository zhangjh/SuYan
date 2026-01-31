/**
 * Multi-Application Test Helper
 * 
 * Feature: windows-platform-support
 * Task 13.2: 多应用测试
 * 
 * This helper tool assists with manual testing by:
 * - Simulating cursor position updates at various screen locations
 * - Testing candidate window positioning logic
 * - Verifying text commit functionality
 * 
 * Validates: Requirements 5.1, 6.1
 */

#ifdef _WIN32

#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

#include "../../../src/shared/ipc_protocol.h"

using namespace suyan::ipc;

namespace {

class TestHelper {
public:
    TestHelper() : m_pipe(INVALID_HANDLE_VALUE), m_sessionId(0) {}
    
    ~TestHelper() {
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
    
    bool focusIn() {
        return sendSimpleRequest(Command::FocusIn, 0, 0);
    }
    
    bool focusOut() {
        return sendSimpleRequest(Command::FocusOut, 0, 0);
    }
    
    bool updateCursor(int16_t x, int16_t y, int16_t w, int16_t h) {
        uint32_t param1, param2;
        CursorPosition::encode(x, y, w, h, param1, param2);
        return sendSimpleRequest(Command::UpdateCursor, param1, param2);
    }
    
    bool processKey(uint32_t vk, uint32_t mod) {
        return sendSimpleRequest(Command::ProcessKey, vk, mod);
    }
    
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
    
    bool sendSimpleRequest(Command cmd, uint32_t param1, uint32_t param2) {
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
        
        if (hdr.dataSize > 0) {
            std::vector<uint8_t> data(hdr.dataSize);
            ReadFile(m_pipe, data.data(), hdr.dataSize, &bytesRead, nullptr);
        }
        
        return true;
    }
    
    HANDLE m_pipe;
    uint32_t m_sessionId;
};

void printMonitorInfo() {
    std::cout << "\n=== Monitor Information ===" << std::endl;
    
    int monitorCount = GetSystemMetrics(SM_CMONITORS);
    std::cout << "Number of monitors: " << monitorCount << std::endl;
    
    int virtualWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int virtualHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    int virtualX = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int virtualY = GetSystemMetrics(SM_YVIRTUALSCREEN);
    
    std::cout << "Virtual screen: (" << virtualX << ", " << virtualY << ") "
              << virtualWidth << "x" << virtualHeight << std::endl;
    
    EnumDisplayMonitors(nullptr, nullptr, [](HMONITOR hMonitor, HDC, LPRECT, LPARAM) -> BOOL {
        MONITORINFO mi = {};
        mi.cbSize = sizeof(mi);
        GetMonitorInfo(hMonitor, &mi);
        
        std::cout << "  Monitor: (" << mi.rcMonitor.left << ", " << mi.rcMonitor.top << ") "
                  << (mi.rcMonitor.right - mi.rcMonitor.left) << "x"
                  << (mi.rcMonitor.bottom - mi.rcMonitor.top);
        
        if (mi.dwFlags & MONITORINFOF_PRIMARY) {
            std::cout << " [PRIMARY]";
        }
        std::cout << std::endl;
        
        return TRUE;
    }, 0);
}

void testCursorPositions(TestHelper& helper) {
    std::cout << "\n=== Testing Cursor Positions ===" << std::endl;
    
    helper.focusIn();
    
    const char* input = "ni";
    for (int i = 0; input[i] != '\0'; ++i) {
        helper.processKey(toupper(input[i]), 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    struct TestPosition {
        int16_t x, y;
        const char* description;
    };
    
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int virtualX = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int virtualY = GetSystemMetrics(SM_YVIRTUALSCREEN);
    
    std::vector<TestPosition> positions = {
        {100, 100, "Top-left area"},
        {static_cast<int16_t>(screenWidth / 2), static_cast<int16_t>(screenHeight / 2), "Center"},
        {static_cast<int16_t>(screenWidth - 100), 100, "Top-right area"},
        {100, static_cast<int16_t>(screenHeight - 100), "Bottom-left area"},
        {static_cast<int16_t>(screenWidth - 100), static_cast<int16_t>(screenHeight - 100), "Bottom-right area"},
    };
    
    if (virtualX < 0) {
        positions.push_back({static_cast<int16_t>(virtualX + 100), 100, "Extended monitor (left)"});
    }
    if (virtualY < 0) {
        positions.push_back({100, static_cast<int16_t>(virtualY + 100), "Extended monitor (top)"});
    }
    
    for (const auto& pos : positions) {
        std::cout << "Testing position: " << pos.description 
                  << " (" << pos.x << ", " << pos.y << ")" << std::endl;
        
        helper.updateCursor(pos.x, pos.y, 2, 20);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        std::cout << "  Press Enter to continue...";
        std::cin.get();
    }
    
    helper.processKey(VK_ESCAPE, 0);
    helper.focusOut();
}

void interactiveMode(TestHelper& helper) {
    std::cout << "\n=== Interactive Mode ===" << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  f - Focus in" << std::endl;
    std::cout << "  u - Focus out" << std::endl;
    std::cout << "  c <x> <y> - Update cursor position" << std::endl;
    std::cout << "  t <text> - Type text (letters only)" << std::endl;
    std::cout << "  s - Space (commit)" << std::endl;
    std::cout << "  e - Escape (cancel)" << std::endl;
    std::cout << "  q - Quit" << std::endl;
    std::cout << std::endl;
    
    std::string line;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, line);
        
        if (line.empty()) continue;
        
        char cmd = line[0];
        
        if (cmd == 'q' || cmd == 'Q') {
            break;
        } else if (cmd == 'f' || cmd == 'F') {
            helper.focusIn();
            std::cout << "Focus in" << std::endl;
        } else if (cmd == 'u' || cmd == 'U') {
            helper.focusOut();
            std::cout << "Focus out" << std::endl;
        } else if (cmd == 'c' || cmd == 'C') {
            int x, y;
            if (sscanf(line.c_str() + 1, "%d %d", &x, &y) == 2) {
                helper.updateCursor(static_cast<int16_t>(x), static_cast<int16_t>(y), 2, 20);
                std::cout << "Cursor updated to (" << x << ", " << y << ")" << std::endl;
            } else {
                std::cout << "Usage: c <x> <y>" << std::endl;
            }
        } else if (cmd == 't' || cmd == 'T') {
            const char* text = line.c_str() + 1;
            while (*text == ' ') ++text;
            for (int i = 0; text[i] != '\0'; ++i) {
                if (isalpha(text[i])) {
                    helper.processKey(toupper(text[i]), 0);
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
            }
            std::cout << "Typed: " << text << std::endl;
        } else if (cmd == 's' || cmd == 'S') {
            helper.processKey(VK_SPACE, 0);
            std::cout << "Space pressed" << std::endl;
        } else if (cmd == 'e' || cmd == 'E') {
            helper.processKey(VK_ESCAPE, 0);
            std::cout << "Escape pressed" << std::endl;
        } else {
            std::cout << "Unknown command: " << cmd << std::endl;
        }
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "SuYan IME Multi-Application Test Helper" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\nFeature: windows-platform-support" << std::endl;
    std::cout << "Task 13.2: Multi-Application Testing" << std::endl;
    std::cout << std::endl;
    
    printMonitorInfo();
    
    TestHelper helper;
    
    std::cout << "\nConnecting to server..." << std::endl;
    if (!helper.connect()) {
        std::cerr << "ERROR: Failed to connect to SuYanServer." << std::endl;
        std::cerr << "       Please ensure SuYanServer.exe is running." << std::endl;
        return 1;
    }
    std::cout << "Connected successfully." << std::endl;
    
    std::cout << "\nSelect mode:" << std::endl;
    std::cout << "  1 - Automated cursor position test" << std::endl;
    std::cout << "  2 - Interactive mode" << std::endl;
    std::cout << "  q - Quit" << std::endl;
    std::cout << std::endl;
    
    std::string choice;
    std::cout << "Choice: ";
    std::getline(std::cin, choice);
    
    if (choice == "1") {
        testCursorPositions(helper);
    } else if (choice == "2") {
        interactiveMode(helper);
    }
    
    helper.disconnect();
    std::cout << "\nDisconnected. Goodbye!" << std::endl;
    
    return 0;
}

#else  // !_WIN32

#include <iostream>

int main() {
    std::cout << "Multi-Application Test Helper is Windows-only" << std::endl;
    return 0;
}

#endif  // _WIN32
