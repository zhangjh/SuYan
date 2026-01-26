/**
 * 注册表配置
 * Task 1.1: Windows 平台目录结构
 * 
 * 定义输入法注册信息和注册/注销函数
 */

#ifndef SUYAN_WINDOWS_REGISTRY_CONFIG_H
#define SUYAN_WINDOWS_REGISTRY_CONFIG_H

#ifdef _WIN32

#include <windows.h>
#include <string>

namespace suyan {

/**
 * 输入法注册信息
 */
struct IMERegistryInfo {
    std::wstring clsid;           // COM CLSID
    std::wstring profileGuid;     // 语言配置 GUID
    std::wstring description;     // 描述
    std::wstring iconFile;        // 图标文件路径
    int iconIndex;                // 图标索引
    LANGID langId;                // 语言 ID (0x0804 = 简体中文)
};

/**
 * 获取默认注册信息
 */
IMERegistryInfo getDefaultRegistryInfo();

/**
 * 注册输入法到系统
 * @param info 注册信息
 * @return 是否成功
 */
bool registerIME(const IMERegistryInfo& info);

/**
 * 从系统注销输入法
 * @param info 注册信息
 * @return 是否成功
 */
bool unregisterIME(const IMERegistryInfo& info);

} // namespace suyan

#endif // _WIN32

#endif // SUYAN_WINDOWS_REGISTRY_CONFIG_H
