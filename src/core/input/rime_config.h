// src/core/input/rime_config.h
// RIME 配置管理

#ifndef IME_RIME_CONFIG_H
#define IME_RIME_CONFIG_H

#include <string>
#include <vector>

namespace ime {
namespace input {

// RIME 配置路径
struct RimeConfigPaths {
    std::string sharedDataDir;   // 共享数据目录（词库等）
    std::string userDataDir;     // 用户数据目录（用户词库、配置）
    std::string logDir;          // 日志目录
};

// 词库信息
struct DictionaryInfo {
    std::string id;       // 词库 ID
    std::string name;     // 词库名称
    std::string path;     // 词库路径
    int priority;         // 优先级（数值越大优先级越高）
    bool enabled;         // 是否启用
};

// RIME 配置管理器
class RimeConfig {
public:
    // 获取默认配置路径
    static RimeConfigPaths GetDefaultPaths();

    // 获取平台特定的用户数据目录
    static std::string GetPlatformUserDataDir();

    // 获取平台特定的共享数据目录
    static std::string GetPlatformSharedDataDir();

    // 获取默认词库加载顺序
    static std::vector<DictionaryInfo> GetDefaultDictionaryOrder();

    // 获取方案 ID
    static std::string GetSchemaId() { return "ime_pinyin"; }

    // 获取方案名称
    static std::string GetSchemaName() { return "简体拼音"; }

    // 检查配置文件是否存在
    static bool ConfigFilesExist(const std::string& userDataDir);

    // 部署配置文件到用户目录
    static bool DeployConfigFiles(const std::string& sourceDir,
                                  const std::string& userDataDir);

    // 部署词库文件
    static bool DeployDictionaries(const std::string& rimeIceDir,
                                   const std::string& userDataDir);

private:
    // 复制文件
    static bool CopyFile(const std::string& src, const std::string& dst);

    // 复制目录
    static bool CopyDirectory(const std::string& src, const std::string& dst);

    // 创建目录
    static bool CreateDirectoryIfNotExists(const std::string& path);
};

// 词库加载顺序常量
namespace DictionaryOrder {
    // 基础词库（最高优先级）
    constexpr int BASE_PRIORITY = 100;
    // 扩展词库
    constexpr int EXT_PRIORITY = 90;
    // 腾讯词库
    constexpr int TENCENT_PRIORITY = 80;
    // 其他词库
    constexpr int OTHERS_PRIORITY = 70;
    // 用户词库（动态调整）
    constexpr int USER_PRIORITY = 200;
}

}  // namespace input
}  // namespace ime

#endif  // IME_RIME_CONFIG_H
