#pragma once

#include <memory>
#include <cstdint>
#include <string>
#include <QRect>

#ifdef _WIN32

namespace suyan {

class IPCServer;
class InputEngine;
class CandidateWindow;

class ServerApp {
public:
    ServerApp();
    ~ServerApp();

    ServerApp(const ServerApp&) = delete;
    ServerApp& operator=(const ServerApp&) = delete;

    bool initialize();
    void shutdown();
    bool isRunning() const;

    static int convertVirtualKeyToRimeKey(int vk, int modifiers);

private:
    uint32_t handleIPCRequest(uint32_t sessionId, uint32_t cmd,
                              uint32_t param1, uint32_t param2,
                              std::wstring& response);

    void handleHandshake(uint32_t sessionId, std::wstring& response);
    uint32_t handleProcessKey(uint32_t sessionId, uint32_t vk, uint32_t modifiers, std::wstring& response);
    void handleFocusIn(uint32_t sessionId);
    void handleFocusOut(uint32_t sessionId);
    void handleUpdateCursor(uint32_t sessionId, uint32_t param1, uint32_t param2);
    void handleToggleMode(uint32_t sessionId);
    void handleToggleLayout(uint32_t sessionId);
    uint32_t handleQueryMode(uint32_t sessionId);

    std::unique_ptr<IPCServer> m_ipcServer;
    std::unique_ptr<InputEngine> m_inputEngine;
    CandidateWindow* m_candidateWindow;
    bool m_running;

    QRect m_lastCursorRect;
};

}  // namespace suyan

#endif  // _WIN32
