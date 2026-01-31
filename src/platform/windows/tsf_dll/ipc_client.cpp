#include "ipc_client.h"
#include "logger.h"
#include "../../../shared/ipc_protocol.h"
#include <shellapi.h>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

namespace suyan {

IPCClient::IPCClient()
    : m_pipe(INVALID_HANDLE_VALUE)
    , m_sessionId(0)
    , m_connected(false) {
}

IPCClient::~IPCClient() {
    disconnect();
}

bool IPCClient::connect() {
    if (m_connected) {
        return true;
    }

    log::info("IPCClient: Attempting to connect to server");

    for (int attempt = 1; attempt <= MAX_CONNECT_RETRIES; ++attempt) {
        log::debug("IPCClient: Connection attempt %d/%d", attempt, MAX_CONNECT_RETRIES);

        if (tryConnect()) {
            if (handshake()) {
                m_connected = true;
                log::info("IPCClient: Connected successfully, sessionId=%u", m_sessionId);
                return true;
            }
            CloseHandle(m_pipe);
            m_pipe = INVALID_HANDLE_VALUE;
        }

        if (attempt == 1) {
            log::info("IPCClient: Server not available, attempting to start");
            if (ensureServer()) {
                continue;
            }
        }

        if (attempt < MAX_CONNECT_RETRIES) {
            log::debug("IPCClient: Waiting %dms before retry", CONNECT_RETRY_DELAY_MS);
            Sleep(CONNECT_RETRY_DELAY_MS);
        }
    }

    log::error("IPCClient: Failed to connect after %d attempts", MAX_CONNECT_RETRIES);
    return false;
}

void IPCClient::disconnect() {
    if (!m_connected) {
        return;
    }

    log::info("IPCClient: Disconnecting, sessionId=%u", m_sessionId);

    ipc::Request req = {};
    req.cmd = ipc::Command::Disconnect;
    req.sessionId = m_sessionId;

    uint8_t buffer[sizeof(ipc::Request)];
    ipc::serializeRequest(req, buffer);

    DWORD written;
    WriteFile(m_pipe, buffer, sizeof(buffer), &written, nullptr);

    CloseHandle(m_pipe);
    m_pipe = INVALID_HANDLE_VALUE;
    m_sessionId = 0;
    m_connected = false;

    log::info("IPCClient: Disconnected");
}

bool IPCClient::isConnected() const {
    return m_connected;
}

bool IPCClient::tryConnect() {
    m_pipe = CreateFileW(
        ipc::PIPE_NAME,
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
    );

    if (m_pipe == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        if (err == ERROR_PIPE_BUSY) {
            log::debug("IPCClient: Pipe busy, waiting...");
            if (WaitNamedPipeW(ipc::PIPE_NAME, PIPE_TIMEOUT_MS)) {
                m_pipe = CreateFileW(
                    ipc::PIPE_NAME,
                    GENERIC_READ | GENERIC_WRITE,
                    0,
                    nullptr,
                    OPEN_EXISTING,
                    0,
                    nullptr
                );
            }
        }
    }

    if (m_pipe == INVALID_HANDLE_VALUE) {
        log::debug("IPCClient: CreateFile failed, error=%lu", GetLastError());
        return false;
    }

    DWORD mode = PIPE_READMODE_BYTE | PIPE_WAIT;
    if (!SetNamedPipeHandleState(m_pipe, &mode, nullptr, nullptr)) {
        log::warning("IPCClient: SetNamedPipeHandleState failed, error=%lu", GetLastError());
    }

    log::debug("IPCClient: Pipe opened successfully");
    return true;
}

bool IPCClient::handshake() {
    log::debug("IPCClient: Performing handshake");

    uint32_t result = 0;
    if (!sendRequest(static_cast<uint32_t>(ipc::Command::Handshake),
                     ipc::PROTOCOL_VERSION, 0, &result, nullptr)) {
        log::error("IPCClient: Handshake request failed");
        return false;
    }

    if (result == 0) {
        log::error("IPCClient: Handshake rejected by server");
        return false;
    }

    m_sessionId = result;
    log::debug("IPCClient: Handshake successful, sessionId=%u", m_sessionId);
    return true;
}

bool IPCClient::sendRequest(uint32_t cmd, uint32_t param1, uint32_t param2,
                            uint32_t* result, std::wstring* data) {
    if (m_pipe == INVALID_HANDLE_VALUE) {
        return false;
    }

    ipc::Request req = {};
    req.cmd = static_cast<ipc::Command>(cmd);
    req.sessionId = m_sessionId;
    req.param1 = param1;
    req.param2 = param2;

    uint8_t reqBuffer[sizeof(ipc::Request)];
    ipc::serializeRequest(req, reqBuffer);

    DWORD written;
    if (!WriteFile(m_pipe, reqBuffer, sizeof(reqBuffer), &written, nullptr)) {
        log::error("IPCClient: WriteFile failed, error=%lu", GetLastError());
        m_connected = false;
        return false;
    }

    uint8_t hdrBuffer[sizeof(ipc::ResponseHeader)];
    DWORD bytesRead;
    if (!ReadFile(m_pipe, hdrBuffer, sizeof(hdrBuffer), &bytesRead, nullptr)) {
        log::error("IPCClient: ReadFile (header) failed, error=%lu", GetLastError());
        m_connected = false;
        return false;
    }

    if (bytesRead != sizeof(hdrBuffer)) {
        log::error("IPCClient: Incomplete header, got %lu bytes", bytesRead);
        m_connected = false;
        return false;
    }

    ipc::ResponseHeader hdr = ipc::deserializeResponseHeader(hdrBuffer);

    if (result) {
        *result = hdr.result;
    }

    if (hdr.dataSize > 0 && data) {
        size_t charCount = hdr.dataSize / sizeof(wchar_t);
        data->resize(charCount);
        if (!ReadFile(m_pipe, data->data(), hdr.dataSize, &bytesRead, nullptr)) {
            log::error("IPCClient: ReadFile (data) failed, error=%lu", GetLastError());
            m_connected = false;
            return false;
        }
    }

    return true;
}

bool IPCClient::processKey(uint32_t vk, uint32_t modifiers, std::wstring& commitText) {
    if (!m_connected) {
        return false;
    }

    uint32_t result = 0;
    commitText.clear();

    if (!sendRequest(static_cast<uint32_t>(ipc::Command::ProcessKey),
                     vk, modifiers, &result, &commitText)) {
        return false;
    }

    return result != 0;
}

void IPCClient::updateCursor(int16_t x, int16_t y, int16_t width, int16_t height) {
    if (!m_connected) {
        return;
    }

    uint32_t param1, param2;
    ipc::CursorPosition::encode(x, y, width, height, param1, param2);

    sendRequest(static_cast<uint32_t>(ipc::Command::UpdateCursor),
                param1, param2, nullptr, nullptr);
}

void IPCClient::focusIn() {
    if (!m_connected) {
        return;
    }

    log::debug("IPCClient: Sending FocusIn");
    sendRequest(static_cast<uint32_t>(ipc::Command::FocusIn), 0, 0, nullptr, nullptr);
}

void IPCClient::focusOut() {
    if (!m_connected) {
        return;
    }

    log::debug("IPCClient: Sending FocusOut");
    sendRequest(static_cast<uint32_t>(ipc::Command::FocusOut), 0, 0, nullptr, nullptr);
}

void IPCClient::toggleMode() {
    if (!m_connected) {
        return;
    }

    log::debug("IPCClient: Sending ToggleMode");
    sendRequest(static_cast<uint32_t>(ipc::Command::ToggleMode), 0, 0, nullptr, nullptr);
}

void IPCClient::toggleLayout() {
    if (!m_connected) {
        return;
    }

    log::debug("IPCClient: Sending ToggleLayout");
    sendRequest(static_cast<uint32_t>(ipc::Command::ToggleLayout), 0, 0, nullptr, nullptr);
}

bool IPCClient::queryMode() {
    if (!m_connected) {
        return true;
    }

    uint32_t result = 0;
    if (!sendRequest(static_cast<uint32_t>(ipc::Command::QueryMode), 0, 0, &result, nullptr)) {
        return true;
    }

    log::debug("IPCClient: QueryMode result=%u", result);
    return result != 0;
}


bool IPCClient::ensureServer() {
    if (tryConnect()) {
        CloseHandle(m_pipe);
        m_pipe = INVALID_HANDLE_VALUE;
        log::debug("IPCClient: Server already running");
        return true;
    }

    log::info("IPCClient: Starting server process");
    if (!startServer()) {
        log::error("IPCClient: Failed to start server");
        return false;
    }

    if (!waitForServer(SERVER_WAIT_TIMEOUT_MS)) {
        log::error("IPCClient: Server did not become ready within %dms", SERVER_WAIT_TIMEOUT_MS);
        return false;
    }

    log::info("IPCClient: Server started successfully");
    return true;
}

std::wstring IPCClient::getServerPath() {
    HKEY hKey;
    LONG result = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\SuYan",
        0,
        KEY_READ | KEY_WOW64_64KEY,
        &hKey
    );

    if (result != ERROR_SUCCESS) {
        result = RegOpenKeyExW(
            HKEY_CURRENT_USER,
            L"SOFTWARE\\SuYan",
            0,
            KEY_READ,
            &hKey
        );
    }

    if (result != ERROR_SUCCESS) {
        log::warning("IPCClient: Registry key not found, error=%ld", result);
        return L"";
    }

    wchar_t installPath[MAX_PATH] = {};
    DWORD pathSize = sizeof(installPath);
    DWORD type = 0;

    result = RegQueryValueExW(
        hKey,
        L"InstallPath",
        nullptr,
        &type,
        reinterpret_cast<LPBYTE>(installPath),
        &pathSize
    );

    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS || type != REG_SZ) {
        log::warning("IPCClient: InstallPath not found in registry, error=%ld", result);
        return L"";
    }

    std::wstring serverPath = installPath;
    if (!serverPath.empty() && serverPath.back() != L'\\') {
        serverPath += L'\\';
    }
    serverPath += L"SuYanServer.exe";

    log::debug("IPCClient: Server path from registry: %ls", serverPath.c_str());
    return serverPath;
}

bool IPCClient::startServer() {
    std::wstring serverPath = getServerPath();

    if (serverPath.empty()) {
        wchar_t modulePath[MAX_PATH];
        if (GetModuleFileNameW(nullptr, modulePath, MAX_PATH) == 0) {
            log::error("IPCClient: GetModuleFileName failed, error=%lu", GetLastError());
            return false;
        }

        PathRemoveFileSpecW(modulePath);
        serverPath = modulePath;
        serverPath += L"\\SuYanServer.exe";
        log::debug("IPCClient: Using fallback server path: %ls", serverPath.c_str());
    }

    if (GetFileAttributesW(serverPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        log::error("IPCClient: Server executable not found: %ls", serverPath.c_str());
        return false;
    }

    SHELLEXECUTEINFOW sei = {};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
    sei.lpVerb = L"open";
    sei.lpFile = serverPath.c_str();
    sei.nShow = SW_HIDE;

    if (!ShellExecuteExW(&sei)) {
        log::error("IPCClient: ShellExecuteEx failed, error=%lu", GetLastError());
        return false;
    }

    if (sei.hProcess) {
        CloseHandle(sei.hProcess);
    }

    log::info("IPCClient: Server process started");
    return true;
}

bool IPCClient::waitForServer(DWORD timeoutMs) {
    DWORD startTime = GetTickCount();
    DWORD elapsed = 0;

    while (elapsed < timeoutMs) {
        if (tryConnect()) {
            CloseHandle(m_pipe);
            m_pipe = INVALID_HANDLE_VALUE;
            log::debug("IPCClient: Server ready after %lums", elapsed);
            return true;
        }

        Sleep(100);
        elapsed = GetTickCount() - startTime;
    }

    return false;
}

}  // namespace suyan
