/**
 * Windows 平台主入口
 * Task 4: TSFBridge COM 基础实现
 * 
 * DLL 入口点，初始化模块句柄
 */

#ifdef _WIN32

#include <windows.h>
#include "tsf_bridge.h"

// DLL 入口点
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    (void)lpvReserved; // 未使用
    
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            // DLL 被加载
            // 保存模块句柄，用于后续获取 DLL 路径
            suyan::SetModuleHandle(hinstDLL);
            // 禁用线程通知以提高性能
            DisableThreadLibraryCalls(hinstDLL);
            break;
            
        case DLL_PROCESS_DETACH:
            // DLL 被卸载
            // 清理工作将在 Task 8 中实现
            break;
    }
    
    return TRUE;
}

#endif // _WIN32
