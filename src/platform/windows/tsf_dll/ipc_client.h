#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <cstdint>

namespace suyan {

class IPCClient {
public:
    IPCClient();
    ~IPCClient();

    IPCClient(const IPCClient&) = delete;
    IPCClient& operator=(const IPCClient&) = delete;

    bool connect();
    void disconnect();
    bool isConnected() const;
    bool ensureServer();

    bool processKey(uint32_t vk, uint32_t modifiers, std::wstring& commitText);
    void updateCursor(int16_t x, int16_t y, int16_t width, int16_t height);
    void focusIn();
    void focusOut();
    void toggleMode();
    void toggleLayout();
    bool queryMode();

    uint32_t sessionId() const { return m_sessionId; }

private:
    bool sendRequest(uint32_t cmd, uint32_t param1, uint32_t param2,
                     uint32_t* result, std::wstring* data);
    bool handshake();
    bool tryConnect();
    bool startServer();
    bool waitForServer(DWORD timeoutMs);
    std::wstring getServerPath();

    HANDLE m_pipe;
    uint32_t m_sessionId;
    bool m_connected;

    static constexpr int MAX_CONNECT_RETRIES = 3;
    static constexpr DWORD CONNECT_RETRY_DELAY_MS = 100;
    static constexpr DWORD SERVER_WAIT_TIMEOUT_MS = 3000;
    static constexpr DWORD PIPE_TIMEOUT_MS = 5000;
};

}  // namespace suyan
