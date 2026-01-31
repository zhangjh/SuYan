#ifdef _WIN32

#include "server_app.h"
#include "logger.h"
#include "../core/ipc_server.h"
#include "../ui/suyan_ui_init.h"
#include <QApplication>
#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    int argc = 0;
    char* argv[] = {nullptr};
    QApplication app(argc, argv);
    app.setApplicationName("SuYan");
    app.setOrganizationName("SuYan");
    app.setQuitOnLastWindowClosed(false);

    suyan::log::initialize(L"server");
    suyan::log::setLevel(suyan::log::Level::Debug);
    suyan::log::info("SuYanServer starting...");

    suyan::SingleInstanceLock instanceLock;
    if (!instanceLock.tryAcquire()) {
        suyan::log::info("Another instance is already running, exiting");
        suyan::log::shutdown();
        return 0;
    }

    suyan::initializeUISimple();

    suyan::ServerApp serverApp;
    if (!serverApp.initialize()) {
        suyan::log::error("Failed to initialize ServerApp");
        suyan::log::shutdown();
        return 1;
    }

    suyan::log::info("SuYanServer running");
    int result = app.exec();

    serverApp.shutdown();
    suyan::log::info("SuYanServer exiting with code %d", result);
    suyan::log::shutdown();

    return result;
}

#else

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    return 0;
}

#endif  // _WIN32
