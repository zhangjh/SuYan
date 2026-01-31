#ifdef _WIN32

#include "ipc_server.h"
#include "../shared/ipc_protocol.h"
#include "../server/logger.h"

namespace suyan {

constexpr wchar_t SingleInstanceLock::MUTEX_NAME[];

SingleInstanceLock::SingleInstanceLock()
    : m_mutex(nullptr)
    , m_acquired(false) {
}

SingleInstanceLock::~SingleInstanceLock() {
    release();
}

bool SingleInstanceLock::tryAcquire() {
    if (m_acquired) {
        return true;
    }

    m_mutex = CreateMutexW(nullptr, FALSE, MUTEX_NAME);
    DWORD err = GetLastError();

    if (!m_mutex) {
        log::error("SingleInstanceLock: CreateMutex failed, error=%lu", err);
        return false;
    }

    if (err == ERROR_ALREADY_EXISTS) {
        log::info("SingleInstanceLock: Another instance is already running");
        CloseHandle(m_mutex);
        m_mutex = nullptr;
        return false;
    }

    m_acquired = true;
    log::info("SingleInstanceLock: Acquired successfully");
    return true;
}

void SingleInstanceLock::release() {
    if (m_mutex) {
        if (m_acquired) {
            ReleaseMutex(m_mutex);
            log::info("SingleInstanceLock: Released");
        }
        CloseHandle(m_mutex);
        m_mutex = nullptr;
        m_acquired = false;
    }
}

bool SingleInstanceLock::isAcquired() const {
    return m_acquired;
}

IPCServer::IPCServer()
    : m_iocp(nullptr)
    , m_running(false)
    , m_nextSessionId(1) {
}

IPCServer::~IPCServer() {
    stop();
}

void IPCServer::setHandler(RequestHandler handler) {
    m_handler = std::move(handler);
}

bool IPCServer::start(int workerThreads) {
    if (m_running) {
        log::warning("IPCServer: Already running");
        return true;
    }

    log::info("IPCServer: Starting with %d worker threads", workerThreads);

    m_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, workerThreads);
    if (!m_iocp) {
        log::error("IPCServer: CreateIoCompletionPort failed, error=%lu", GetLastError());
        return false;
    }

    m_running = true;

    m_acceptThread = std::thread(&IPCServer::acceptThread, this);

    for (int i = 0; i < workerThreads; ++i) {
        m_workerThreads.emplace_back(&IPCServer::workerThread, this);
    }

    log::info("IPCServer: Started successfully");
    return true;
}

void IPCServer::stop() {
    if (!m_running) {
        return;
    }

    log::info("IPCServer: Stopping...");
    m_running = false;

    if (m_acceptThread.joinable()) {
        m_acceptThread.join();
    }

    for (size_t i = 0; i < m_workerThreads.size(); ++i) {
        PostQueuedCompletionStatus(m_iocp, 0, 0, nullptr);
    }

    for (auto& thread : m_workerThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    m_workerThreads.clear();

    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        for (auto& pair : m_clients) {
            if (pair.second->pipe != INVALID_HANDLE_VALUE) {
                DisconnectNamedPipe(pair.second->pipe);
                CloseHandle(pair.second->pipe);
            }
        }
        m_clients.clear();
    }

    if (m_iocp) {
        CloseHandle(m_iocp);
        m_iocp = nullptr;
    }

    log::info("IPCServer: Stopped");
}

bool IPCServer::isRunning() const {
    return m_running;
}

size_t IPCServer::clientCount() const {
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    return m_clients.size();
}

HANDLE IPCServer::createPipeInstance() {
    SECURITY_DESCRIPTOR sd;
    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&sd, TRUE, nullptr, FALSE);

    SECURITY_ATTRIBUTES sa = {};
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = FALSE;

    HANDLE pipe = CreateNamedPipeW(
        ipc::PIPE_NAME,
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        PIPE_BUFFER_SIZE,
        PIPE_BUFFER_SIZE,
        PIPE_TIMEOUT_MS,
        &sa
    );

    if (pipe == INVALID_HANDLE_VALUE) {
        log::error("IPCServer: CreateNamedPipe failed, error=%lu", GetLastError());
    }

    return pipe;
}

void IPCServer::acceptThread() {
    log::info("IPCServer: Accept thread started");

    while (m_running) {
        HANDLE pipe = createPipeInstance();
        if (pipe == INVALID_HANDLE_VALUE) {
            Sleep(100);
            continue;
        }

        OVERLAPPED connectOverlapped = {};
        connectOverlapped.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);

        BOOL connected = ConnectNamedPipe(pipe, &connectOverlapped);
        DWORD err = GetLastError();

        if (!connected) {
            if (err == ERROR_IO_PENDING) {
                DWORD waitResult = WaitForSingleObject(connectOverlapped.hEvent, 500);
                while (waitResult == WAIT_TIMEOUT && m_running) {
                    waitResult = WaitForSingleObject(connectOverlapped.hEvent, 500);
                }

                if (!m_running) {
                    CancelIo(pipe);
                    CloseHandle(connectOverlapped.hEvent);
                    CloseHandle(pipe);
                    break;
                }

                if (waitResult != WAIT_OBJECT_0) {
                    CloseHandle(connectOverlapped.hEvent);
                    CloseHandle(pipe);
                    continue;
                }
            } else if (err != ERROR_PIPE_CONNECTED) {
                log::error("IPCServer: ConnectNamedPipe failed, error=%lu", err);
                CloseHandle(connectOverlapped.hEvent);
                CloseHandle(pipe);
                continue;
            }
        }

        CloseHandle(connectOverlapped.hEvent);

        auto client = std::make_unique<ClientContext>();
        client->pipe = pipe;
        client->isActive = true;

        if (!CreateIoCompletionPort(pipe, m_iocp, reinterpret_cast<ULONG_PTR>(client.get()), 0)) {
            log::error("IPCServer: Failed to associate pipe with IOCP, error=%lu", GetLastError());
            DisconnectNamedPipe(pipe);
            CloseHandle(pipe);
            continue;
        }

        log::info("IPCServer: New client connected (pending handshake)");

        uint32_t tempSessionId = generateSessionId();
        client->sessionId = tempSessionId;
        
        ClientContext* clientPtr = client.get();
        {
            std::lock_guard<std::mutex> lock(m_clientsMutex);
            m_clients[tempSessionId] = std::move(client);
        }

        startRead(clientPtr);
    }

    log::info("IPCServer: Accept thread stopped");
}

void IPCServer::workerThread() {
    log::debug("IPCServer: Worker thread started");

    while (m_running) {
        DWORD bytesTransferred = 0;
        ULONG_PTR completionKey = 0;
        LPOVERLAPPED overlapped = nullptr;

        BOOL success = GetQueuedCompletionStatus(
            m_iocp,
            &bytesTransferred,
            &completionKey,
            &overlapped,
            1000
        );

        if (!overlapped) {
            if (!success && GetLastError() == WAIT_TIMEOUT) {
                continue;
            }
            if (!m_running) {
                break;
            }
            continue;
        }

        auto* client = reinterpret_cast<ClientContext*>(completionKey);
        auto* ioContext = reinterpret_cast<IOContext*>(overlapped);

        if (!success || bytesTransferred == 0) {
            if (client && client->isActive) {
                log::info("IPCServer: Client disconnected, sessionId=%u", client->sessionId);
                client->isActive = false;
                removeClient(client->sessionId);
            }
            continue;
        }

        if (ioContext->operation == IOOperation::Read) {
            processRequest(client, ioContext->buffer, bytesTransferred);
            if (client->isActive) {
                startRead(client);
            }
        }
    }

    log::debug("IPCServer: Worker thread stopped");
}

void IPCServer::startRead(ClientContext* client) {
    if (!client || !client->isActive) {
        return;
    }

    memset(&client->readContext.overlapped, 0, sizeof(OVERLAPPED));

    BOOL success = ReadFile(
        client->pipe,
        client->readContext.buffer,
        sizeof(client->readContext.buffer),
        nullptr,
        &client->readContext.overlapped
    );

    if (!success && GetLastError() != ERROR_IO_PENDING) {
        log::error("IPCServer: ReadFile failed, error=%lu", GetLastError());
        client->isActive = false;
        removeClient(client->sessionId);
    }
}

void IPCServer::processRequest(ClientContext* client, const uint8_t* data, DWORD size) {
    if (size < sizeof(ipc::Request)) {
        log::warning("IPCServer: Received incomplete request, size=%lu", size);
        return;
    }

    ipc::Request req = ipc::deserializeRequest(data);

    log::debug("IPCServer: Received cmd=%u, sessionId=%u, param1=%u, param2=%u",
               static_cast<uint32_t>(req.cmd), req.sessionId, req.param1, req.param2);

    if (req.cmd == ipc::Command::Handshake) {
        uint32_t protocolVersion = req.param1;
        if (protocolVersion != ipc::PROTOCOL_VERSION) {
            log::warning("IPCServer: Protocol version mismatch, client=%u, server=%u",
                        protocolVersion, ipc::PROTOCOL_VERSION);
            sendResponse(client, 0, L"");
            return;
        }

        log::info("IPCServer: Client handshake complete, sessionId=%u", client->sessionId);
        sendResponse(client, client->sessionId, L"");
        return;
    }

    if (req.cmd == ipc::Command::Disconnect) {
        log::info("IPCServer: Client requested disconnect, sessionId=%u", req.sessionId);
        client->isActive = false;
        removeClient(req.sessionId);
        return;
    }

    if (!m_handler) {
        log::warning("IPCServer: No handler set, ignoring request");
        sendResponse(client, 0, L"");
        return;
    }

    std::wstring responseData;
    uint32_t result = m_handler(
        req.sessionId,
        static_cast<uint32_t>(req.cmd),
        req.param1,
        req.param2,
        responseData
    );

    sendResponse(client, result, responseData);
}

void IPCServer::sendResponse(ClientContext* client, uint32_t result, const std::wstring& data) {
    if (!client || !client->isActive) {
        return;
    }

    ipc::ResponseHeader hdr;
    hdr.result = result;
    hdr.dataSize = static_cast<uint32_t>(data.size() * sizeof(wchar_t));

    client->pendingWriteData.resize(sizeof(ipc::ResponseHeader) + hdr.dataSize);
    ipc::serializeResponseHeader(hdr, client->pendingWriteData.data());

    if (hdr.dataSize > 0) {
        memcpy(client->pendingWriteData.data() + sizeof(ipc::ResponseHeader),
               data.data(), hdr.dataSize);
    }

    memset(&client->writeContext.overlapped, 0, sizeof(OVERLAPPED));
    client->writeContext.overlapped.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);

    DWORD bytesWritten = 0;
    BOOL success = WriteFile(
        client->pipe,
        client->pendingWriteData.data(),
        static_cast<DWORD>(client->pendingWriteData.size()),
        &bytesWritten,
        &client->writeContext.overlapped
    );

    if (!success) {
        DWORD err = GetLastError();
        if (err == ERROR_IO_PENDING) {
            WaitForSingleObject(client->writeContext.overlapped.hEvent, 5000);
            GetOverlappedResult(client->pipe, &client->writeContext.overlapped, &bytesWritten, FALSE);
        } else {
            log::error("IPCServer: WriteFile failed, error=%lu", err);
        }
    }

    CloseHandle(client->writeContext.overlapped.hEvent);
}

void IPCServer::removeClient(uint32_t sessionId) {
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    auto it = m_clients.find(sessionId);
    if (it != m_clients.end()) {
        if (it->second->pipe != INVALID_HANDLE_VALUE) {
            DisconnectNamedPipe(it->second->pipe);
            CloseHandle(it->second->pipe);
        }
        m_clients.erase(it);
        log::debug("IPCServer: Removed client sessionId=%u, remaining=%zu",
                   sessionId, m_clients.size());
    }
}

uint32_t IPCServer::generateSessionId() {
    return m_nextSessionId.fetch_add(1);
}

}  // namespace suyan

#endif  // _WIN32
