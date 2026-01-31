#ifdef _WIN32

// Qt headers must be included before rime_api.h because rime_api.h defines Bool macro
// which conflicts with Qt's qmetatype.h
#include <QStandardPaths>
#include <QDir>
#include <QApplication>
#include <QMetaObject>

#include "server_app.h"
#include "logger.h"
#include "../core/ipc_server.h"
#include "../core/input_engine.h"
#include "../ui/candidate_window.h"
#include "../ui/layout_manager.h"
#include "../shared/ipc_protocol.h"
#include <windows.h>

namespace suyan {

namespace {

struct KeyMapping {
    int vk;
    int rimeKey;
};

const KeyMapping SPECIAL_KEYS[] = {
    {VK_SPACE,   0x20},
    {VK_RETURN,  0xff0d},
    {VK_BACK,    0xff08},
    {VK_ESCAPE,  0xff1b},
    {VK_TAB,     0xff09},
    {VK_LEFT,    0xff51},
    {VK_UP,      0xff52},
    {VK_RIGHT,   0xff53},
    {VK_DOWN,    0xff54},
    {VK_PRIOR,   0xff55},
    {VK_NEXT,    0xff56},
    {VK_HOME,    0xff50},
    {VK_END,     0xff57},
    {VK_DELETE,  0xffff},
    {VK_INSERT,  0xff63},
};

struct OEMKeyMapping {
    int vk;
    char normal;
    char shifted;
};

const OEMKeyMapping OEM_KEYS[] = {
    {VK_OEM_1,      ';', ':'},
    {VK_OEM_PLUS,   '=', '+'},
    {VK_OEM_COMMA,  ',', '<'},
    {VK_OEM_MINUS,  '-', '_'},
    {VK_OEM_PERIOD, '.', '>'},
    {VK_OEM_2,      '/', '?'},
    {VK_OEM_3,      '`', '~'},
    {VK_OEM_4,      '[', '{'},
    {VK_OEM_5,      '\\', '|'},
    {VK_OEM_6,      ']', '}'},
    {VK_OEM_7,      '\'', '"'},
};

}  // namespace

ServerApp::ServerApp()
    : m_candidateWindow(nullptr)
    , m_running(false)
    , m_lastCursorRect(0, 0, 1, 20) {
}

ServerApp::~ServerApp() {
    shutdown();
}

bool ServerApp::initialize() {
    if (m_running) {
        return true;
    }

    log::info("ServerApp initializing...");

    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appData);
    dir.cdUp();
    QString suyanDir = dir.filePath(QStringLiteral("SuYan"));
    QString userDataDir = suyanDir + QStringLiteral("/rime");

    // 共享数据目录：从注册表读取安装路径，或使用默认路径
    QString sharedDataDir;
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\SuYan", 0, 
                      KEY_READ | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS) {
        wchar_t installPath[MAX_PATH] = {};
        DWORD pathSize = sizeof(installPath);
        DWORD type = 0;
        if (RegQueryValueExW(hKey, L"InstallPath", nullptr, &type,
                             reinterpret_cast<LPBYTE>(installPath), &pathSize) == ERROR_SUCCESS) {
            sharedDataDir = QString::fromWCharArray(installPath) + QStringLiteral("/rime");
        }
        RegCloseKey(hKey);
    }
    
    if (sharedDataDir.isEmpty()) {
        sharedDataDir = QStringLiteral("C:/Program Files/SuYan/rime");
    }
    
    log::info("User data dir: %s", userDataDir.toUtf8().constData());
    log::info("Shared data dir: %s", sharedDataDir.toUtf8().constData());

    QDir().mkpath(userDataDir);

    m_inputEngine = std::make_unique<InputEngine>();
    if (!m_inputEngine->initialize(userDataDir.toStdString(), sharedDataDir.toStdString())) {
        log::error("Failed to initialize InputEngine");
        return false;
    }
    log::info("InputEngine initialized");

    m_candidateWindow = new CandidateWindow();
    m_candidateWindow->connectToThemeManager();
    m_candidateWindow->connectToLayoutManager();
    m_candidateWindow->syncFromManagers();

    m_inputEngine->setStateChangedCallback([this](const InputState& state) {
        QMetaObject::invokeMethod(qApp, [this, state]() {
            if (state.isComposing && !state.candidates.empty()) {
                m_candidateWindow->updateCandidates(state);
                if (!m_candidateWindow->isWindowVisible()) {
                    m_candidateWindow->showAtNative(m_lastCursorRect);
                }
            } else {
                m_candidateWindow->hideWindow();
            }
        }, Qt::BlockingQueuedConnection);
    });

    m_inputEngine->setCommitTextCallback([](const std::string& text) {
        log::debug("Commit text: %s", text.c_str());
    });

    m_ipcServer = std::make_unique<IPCServer>();
    m_ipcServer->setHandler([this](uint32_t sessionId, uint32_t cmd,
                                   uint32_t param1, uint32_t param2,
                                   std::wstring& response) {
        return handleIPCRequest(sessionId, cmd, param1, param2, response);
    });

    if (!m_ipcServer->start(4)) {
        log::error("Failed to start IPC server");
        return false;
    }
    log::info("IPC server started");

    m_running = true;
    log::info("ServerApp initialized successfully");
    return true;
}

void ServerApp::shutdown() {
    if (!m_running) {
        return;
    }

    log::info("ServerApp shutting down...");

    if (m_ipcServer) {
        m_ipcServer->stop();
        m_ipcServer.reset();
    }

    if (m_inputEngine) {
        m_inputEngine->shutdown();
        m_inputEngine.reset();
    }

    if (m_candidateWindow) {
        m_candidateWindow->disconnectFromThemeManager();
        m_candidateWindow->disconnectFromLayoutManager();
        delete m_candidateWindow;
        m_candidateWindow = nullptr;
    }

    m_running = false;
    log::info("ServerApp shutdown complete");
}

bool ServerApp::isRunning() const {
    return m_running;
}

uint32_t ServerApp::handleIPCRequest(uint32_t sessionId, uint32_t cmd,
                                     uint32_t param1, uint32_t param2,
                                     std::wstring& response) {
    auto command = static_cast<ipc::Command>(cmd);

    switch (command) {
        case ipc::Command::Handshake:
            handleHandshake(sessionId, response);
            return sessionId;

        case ipc::Command::Disconnect:
            log::info("Client %u disconnected", sessionId);
            return 1;

        case ipc::Command::ProcessKey:
            return handleProcessKey(sessionId, param1, param2, response);

        case ipc::Command::FocusIn:
            handleFocusIn(sessionId);
            return 1;

        case ipc::Command::FocusOut:
            handleFocusOut(sessionId);
            return 1;

        case ipc::Command::UpdateCursor:
            handleUpdateCursor(sessionId, param1, param2);
            return 1;

        case ipc::Command::ToggleMode:
            handleToggleMode(sessionId);
            return 1;

        case ipc::Command::ToggleLayout:
            handleToggleLayout(sessionId);
            return 1;

        case ipc::Command::QueryMode:
            return handleQueryMode(sessionId);

        default:
            log::warning("Unknown command: 0x%04x from session %u", cmd, sessionId);
            return 0;
    }
}

void ServerApp::handleHandshake(uint32_t sessionId, std::wstring& response) {
    log::info("Handshake from session %u, protocol version %u", sessionId, ipc::PROTOCOL_VERSION);
    response.clear();
}

uint32_t ServerApp::handleProcessKey(uint32_t sessionId, uint32_t vk, uint32_t modifiers, std::wstring& response) {
    int rimeKey = convertVirtualKeyToRimeKey(static_cast<int>(vk), static_cast<int>(modifiers));
    if (rimeKey == 0) {
        log::debug("ProcessKey: vk=0x%02x mod=0x%02x - unknown key, not processed", vk, modifiers);
        response.clear();
        return 0;
    }

    int rimeMod = 0;
    if (modifiers & ipc::ModShift) rimeMod |= KeyModifier::Shift;
    if (modifiers & ipc::ModControl) rimeMod |= KeyModifier::Control;
    if (modifiers & ipc::ModAlt) rimeMod |= KeyModifier::Alt;

    std::string commitText;
    auto oldCallback = m_inputEngine->getCommitTextCallback();
    m_inputEngine->setCommitTextCallback([&commitText](const std::string& text) {
        commitText = text;
    });

    bool processed = m_inputEngine->processKeyEvent(rimeKey, rimeMod);

    m_inputEngine->setCommitTextCallback(oldCallback);

    if (!commitText.empty()) {
        int wlen = MultiByteToWideChar(CP_UTF8, 0, commitText.c_str(), -1, nullptr, 0);
        if (wlen > 0) {
            response.resize(wlen - 1);
            MultiByteToWideChar(CP_UTF8, 0, commitText.c_str(), -1, response.data(), wlen);
        }
        log::info("ProcessKey: vk=0x%02x rimeKey=0x%04x commitText=%s", vk, rimeKey, commitText.c_str());
    } else {
        response.clear();
    }

    log::debug("ProcessKey: vk=0x%02x mod=0x%02x rimeKey=0x%04x processed=%d",
               vk, modifiers, rimeKey, processed);

    return processed ? 1 : 0;
}

void ServerApp::handleFocusIn(uint32_t sessionId) {
    log::debug("FocusIn from session %u", sessionId);
    m_inputEngine->activate();
}

void ServerApp::handleFocusOut(uint32_t sessionId) {
    log::debug("FocusOut from session %u", sessionId);
    m_inputEngine->deactivate();
    QMetaObject::invokeMethod(qApp, [this]() {
        if (m_candidateWindow) {
            m_candidateWindow->hideWindow();
        }
    }, Qt::BlockingQueuedConnection);
}

void ServerApp::handleUpdateCursor(uint32_t sessionId, uint32_t param1, uint32_t param2) {
    int16_t x, y, w, h;
    ipc::CursorPosition::decode(param1, param2, x, y, w, h);

    log::debug("UpdateCursor from session %u: x=%d y=%d w=%d h=%d", sessionId, x, y, w, h);

    m_lastCursorRect = QRect(x, y, w > 0 ? w : 1, h > 0 ? h : 20);

    QMetaObject::invokeMethod(qApp, [this]() {
        if (m_candidateWindow && m_candidateWindow->isWindowVisible()) {
            m_candidateWindow->showAtNative(m_lastCursorRect);
        }
    }, Qt::BlockingQueuedConnection);
}

int ServerApp::convertVirtualKeyToRimeKey(int vk, int modifiers) {
    for (const auto& mapping : SPECIAL_KEYS) {
        if (mapping.vk == vk) {
            return mapping.rimeKey;
        }
    }

    if (vk >= 'A' && vk <= 'Z') {
        bool shift = (modifiers & ipc::ModShift) != 0;
        if (shift) {
            return vk;
        } else {
            return vk + ('a' - 'A');
        }
    }

    if (vk >= '0' && vk <= '9') {
        return vk;
    }

    for (const auto& mapping : OEM_KEYS) {
        if (mapping.vk == vk) {
            bool shift = (modifiers & ipc::ModShift) != 0;
            return shift ? mapping.shifted : mapping.normal;
        }
    }

    if (vk >= VK_F1 && vk <= VK_F12) {
        return 0xffbe + (vk - VK_F1);
    }

    return 0;
}

void ServerApp::handleToggleMode(uint32_t sessionId) {
    log::debug("ToggleMode from session %u", sessionId);
    if (m_inputEngine) {
        m_inputEngine->toggleMode();
    }
}

void ServerApp::handleToggleLayout(uint32_t sessionId) {
    log::debug("ToggleLayout from session %u", sessionId);
    LayoutManager::instance().toggleLayout();
}

uint32_t ServerApp::handleQueryMode(uint32_t sessionId) {
    log::debug("QueryMode from session %u", sessionId);
    if (m_inputEngine) {
        return m_inputEngine->getMode() == InputMode::Chinese ? 1 : 0;
    }
    return 1;
}

}  // namespace suyan

#endif  // _WIN32
