// src/platform/macos/ImeIntegration.swift
// Swift 封装层 - 提供 Swift 友好的 API 供 Squirrel 调用

import Foundation

/// 输入模式枚举
@objc public enum ImeInputMode: Int {
    case chinese = 0    // 中文模式
    case english = 1    // 英文模式
    case tempEnglish = 2 // 临时英文模式
}

/// IME 集成管理器
/// 提供词频学习、候选词合并、自动学词等功能
@objc public final class ImeIntegrationManager: NSObject {
    
    /// 单例实例
    @objc public static let shared = ImeIntegrationManager()
    
    /// 是否已初始化
    @objc public private(set) var isInitialized: Bool = false
    
    private override init() {
        super.init()
    }
    
    // MARK: - 生命周期
    
    /// 初始化集成层
    /// - Parameters:
    ///   - userDataPath: 用户数据目录，默认为 ~/Library/Rime
    ///   - sharedDataPath: 共享数据目录
    /// - Returns: 是否成功
    @objc public func initialize(userDataPath: String? = nil, 
                                  sharedDataPath: String? = nil) -> Bool {
        let userPath = userDataPath ?? defaultUserDataPath()
        let sharedPath = sharedDataPath ?? defaultSharedDataPath()
        
        let result = ImeIntegration_Initialize(
            userPath.cString(using: .utf8),
            sharedPath.cString(using: .utf8)
        )
        
        isInitialized = (result == 0)
        return isInitialized
    }
    
    /// 关闭集成层
    @objc public func shutdown() {
        ImeIntegration_Shutdown()
        isInitialized = false
    }
    
    // MARK: - 候选词处理
    
    /// 合并用户高频词到候选词列表
    /// - Parameters:
    ///   - candidates: librime 返回的候选词列表
    ///   - pinyin: 当前输入的拼音
    /// - Returns: 合并后的候选词列表
    @objc public func mergeCandidates(_ candidates: [String], 
                                       pinyin: String) -> [String] {
        guard isInitialized else { return candidates }
        
        // 转换为 C 字符串数组
        var cStrings = candidates.map { strdup($0) }
        cStrings.append(nil)  // NULL 结尾
        
        // 分配输出缓冲区
        let bufferSize = max(candidates.count + 10, 20)
        var outBuffer = [UnsafeMutablePointer<CChar>?](repeating: nil, count: bufferSize)
        
        let count = ImeIntegration_MergeCandidates(
            &cStrings,
            pinyin.cString(using: .utf8),
            &outBuffer,
            Int32(bufferSize)
        )
        
        // 释放输入字符串
        for ptr in cStrings where ptr != nil {
            free(ptr)
        }
        
        // 转换结果
        var result = [String]()
        for i in 0..<Int(count) {
            if let ptr = outBuffer[i] {
                result.append(String(cString: ptr))
            }
        }
        
        // 释放输出缓冲区
        ImeIntegration_FreeMergedCandidates(&outBuffer, count)
        
        return result
    }
    
    // MARK: - 词频记录
    
    /// 记录用户选择了某个候选词
    /// - Parameters:
    ///   - word: 选择的词
    ///   - pinyin: 对应的拼音
    @objc public func recordSelection(word: String, pinyin: String) {
        guard isInitialized else { return }
        ImeIntegration_RecordSelection(
            word.cString(using: .utf8),
            pinyin.cString(using: .utf8)
        )
    }
    
    /// 记录连续选择（用于自动学词）
    /// - Parameters:
    ///   - word: 选择的词
    ///   - pinyin: 对应的拼音
    @objc public func recordConsecutiveSelection(word: String, pinyin: String) {
        guard isInitialized else { return }
        ImeIntegration_RecordConsecutive(
            word.cString(using: .utf8),
            pinyin.cString(using: .utf8)
        )
    }
    
    /// 提交完成（触发自动学词检查）
    @objc public func onCommitComplete() {
        guard isInitialized else { return }
        ImeIntegration_OnCommit()
    }
    
    // MARK: - 输入模式
    
    /// 获取当前输入模式
    @objc public var inputMode: ImeInputMode {
        get {
            let mode = ImeIntegration_GetInputMode()
            return ImeInputMode(rawValue: Int(mode)) ?? .chinese
        }
        set {
            ImeIntegration_SetInputMode(Int32(newValue.rawValue))
        }
    }
    
    /// 切换输入模式
    @objc public func toggleInputMode() {
        ImeIntegration_ToggleInputMode()
    }
    
    // MARK: - 配置
    
    /// 获取配置值
    /// - Parameters:
    ///   - key: 配置键
    ///   - defaultValue: 默认值
    /// - Returns: 配置值
    @objc public func getConfig(_ key: String, defaultValue: String = "") -> String {
        guard isInitialized else { return defaultValue }
        
        var buffer = [CChar](repeating: 0, count: 1024)
        _ = ImeIntegration_GetConfig(
            key.cString(using: .utf8),
            defaultValue.cString(using: .utf8),
            &buffer,
            Int32(buffer.count)
        )
        
        return String(cString: buffer)
    }
    
    /// 设置配置值
    /// - Parameters:
    ///   - key: 配置键
    ///   - value: 配置值
    /// - Returns: 是否成功
    @objc @discardableResult
    public func setConfig(_ key: String, value: String) -> Bool {
        guard isInitialized else { return false }
        
        let result = ImeIntegration_SetConfig(
            key.cString(using: .utf8),
            value.cString(using: .utf8)
        )
        
        return result == 0
    }
    
    // MARK: - 私有方法
    
    private func defaultUserDataPath() -> String {
        let homeDir = FileManager.default.homeDirectoryForCurrentUser
        return homeDir.appendingPathComponent("Library/Rime").path
    }
    
    private func defaultSharedDataPath() -> String {
        return "/Library/Input Methods/Squirrel.app/Contents/SharedSupport"
    }
}

// MARK: - Squirrel 集成扩展

extension ImeIntegrationManager {
    
    /// 处理候选词选择（集成到 Squirrel 的 selectCandidate 流程）
    /// - Parameters:
    ///   - candidate: 选择的候选词
    ///   - pinyin: 对应的拼音
    ///   - isConsecutive: 是否是连续选择（用于自动学词）
    @objc public func handleCandidateSelection(candidate: String, 
                                                pinyin: String,
                                                isConsecutive: Bool = true) {
        if isConsecutive {
            recordConsecutiveSelection(word: candidate, pinyin: pinyin)
        } else {
            recordSelection(word: candidate, pinyin: pinyin)
        }
    }
    
    /// 处理提交完成（集成到 Squirrel 的 commitComposition 流程）
    @objc public func handleCommitComposition() {
        onCommitComplete()
    }
    
    /// 获取输入模式显示名称
    @objc public var inputModeDisplayName: String {
        switch inputMode {
        case .chinese:
            return "中"
        case .english:
            return "英"
        case .tempEnglish:
            return "临"
        }
    }
    
    /// 获取输入模式完整名称
    @objc public var inputModeFullName: String {
        switch inputMode {
        case .chinese:
            return "中文模式"
        case .english:
            return "英文模式"
        case .tempEnglish:
            return "临时英文模式"
        }
    }
}
