// src/core/input/rime_config.cpp
// RIME 配置管理实现

#include "rime_config.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>

#ifdef _WIN32
#include <shlobj.h>
#include <windows.h>
#elif defined(__APPLE__)
#include <pwd.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace ime {
namespace input {

RimeConfigPaths RimeConfig::GetDefaultPaths() {
    RimeConfigPaths paths;
    paths.userDataDir = GetPlatformUserDataDir();
    paths.sharedDataDir = GetPlatformSharedDataDir();
    paths.logDir = paths.userDataDir + "/logs";
    return paths;
}

std::string RimeConfig::GetPlatformUserDataDir() {
#ifdef _WIN32
    // Windows: %APPDATA%\CrossPlatformIME
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, path))) {
        return std::string(path) + "\\CrossPlatformIME";
    }
    return ".\\CrossPlatformIME";
#elif defined(__APPLE__)
    // macOS: ~/Library/Rime
    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pw = getpwuid(getuid());
        if (pw) {
            home = pw->pw_dir;
        }
    }
    if (home) {
        return std::string(home) + "/Library/Rime";
    }
    return "./Rime";
#else
    // Linux: ~/.config/rime
    const char* home = getenv("HOME");
    if (home) {
        return std::string(home) + "/.config/rime";
    }
    return "./rime";
#endif
}

std::string RimeConfig::GetPlatformSharedDataDir() {
#ifdef _WIN32
    // Windows: 程序目录/data
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    std::string exePath(path);
    size_t pos = exePath.find_last_of("\\/");
    if (pos != std::string::npos) {
        return exePath.substr(0, pos) + "\\data\\rime";
    }
    return ".\\data\\rime";
#elif defined(__APPLE__)
    // macOS: /Library/Input Methods/CrossPlatformIME.app/Contents/SharedSupport
    // 或开发时使用相对路径
    return "/Library/Input Methods/CrossPlatformIME.app/Contents/SharedSupport";
#else
    // Linux: /usr/share/rime-data
    return "/usr/share/rime-data";
#endif
}

std::vector<DictionaryInfo> RimeConfig::GetDefaultDictionaryOrder() {
    std::vector<DictionaryInfo> dicts;

    // 基础字表
    dicts.push_back({
        "8105",
        "通用规范汉字表",
        "cn_dicts/8105.dict.yaml",
        DictionaryOrder::BASE_PRIORITY + 10,
        true
    });

    // 基础词库
    dicts.push_back({
        "base",
        "基础词库",
        "cn_dicts/base.dict.yaml",
        DictionaryOrder::BASE_PRIORITY,
        true
    });

    // 扩展词库
    dicts.push_back({
        "ext",
        "扩展词库",
        "cn_dicts/ext.dict.yaml",
        DictionaryOrder::EXT_PRIORITY,
        true
    });

    // 腾讯词库
    dicts.push_back({
        "tencent",
        "腾讯词向量词库",
        "cn_dicts/tencent.dict.yaml",
        DictionaryOrder::TENCENT_PRIORITY,
        true
    });

    // 其他词库
    dicts.push_back({
        "others",
        "其他词汇",
        "cn_dicts/others.dict.yaml",
        DictionaryOrder::OTHERS_PRIORITY,
        true
    });

    // 英文词库
    dicts.push_back({
        "en",
        "英文词库",
        "en_dicts/en.dict.yaml",
        DictionaryOrder::OTHERS_PRIORITY - 10,
        true
    });

    dicts.push_back({
        "en_ext",
        "英文扩展词库",
        "en_dicts/en_ext.dict.yaml",
        DictionaryOrder::OTHERS_PRIORITY - 20,
        true
    });

    return dicts;
}

bool RimeConfig::ConfigFilesExist(const std::string& userDataDir) {
    // 检查必要的配置文件是否存在
    std::vector<std::string> requiredFiles = {
        "default.custom.yaml",
        "ime_pinyin.schema.yaml",
        "ime_pinyin.dict.yaml"
    };

    for (const auto& file : requiredFiles) {
        fs::path filePath = fs::path(userDataDir) / file;
        if (!fs::exists(filePath)) {
            return false;
        }
    }

    return true;
}

bool RimeConfig::DeployConfigFiles(const std::string& sourceDir,
                                   const std::string& userDataDir) {
    // 创建用户数据目录
    if (!CreateDirectoryIfNotExists(userDataDir)) {
        return false;
    }

    // 需要复制的配置文件
    std::vector<std::string> configFiles = {
        "default.custom.yaml",
        "ime_pinyin.schema.yaml",
        "ime_pinyin.dict.yaml",
        "melt_eng.schema.yaml",
        "melt_eng.dict.yaml"
    };

    for (const auto& file : configFiles) {
        fs::path srcPath = fs::path(sourceDir) / file;
        fs::path dstPath = fs::path(userDataDir) / file;

        if (fs::exists(srcPath)) {
            if (!CopyFile(srcPath.string(), dstPath.string())) {
                return false;
            }
        }
    }

    return true;
}

bool RimeConfig::DeployDictionaries(const std::string& rimeIceDir,
                                    const std::string& userDataDir) {
    // 创建用户数据目录
    if (!CreateDirectoryIfNotExists(userDataDir)) {
        return false;
    }

    // 复制中文词库目录
    fs::path cnDictsSrc = fs::path(rimeIceDir) / "cn_dicts";
    fs::path cnDictsDst = fs::path(userDataDir) / "cn_dicts";
    if (fs::exists(cnDictsSrc)) {
        if (!CopyDirectory(cnDictsSrc.string(), cnDictsDst.string())) {
            return false;
        }
    }

    // 复制英文词库目录
    fs::path enDictsSrc = fs::path(rimeIceDir) / "en_dicts";
    fs::path enDictsDst = fs::path(userDataDir) / "en_dicts";
    if (fs::exists(enDictsSrc)) {
        if (!CopyDirectory(enDictsSrc.string(), enDictsDst.string())) {
            return false;
        }
    }

    return true;
}

bool RimeConfig::CopyFile(const std::string& src, const std::string& dst) {
    try {
        // 确保目标目录存在
        fs::path dstPath(dst);
        if (dstPath.has_parent_path()) {
            fs::create_directories(dstPath.parent_path());
        }

        fs::copy_file(src, dst, fs::copy_options::overwrite_existing);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool RimeConfig::CopyDirectory(const std::string& src, const std::string& dst) {
    try {
        fs::create_directories(dst);
        fs::copy(src, dst, fs::copy_options::recursive | 
                          fs::copy_options::overwrite_existing);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool RimeConfig::CreateDirectoryIfNotExists(const std::string& path) {
    try {
        if (!fs::exists(path)) {
            fs::create_directories(path);
        }
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

}  // namespace input
}  // namespace ime
