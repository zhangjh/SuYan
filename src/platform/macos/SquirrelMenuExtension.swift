// src/platform/macos/SquirrelMenuExtension.swift
// Squirrel 菜单栏扩展 - 增强菜单栏图标和下拉菜单功能

import AppKit
import Foundation

/// 菜单栏图标管理器
/// 提供输入模式状态图标和下拉菜单功能
@objc public final class SquirrelMenuExtension: NSObject {
    
    /// 单例实例
    @objc public static let shared = SquirrelMenuExtension()
    
    /// 当前输入模式
    @objc public private(set) var currentMode: ImeInputMode = .chinese
    
    /// 当前方案名称
    @objc public var schemaName: String = "简体拼音"
    
    /// 是否已初始化
    @objc public private(set) var isInitialized: Bool = false
    
    /// 模式切换回调
    @objc public var onModeChanged: ((ImeInputMode) -> Void)?
    
    /// 菜单命令回调
    @objc public var onMenuCommand: ((MenuCommand) -> Void)?
    
    private override init() {
        super.init()
    }
    
    // MARK: - 初始化
    
    /// 初始化菜单扩展
    @objc public func initialize() {
        guard !isInitialized else { return }
        
        // 从集成层同步当前模式
        if ImeIntegrationManager.shared.isInitialized {
            currentMode = ImeIntegrationManager.shared.inputMode
        }
        
        isInitialized = true
    }
    
    /// 关闭菜单扩展
    @objc public func shutdown() {
        isInitialized = false
    }
    
    // MARK: - 模式管理
    
    /// 设置当前输入模式
    /// - Parameter mode: 新的输入模式
    @objc public func setMode(_ mode: ImeInputMode) {
        guard currentMode != mode else { return }
        
        currentMode = mode
        
        // 同步到集成层
        if ImeIntegrationManager.shared.isInitialized {
            ImeIntegrationManager.shared.inputMode = mode
        }
        
        // 触发回调
        onModeChanged?(mode)
    }
    
    /// 切换输入模式（中文 <-> 英文）
    @objc public func toggleMode() {
        switch currentMode {
        case .chinese:
            setMode(.english)
        case .english, .tempEnglish:
            setMode(.chinese)
        }
    }
    
    // MARK: - 图标
    
    /// 获取当前模式的状态栏图标名称
    @objc public var statusIconName: String {
        switch currentMode {
        case .chinese:
            return "rime"  // 中文模式图标
        case .english:
            return "rime-abc"  // 英文模式图标
        case .tempEnglish:
            return "rime-abc"  // 临时英文模式也使用英文图标
        }
    }
    
    /// 获取当前模式的状态栏图标（NSImage）
    @objc public var statusIcon: NSImage? {
        // 尝试从 bundle 加载图标
        if let image = NSImage(named: statusIconName) {
            return image
        }
        
        // 如果没有图标，创建文字图标
        return createTextIcon(for: currentMode)
    }
    
    /// 创建文字图标
    private func createTextIcon(for mode: ImeInputMode) -> NSImage {
        let text: String
        switch mode {
        case .chinese:
            text = "中"
        case .english:
            text = "英"
        case .tempEnglish:
            text = "临"
        }
        
        let size = NSSize(width: 18, height: 18)
        let image = NSImage(size: size)
        
        image.lockFocus()
        
        let attributes: [NSAttributedString.Key: Any] = [
            .font: NSFont.systemFont(ofSize: 14, weight: .medium),
            .foregroundColor: NSColor.labelColor
        ]
        
        let attributedString = NSAttributedString(string: text, attributes: attributes)
        let stringSize = attributedString.size()
        let point = NSPoint(
            x: (size.width - stringSize.width) / 2,
            y: (size.height - stringSize.height) / 2
        )
        
        attributedString.draw(at: point)
        
        image.unlockFocus()
        image.isTemplate = true
        
        return image
    }
    
    // MARK: - 提示文本
    
    /// 获取状态栏提示文本
    @objc public var tooltipText: String {
        var tooltip = "跨平台输入法"
        tooltip += " - \(modeName)"
        if !schemaName.isEmpty {
            tooltip += " [\(schemaName)]"
        }
        return tooltip
    }
    
    /// 获取当前模式名称
    @objc public var modeName: String {
        switch currentMode {
        case .chinese:
            return "中文"
        case .english:
            return "英文"
        case .tempEnglish:
            return "临时英文"
        }
    }
    
    // MARK: - 菜单
    
    /// 创建扩展菜单项
    /// - Returns: 菜单项数组
    @objc public func createExtendedMenuItems() -> [NSMenuItem] {
        var items: [NSMenuItem] = []
        
        // 中文模式
        let chineseItem = NSMenuItem(
            title: NSLocalizedString("Chinese Mode", comment: "中文模式"),
            action: #selector(handleChineseMode(_:)),
            keyEquivalent: ""
        )
        chineseItem.target = self
        chineseItem.state = (currentMode == .chinese) ? .on : .off
        items.append(chineseItem)
        
        // 英文模式
        let englishItem = NSMenuItem(
            title: NSLocalizedString("English Mode", comment: "英文模式"),
            action: #selector(handleEnglishMode(_:)),
            keyEquivalent: ""
        )
        englishItem.target = self
        englishItem.state = (currentMode == .english || currentMode == .tempEnglish) ? .on : .off
        items.append(englishItem)
        
        // 分隔符
        items.append(NSMenuItem.separator())
        
        // 关于
        let aboutItem = NSMenuItem(
            title: NSLocalizedString("About Cross-Platform IME...", comment: "关于跨平台输入法..."),
            action: #selector(handleAbout(_:)),
            keyEquivalent: ""
        )
        aboutItem.target = self
        items.append(aboutItem)
        
        return items
    }
    
    /// 将扩展菜单项添加到现有菜单
    /// - Parameter menu: 目标菜单
    @objc public func addExtendedItems(to menu: NSMenu) {
        // 在菜单开头添加分隔符
        menu.insertItem(NSMenuItem.separator(), at: 0)
        
        // 添加扩展菜单项
        let extendedItems = createExtendedMenuItems()
        for (index, item) in extendedItems.enumerated() {
            menu.insertItem(item, at: index)
        }
    }
    
    // MARK: - 菜单动作
    
    @objc private func handleChineseMode(_ sender: Any?) {
        setMode(.chinese)
        onMenuCommand?(.switchToChinese)
    }
    
    @objc private func handleEnglishMode(_ sender: Any?) {
        setMode(.english)
        onMenuCommand?(.switchToEnglish)
    }
    
    @objc private func handleAbout(_ sender: Any?) {
        showAboutDialog()
        onMenuCommand?(.about)
    }
    
    // MARK: - 对话框
    
    /// 显示关于对话框
    @objc public func showAboutDialog() {
        let alert = NSAlert()
        alert.messageText = NSLocalizedString("Cross-Platform IME", comment: "跨平台输入法")
        alert.informativeText = """
        版本 1.0.0
        
        基于 RIME 开源引擎开发
        支持 Windows 和 macOS 双平台
        
        功能特性：
        • 简体拼音输入
        • 智能词频学习
        • 自动学词
        • 云端词库同步
        """
        alert.alertStyle = .informational
        alert.addButton(withTitle: NSLocalizedString("OK", comment: "确定"))
        alert.runModal()
    }
}

// MARK: - 菜单命令枚举

/// 菜单命令
@objc public enum MenuCommand: Int {
    case switchToChinese = 0
    case switchToEnglish = 1
    case toggleMode = 2
    case about = 10
    case settings = 11
    case deploy = 12
    case sync = 13
}

// MARK: - Squirrel 集成扩展

extension SquirrelMenuExtension {
    
    /// 处理 Squirrel 的 ascii_mode 选项变化
    /// - Parameter asciiMode: 是否为 ASCII 模式
    @objc public func handleAsciiModeChange(_ asciiMode: Bool) {
        if asciiMode {
            setMode(.english)
        } else {
            setMode(.chinese)
        }
    }
    
    /// 从 RIME 状态同步输入模式
    /// - Parameter session: RIME 会话 ID
    @objc public func syncFromRimeSession(_ session: UInt) {
        // 这个方法需要在 Squirrel 中调用 RIME API 来获取 ascii_mode 状态
        // 由于我们无法直接访问 RIME API，这里只是一个占位符
        // 实际实现需要在 Squirrel 的 SquirrelInputController 中完成
    }
    
    /// 更新菜单项状态
    /// - Parameter menu: 要更新的菜单
    @objc public func updateMenuItemStates(in menu: NSMenu) {
        for item in menu.items {
            if item.action == #selector(handleChineseMode(_:)) {
                item.state = (currentMode == .chinese) ? .on : .off
            } else if item.action == #selector(handleEnglishMode(_:)) {
                item.state = (currentMode == .english || currentMode == .tempEnglish) ? .on : .off
            }
        }
    }
}

// MARK: - 状态栏图标辅助

extension SquirrelMenuExtension {
    
    /// 创建带有模式指示的状态栏图标
    /// - Parameters:
    ///   - baseImage: 基础图标
    ///   - mode: 输入模式
    /// - Returns: 带有模式指示的图标
    @objc public func createStatusBarIcon(baseImage: NSImage?, mode: ImeInputMode) -> NSImage {
        guard let base = baseImage else {
            return createTextIcon(for: mode)
        }
        
        // 如果有基础图标，直接返回（可以在这里添加模式指示器）
        return base
    }
    
    /// 获取模式切换的快捷键描述
    @objc public var modeToggleShortcut: String {
        return "Shift"
    }
}
