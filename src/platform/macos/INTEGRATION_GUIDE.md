# Squirrel 集成指南

本文档说明如何将跨平台输入法的核心模块集成到 Squirrel (鼠须管) 前端。

## 概述

集成层提供以下功能：
- 用户词频学习和排序优化
- 候选词合并（用户高频词优先）
- 自动学词（连续单字组成词组）
- 输入模式管理
- 菜单栏图标和下拉菜单增强

## 文件结构

```
src/platform/macos/
├── squirrel_integration.h      # C/C++ 头文件（含 C 接口）
├── squirrel_integration.cpp    # C/C++ 实现
├── ImeIntegration.swift        # Swift 封装层（核心功能）
├── SquirrelMenuExtension.swift # Swift 菜单扩展（状态图标和菜单）
├── rime_config/
│   ├── default.custom.yaml     # RIME 默认配置
│   ├── ime_pinyin.schema.yaml  # 简体拼音方案
│   ├── ime_pinyin.dict.yaml    # 词库文件
│   └── squirrel.custom.yaml    # Squirrel 外观配置
└── INTEGRATION_GUIDE.md        # 本文档
```

## 集成步骤

### 1. 添加 Bridging Header

在 Squirrel 项目的 `Squirrel-Bridging-Header.h` 中添加：

```c
// 跨平台输入法集成
#import "squirrel_integration.h"
```

### 2. 添加 Swift 文件

将以下 Swift 文件添加到 Squirrel 项目中：
- `ImeIntegration.swift` - 核心功能封装
- `SquirrelMenuExtension.swift` - 菜单栏扩展

### 3. 链接静态库

在 Xcode 项目设置中：
1. 添加 `libime_platform_macos.a` 到 "Link Binary With Libraries"
2. 添加 `libime_core_*.a` 相关库
3. 添加 SQLite3 框架

### 4. 修改 SquirrelApplicationDelegate

在 `applicationWillFinishLaunching` 中初始化：

```swift
func applicationWillFinishLaunching(_ notification: Notification) {
    // 初始化跨平台输入法集成层
    let userDir = SquirrelApp.userDir.path()
    let sharedDir = Bundle.main.sharedSupportPath ?? ""
    
    if ImeIntegrationManager.shared.initialize(
        userDataPath: userDir,
        sharedDataPath: sharedDir
    ) {
        print("IME Integration initialized successfully")
    } else {
        print("IME Integration initialization failed")
    }
    
    // 初始化菜单扩展
    SquirrelMenuExtension.shared.initialize()
    
    // 设置模式切换回调
    SquirrelMenuExtension.shared.onModeChanged = { [weak self] mode in
        // 同步到 RIME
        if let session = self?.getCurrentSession() {
            let asciiMode = (mode == .english || mode == .tempEnglish)
            self?.rimeAPI.set_option(session, "ascii_mode", asciiMode)
        }
    }
    
    // 原有代码...
    panel = SquirrelPanel(position: .zero)
    addObservers()
}

func applicationWillTerminate(_ notification: Notification) {
    // 关闭菜单扩展
    SquirrelMenuExtension.shared.shutdown()
    
    // 关闭集成层
    ImeIntegrationManager.shared.shutdown()
    
    // 原有代码...
}
```

### 5. 修改 SquirrelInputController

#### 5.1 候选词选择时记录词频

在 `selectCandidate` 方法中添加：

```swift
func selectCandidate(_ index: Int) -> Bool {
    let success = rimeAPI.select_candidate_on_current_page(session, index)
    if success {
        // 获取选中的候选词和拼音
        if let candidate = getCurrentCandidate(at: index),
           let pinyin = getCurrentPinyin() {
            // 记录词频
            ImeIntegrationManager.shared.handleCandidateSelection(
                candidate: candidate,
                pinyin: pinyin,
                isConsecutive: true
            )
        }
        rimeUpdate()
    }
    return success
}
```

#### 5.2 提交时触发自动学词

在 `commitComposition` 方法中添加：

```swift
override func commitComposition(_ sender: Any!) {
    self.client ?= sender as? IMKTextInput
    
    if session != 0 {
        if let input = rimeAPI.get_input(session) {
            commit(string: String(cString: input))
            rimeAPI.clear_composition(session)
            
            // 触发自动学词检查
            ImeIntegrationManager.shared.handleCommitComposition()
        }
    }
}
```

#### 5.3 候选词合并（可选）

如果需要将用户高频词注入到候选词列表前面：

```swift
func showPanel(preedit: String, selRange: NSRange, caretPos: Int, 
               candidates: [String], comments: [String], labels: [String], 
               highlighted: Int, page: Int, lastPage: Bool) {
    
    // 合并用户高频词
    let mergedCandidates: [String]
    if let pinyin = getCurrentPinyin(), !pinyin.isEmpty {
        mergedCandidates = ImeIntegrationManager.shared.mergeCandidates(
            candidates, 
            pinyin: pinyin
        )
    } else {
        mergedCandidates = candidates
    }
    
    // 使用合并后的候选词显示面板
    // ...
}
```

### 6. 复制 RIME 配置文件

将 `rime_config/` 目录下的文件复制到：
- 用户目录：`~/Library/Rime/`
- 或共享目录：`/Library/Input Methods/Squirrel.app/Contents/SharedSupport/`

## API 参考

### ImeIntegrationManager

```swift
// 单例访问
ImeIntegrationManager.shared

// 初始化
func initialize(userDataPath: String?, sharedDataPath: String?) -> Bool

// 关闭
func shutdown()

// 候选词合并
func mergeCandidates(_ candidates: [String], pinyin: String) -> [String]

// 记录选择
func recordSelection(word: String, pinyin: String)
func recordConsecutiveSelection(word: String, pinyin: String)

// 提交完成
func onCommitComplete()

// 输入模式
var inputMode: ImeInputMode { get set }
func toggleInputMode()

// 配置
func getConfig(_ key: String, defaultValue: String) -> String
func setConfig(_ key: String, value: String) -> Bool
```

### SquirrelMenuExtension

```swift
// 单例访问
SquirrelMenuExtension.shared

// 初始化
func initialize()
func shutdown()

// 模式管理
var currentMode: ImeInputMode { get }
func setMode(_ mode: ImeInputMode)
func toggleMode()

// 图标
var statusIconName: String { get }
var statusIcon: NSImage? { get }

// 提示文本
var tooltipText: String { get }
var modeName: String { get }

// 菜单
func createExtendedMenuItems() -> [NSMenuItem]
func addExtendedItems(to menu: NSMenu)
func updateMenuItemStates(in menu: NSMenu)

// 回调
var onModeChanged: ((ImeInputMode) -> Void)?
var onMenuCommand: ((MenuCommand) -> Void)?

// 对话框
func showAboutDialog()
```

### C 接口

如果需要直接使用 C 接口：

```c
int ImeIntegration_Initialize(const char* userDataPath, const char* sharedDataPath);
void ImeIntegration_Shutdown(void);
int ImeIntegration_MergeCandidates(const char** candidates, const char* pinyin, 
                                    char** outBuffer, int bufferSize);
void ImeIntegration_RecordSelection(const char* word, const char* pinyin);
void ImeIntegration_RecordConsecutive(const char* word, const char* pinyin);
void ImeIntegration_OnCommit(void);
int ImeIntegration_GetInputMode(void);
void ImeIntegration_SetInputMode(int mode);
void ImeIntegration_ToggleInputMode(void);
```

## 数据存储

集成层使用 SQLite 数据库存储用户数据：
- 位置：`~/Library/Rime/ime_data.db`
- 包含：词频统计、词库元数据、应用配置

## 注意事项

1. **线程安全**：集成层的所有方法都是线程安全的
2. **内存管理**：Swift 封装层自动处理内存管理
3. **错误处理**：初始化失败时会返回 false，但不会影响 Squirrel 原有功能
4. **性能**：候选词合并操作通常在 1ms 内完成

## 构建说明

### 使用 CMake 构建

```bash
mkdir build && cd build
cmake .. -DPLATFORM_MACOS=ON
make ime_platform_macos
```

### 使用 Xcode 构建

1. 将源文件添加到 Squirrel.xcodeproj
2. 配置 Bridging Header
3. 链接必要的库

## 测试

集成后可以通过以下方式验证：

1. 输入拼音，检查候选词是否正常显示
2. 选择候选词，检查词频是否更新
3. 连续输入单字，检查是否自动学习词组
4. 切换输入模式，检查状态是否正确保存


## 菜单栏集成

### 修改 SquirrelInputController 的 menu() 方法

在 `SquirrelInputController.swift` 中修改 `menu()` 方法以添加扩展菜单项：

```swift
override func menu() -> NSMenu! {
    let menu = NSMenu()
    
    // 添加扩展菜单项（模式切换、关于等）
    SquirrelMenuExtension.shared.addExtendedItems(to: menu)
    
    // 原有菜单项
    let deploy = NSMenuItem(title: NSLocalizedString("Deploy", comment: "Menu item"), 
                            action: #selector(deploy), keyEquivalent: "`")
    deploy.target = self
    deploy.keyEquivalentModifierMask = [.control, .option]
    
    let sync = NSMenuItem(title: NSLocalizedString("Sync user data", comment: "Menu item"), 
                          action: #selector(syncUserData), keyEquivalent: "")
    sync.target = self
    
    let logDir = NSMenuItem(title: NSLocalizedString("Logs...", comment: "Menu item"), 
                            action: #selector(openLogFolder), keyEquivalent: "")
    logDir.target = self
    
    let setting = NSMenuItem(title: NSLocalizedString("Settings...", comment: "Menu item"), 
                             action: #selector(openRimeFolder), keyEquivalent: "")
    setting.target = self
    
    let wiki = NSMenuItem(title: NSLocalizedString("Rime Wiki...", comment: "Menu item"), 
                          action: #selector(openWiki), keyEquivalent: "")
    wiki.target = self
    
    let update = NSMenuItem(title: NSLocalizedString("Check for updates...", comment: "Menu item"), 
                            action: #selector(checkForUpdates), keyEquivalent: "")
    update.target = self

    menu.addItem(NSMenuItem.separator())
    menu.addItem(deploy)
    menu.addItem(sync)
    menu.addItem(logDir)
    menu.addItem(setting)
    menu.addItem(wiki)
    menu.addItem(update)

    return menu
}
```

### 同步 RIME 的 ascii_mode 状态

在 `rimeUpdate()` 方法中同步模式状态：

```swift
func rimeUpdate() {
    // ... 原有代码 ...
    
    var status = RimeStatus_stdbool.rimeStructInit()
    if rimeAPI.get_status(session, &status) {
        // 同步 ascii_mode 到菜单扩展
        let asciiMode = status.is_ascii_mode
        SquirrelMenuExtension.shared.handleAsciiModeChange(asciiMode)
        
        // ... 原有代码 ...
        _ = rimeAPI.free_status(&status)
    }
    
    // ... 原有代码 ...
}
```

### 状态图标更新

如果需要自定义状态栏图标，可以在 `SquirrelApplicationDelegate` 中添加：

```swift
// 监听模式变化并更新图标
SquirrelMenuExtension.shared.onModeChanged = { [weak self] mode in
    // 更新状态栏图标
    if let statusItem = self?.statusItem {
        statusItem.button?.image = SquirrelMenuExtension.shared.statusIcon
        statusItem.button?.toolTip = SquirrelMenuExtension.shared.tooltipText
    }
}
```

## 本地化

菜单扩展使用 `NSLocalizedString` 进行本地化。需要在 `Localizable.xcstrings` 中添加以下字符串：

```json
{
  "Chinese Mode": {
    "localizations": {
      "zh-Hans": { "stringUnit": { "value": "中文模式" } },
      "zh-Hant": { "stringUnit": { "value": "中文模式" } }
    }
  },
  "English Mode": {
    "localizations": {
      "zh-Hans": { "stringUnit": { "value": "英文模式" } },
      "zh-Hant": { "stringUnit": { "value": "英文模式" } }
    }
  },
  "About Cross-Platform IME...": {
    "localizations": {
      "zh-Hans": { "stringUnit": { "value": "关于跨平台输入法..." } },
      "zh-Hant": { "stringUnit": { "value": "關於跨平台輸入法..." } }
    }
  },
  "Cross-Platform IME": {
    "localizations": {
      "zh-Hans": { "stringUnit": { "value": "跨平台输入法" } },
      "zh-Hant": { "stringUnit": { "value": "跨平台輸入法" } }
    }
  },
  "OK": {
    "localizations": {
      "zh-Hans": { "stringUnit": { "value": "确定" } },
      "zh-Hant": { "stringUnit": { "value": "確定" } }
    }
  }
}
```
