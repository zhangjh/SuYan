#pragma once

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <cstdint>
#include <functional>
#include <memory>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <vector>

namespace suyan {

class SingleInstanceLock {
public:
    SingleInstanceLock();
    ~SingleInstanceLock();

    SingleInstanceLock(const SingleInstanceLock&) = delete;
    SingleInstanceLock& operator=(const SingleInstanceLock&) = delete;

    bool tryAcquire();
    void release();
    bool isAcquired() const;

private:
    HANDLE m_mutex;
    bool m_acquired;

    static constexpr wchar_t MUTEX_NAME[] = L"Global\\SuYanInputMethodServer";
};

using RequestHandler = std::function<uint32_t(
    uint32_t sessionId,
    uint32_t cmd,
    uint32_t param1,
    uint32_t param2,
    std::wstring& response
)>;

class IPCServer {
public:
    IPCServer();
    ~IPCServer();

    IPCServer(const IPCServer&) = delete;
    IPCServer& operator=(const IPCServer&) = delete;

    void setHandler(RequestHandler handler);
    bool start(int workerThreads = 4);
    void stop();
    bool isRunning() const;

    size_t clientCount() const;

private:
    enum class IOOperation {
        Read,
        Write
    };

    struct IOContext {
        OVERLAPPED overlapped;
        IOOperation operation;
        uint8_t buffer[4096];
        DWORD bytesTransferred;
    };

    struct ClientContext {
        HANDLE pipe;
        uint32_t sessionId;
        std::atomic<bool> isActive;
        IOContext readContext;
        IOContext writeContext;
        std::vector<uint8_t> pendingWriteData;

        ClientContext() : pipe(INVALID_HANDLE_VALUE), sessionId(0), isActive(false) {
            memset(&readContext, 0, sizeof(readContext));
            memset(&writeContext, 0, sizeof(writeContext));
            readContext.operation = IOOperation::Read;
            writeContext.operation = IOOperation::Write;
        }
    };

    void acceptThread();
    void workerThread();
    void processRequest(ClientContext* client, const uint8_t* data, DWORD size);
    void sendResponse(ClientContext* client, uint32_t result, const std::wstring& data);
    void startRead(ClientContext* client);
    void removeClient(uint32_t sessionId);
    uint32_t generateSessionId();
    HANDLE createPipeInstance();

    HANDLE m_iocp;
    std::atomic<bool> m_running;
    std::atomic<uint32_t> m_nextSessionId;
    RequestHandler m_handler;

    std::unordered_map<uint32_t, std::unique_ptr<ClientContext>> m_clients;
    mutable std::mutex m_clientsMutex;

    std::thread m_acceptThread;
    std::vector<std::thread> m_workerThreads;

    static constexpr DWORD PIPE_BUFFER_SIZE = 4096;
    static constexpr DWORD PIPE_TIMEOUT_MS = 5000;
};

}  // namespace suyan

#endif  // _WIN32
