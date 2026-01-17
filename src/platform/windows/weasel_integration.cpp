// src/platform/windows/weasel_integration.cpp
// Weasel 集成层实现

#ifdef PLATFORM_WINDOWS

#include "weasel_integration.h"

#include <windows.h>
#include <shlobj.h>
#include <filesystem>
#include <sstream>

#include "core/storage/sqlite_storage.h"
#include "core/frequency/frequency_manager_impl.h"
#include "core/input/candidate_merger.h"
#include "core/learning/auto_learner_impl.h"

namespace fs = std::filesystem;

namespace ime {
namespace platform {
namespace windows {

// ==================== WeaselIntegrationConfig ====================

WeaselIntegrationConfig WeaselIntegrationConfig::Default() {
    return WeaselIntegrationConfig{
        .userDataPath = L"",
        .sharedDataPath = L"",
        .logPath = L"",
        .enableCloudSync = true,
        .enableAutoLearn = true,
        .pageSize = 9
    };
}

// ==================== WeaselIntegration ====================

WeaselIntegration& WeaselIntegration::Instance() {
    static WeaselIntegration instance;
    return instance;
}

WeaselIntegration::WeaselIntegration()
    : initialized_(false),
      currentMode_(input::InputMode::Chinese) {
}

WeaselIntegration::~WeaselIntegration() {
    Shutdown();
}

bool WeaselIntegration::Initialize(const WeaselIntegrationConfig& config) {
    if (initialized_) {
        return true;
    }

    config_ = config;

    // 初始化存储层
    if (!InitializeStorage(config.userDataPath)) {
        return false;
    }

    // 初始化词频管理器
    if (!InitializeFrequencyManager()) {
        return false;
    }

    // 初始化候选词合并器
    candidateMerger_ = std::make_unique<input::CandidateMerger>(storage_.get());
    input::MergeConfig mergeConfig = input::MergeConfig::Default();
    mergeConfig.pageSize = config.pageSize;
    candidateMerger_->SetConfig(mergeConfig);

    // 初始化自动学词
    if (config.enableAutoLearn) {
        if (!InitializeAutoLearner()) {
            // 自动学词失败不影响主功能
        }
    }

    // 从存储加载输入模式
    std::string modeStr = storage_->GetConfig("input.default_mode", "chinese");
    if (modeStr == "english") {
        currentMode_ = input::InputMode::English;
    } else {
        currentMode_ = input::InputMode::Chinese;
    }

    initialized_ = true;
    return true;
}

void WeaselIntegration::Shutdown() {
    if (!initialized_) {
        return;
    }

    // 保存输入模式
    if (storage_) {
        std::string modeStr = (currentMode_ == input::InputMode::English) 
                              ? "english" : "chinese";
        storage_->SetConfig("input.default_mode", modeStr);
    }

    // 清理资源
    autoLearner_.reset();
    candidateMerger_.reset();
    frequencyManager_.reset();

    if (storage_) {
        storage_->Close();
        storage_.reset();
    }

    initialized_ = false;
}

bool WeaselIntegration::InitializeStorage(const std::wstring& userDataPath) {
    // 构建数据库路径
    fs::path dbPath;
    if (!userDataPath.empty()) {
        dbPath = fs::path(userDataPath) / L"ime_data.db";
    } else {
        // 使用默认路径
        wchar_t appData[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appData))) {
            dbPath = fs::path(appData) / L"Rime" / L"ime_data.db";
        } else {
            return false;
        }
    }

    // 确保目录存在
    fs::create_directories(dbPath.parent_path());

    // 创建存储实例
    storage_ = std::make_unique<storage::SqliteStorage>(dbPath.string());
    if (!storage_->Initialize()) {
        storage_.reset();
        return false;
    }

    return true;
}

bool WeaselIntegration::InitializeFrequencyManager() {
    frequencyManager_ = std::make_unique<frequency::FrequencyManagerImpl>(
        storage_.get());
    return frequencyManager_->Initialize();
}

bool WeaselIntegration::InitializeAutoLearner() {
    autoLearner_ = std::make_unique<learning::AutoLearnerImpl>(storage_.get());
    return autoLearner_->Initialize();
}

std::vector<std::string> WeaselIntegration::MergeCandidates(
    const std::vector<std::string>& rimeCandidates,
    const std::string& pinyin) {
    
    if (!initialized_ || !candidateMerger_) {
        return rimeCandidates;
    }

    // 将字符串列表转换为 CandidateWord 列表
    std::vector<input::CandidateWord> rimeWords;
    rimeWords.reserve(rimeCandidates.size());
    for (const auto& text : rimeCandidates) {
        input::CandidateWord word;
        word.text = text;
        word.pinyin = pinyin;
        word.isUserWord = false;
        rimeWords.push_back(word);
    }

    // 合并
    auto merged = candidateMerger_->Merge(rimeWords, pinyin);

    // 转换回字符串列表
    std::vector<std::string> result;
    result.reserve(merged.size());
    for (const auto& word : merged) {
        result.push_back(word.text);
    }

    return result;
}

std::vector<std::string> WeaselIntegration::GetUserTopWords(
    const std::string& pinyin, int limit) {
    
    if (!initialized_ || !candidateMerger_) {
        return {};
    }

    auto userWords = candidateMerger_->QueryUserWords(pinyin, limit);
    
    std::vector<std::string> result;
    result.reserve(userWords.size());
    for (const auto& word : userWords) {
        result.push_back(word.text);
    }

    return result;
}

void WeaselIntegration::RecordWordSelection(const std::string& word,
                                            const std::string& pinyin) {
    if (!initialized_) {
        return;
    }

    // 更新词频
    if (frequencyManager_) {
        frequencyManager_->RecordWordSelection(word, pinyin);
    }
}

void WeaselIntegration::RecordConsecutiveSelection(const std::string& word,
                                                   const std::string& pinyin) {
    if (!initialized_) {
        return;
    }

    // 记录词频
    RecordWordSelection(word, pinyin);

    // 记录到自动学词器
    if (autoLearner_) {
        autoLearner_->RecordInput(word, pinyin);
    }
}

void WeaselIntegration::OnCommitComplete() {
    if (!initialized_ || !autoLearner_) {
        return;
    }

    // 触发自动学词检查（处理候选词）
    autoLearner_->ProcessCandidates();
}

input::InputMode WeaselIntegration::GetInputMode() const {
    return currentMode_;
}

void WeaselIntegration::SetInputMode(input::InputMode mode) {
    currentMode_ = mode;

    // 持久化
    if (initialized_ && storage_) {
        std::string modeStr;
        switch (mode) {
            case input::InputMode::English:
                modeStr = "english";
                break;
            case input::InputMode::TempEnglish:
                modeStr = "temp_english";
                break;
            default:
                modeStr = "chinese";
                break;
        }
        storage_->SetConfig("input.current_mode", modeStr);
    }
}

void WeaselIntegration::ToggleInputMode() {
    if (currentMode_ == input::InputMode::Chinese) {
        SetInputMode(input::InputMode::English);
    } else {
        SetInputMode(input::InputMode::Chinese);
    }
}

std::string WeaselIntegration::GetConfig(const std::string& key,
                                         const std::string& defaultValue) {
    if (!initialized_ || !storage_) {
        return defaultValue;
    }
    return storage_->GetConfig(key, defaultValue);
}

bool WeaselIntegration::SetConfig(const std::string& key,
                                  const std::string& value) {
    if (!initialized_ || !storage_) {
        return false;
    }
    return storage_->SetConfig(key, value);
}

std::vector<std::string> WeaselIntegration::GetEnabledDictionaries() {
    if (!initialized_ || !storage_) {
        return {};
    }

    auto dicts = storage_->GetEnabledDictionaries();
    std::vector<std::string> result;
    result.reserve(dicts.size());
    for (const auto& dict : dicts) {
        result.push_back(dict.id);
    }
    return result;
}

bool WeaselIntegration::SetDictionaryEnabled(const std::string& dictId,
                                             bool enabled) {
    if (!initialized_ || !storage_) {
        return false;
    }
    return storage_->SetDictionaryEnabled(dictId, enabled);
}

// ==================== C 接口实现 ====================

extern "C" {

int ImeIntegration_Initialize(const wchar_t* userDataPath,
                               const wchar_t* sharedDataPath) {
    WeaselIntegrationConfig config = WeaselIntegrationConfig::Default();
    if (userDataPath) {
        config.userDataPath = userDataPath;
    }
    if (sharedDataPath) {
        config.sharedDataPath = sharedDataPath;
    }

    return WeaselIntegration::Instance().Initialize(config) ? 0 : -1;
}

void ImeIntegration_Shutdown() {
    WeaselIntegration::Instance().Shutdown();
}

int ImeIntegration_MergeCandidates(const char** candidates,
                                    const char* pinyin,
                                    char** outBuffer,
                                    int bufferSize) {
    if (!candidates || !pinyin || !outBuffer || bufferSize <= 0) {
        return 0;
    }

    // 收集输入候选词
    std::vector<std::string> inputCandidates;
    for (int i = 0; candidates[i] != nullptr; ++i) {
        inputCandidates.push_back(candidates[i]);
    }

    // 合并
    auto merged = WeaselIntegration::Instance().MergeCandidates(
        inputCandidates, pinyin);

    // 输出结果
    int count = std::min(static_cast<int>(merged.size()), bufferSize);
    for (int i = 0; i < count; ++i) {
        // 注意：调用者需要负责释放内存
        outBuffer[i] = _strdup(merged[i].c_str());
    }

    return count;
}

void ImeIntegration_RecordSelection(const char* word, const char* pinyin) {
    if (!word || !pinyin) {
        return;
    }
    WeaselIntegration::Instance().RecordWordSelection(word, pinyin);
}

void ImeIntegration_RecordConsecutive(const char* word, const char* pinyin) {
    if (!word || !pinyin) {
        return;
    }
    WeaselIntegration::Instance().RecordConsecutiveSelection(word, pinyin);
}

void ImeIntegration_OnCommit() {
    WeaselIntegration::Instance().OnCommitComplete();
}

int ImeIntegration_GetInputMode() {
    auto mode = WeaselIntegration::Instance().GetInputMode();
    switch (mode) {
        case input::InputMode::English:
            return 1;
        case input::InputMode::TempEnglish:
            return 2;
        default:
            return 0;
    }
}

void ImeIntegration_SetInputMode(int mode) {
    input::InputMode inputMode;
    switch (mode) {
        case 1:
            inputMode = input::InputMode::English;
            break;
        case 2:
            inputMode = input::InputMode::TempEnglish;
            break;
        default:
            inputMode = input::InputMode::Chinese;
            break;
    }
    WeaselIntegration::Instance().SetInputMode(inputMode);
}

void ImeIntegration_ToggleInputMode() {
    WeaselIntegration::Instance().ToggleInputMode();
}

}  // extern "C"

}  // namespace windows
}  // namespace platform
}  // namespace ime

#endif  // PLATFORM_WINDOWS
