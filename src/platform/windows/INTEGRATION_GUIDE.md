# Weasel 集成指南

本文档说明如何将跨平台输入法的核心模块集成到 Weasel (小狼毫) 前端。

## 概述

我们的集成策略是**最小改动原则**：
1. 保持 Weasel 原有代码结构不变
2. 通过扩展钩子注入我们的功能
3. 核心功能（词频管理、候选词合并、自动学词）通过独立库提供

## 集成步骤

### 1. 准备工作

确保已初始化 submodule：
```bash
git submodule update --init --recursive
```

### 2. 修改 RimeWithWeasel.cpp

在 `deps/weasel/RimeWithWeasel/RimeWithWeasel.cpp` 中添加以下修改：

#### 2.1 添加头文件引用

在文件开头添加：
```cpp
#include "platform/windows/weasel_handler_ext.h"

using ime::platform::windows::WeaselHandlerExtension;
```

#### 2.2 在 Initialize 中初始化扩展

在 `RimeWithWeaselHandler::Initialize()` 函数末尾添加：
```cpp
void RimeWithWeaselHandler::Initialize() {
    // ... 原有代码 ...

    // 初始化跨平台输入法扩展
    WeaselHandlerExtension::Instance().OnInitialize(
        WeaselUserDataPath().c_str(),
        WeaselSharedDataPath().c_str()
    );
}
```

#### 2.3 在 Finalize 中清理扩展

在 `RimeWithWeaselHandler::Finalize()` 函数开头添加：
```cpp
void RimeWithWeaselHandler::Finalize() {
    // 清理跨平台输入法扩展
    WeaselHandlerExtension::Instance().OnFinalize();

    // ... 原有代码 ...
}
```

#### 2.4 在获取候选词后合并用户高频词

在 `_GetCandidateInfo` 函数中，获取候选词后添加合并逻辑：
```cpp
void RimeWithWeaselHandler::_GetCandidateInfo(CandidateInfo& cinfo,
                                              RimeContext& ctx) {
    // ... 原有获取候选词代码 ...

    // 合并用户高频词
    if (ctx.composition.preedit) {
        std::vector<std::string> candidates;
        for (int i = 0; i < ctx.menu.num_candidates; ++i) {
            candidates.push_back(ctx.menu.candidates[i].text);
        }

        std::string pinyin = ctx.composition.preedit;
        WeaselHandlerExtension::Instance().OnCandidatesReady(candidates, pinyin);

        // 更新候选词显示（如果顺序改变）
        // 注意：这里需要根据实际情况调整
    }
}
```

#### 2.5 在选择候选词后记录词频

在 `SelectCandidateOnCurrentPage` 函数中添加：
```cpp
void RimeWithWeaselHandler::SelectCandidateOnCurrentPage(
    size_t index,
    WeaselSessionId ipc_id) {
    // ... 原有代码 ...

    // 记录词选择
    RIME_STRUCT(RimeContext, ctx);
    if (rime_api->get_context(to_session_id(ipc_id), &ctx)) {
        if (index < ctx.menu.num_candidates) {
            std::string word = ctx.menu.candidates[index].text;
            std::string pinyin = ctx.composition.preedit ? ctx.composition.preedit : "";
            WeaselHandlerExtension::Instance().OnCandidateSelected(word, pinyin, index);
        }
        rime_api->free_context(&ctx);
    }
}
```

#### 2.6 在提交文本后触发自动学词

在 `_Respond` 函数中，检测到 commit 时添加：
```cpp
bool RimeWithWeaselHandler::_Respond(WeaselSessionId ipc_id, EatLine eat) {
    // ... 原有代码 ...

    RIME_STRUCT(RimeCommit, commit);
    if (rime_api->get_commit(session_id, &commit)) {
        // ... 原有提交处理 ...

        // 通知扩展层
        WeaselHandlerExtension::Instance().OnTextCommitted(commit.text);

        rime_api->free_commit(&commit);
    }

    // ... 原有代码 ...
}
```

### 3. 修改 Visual Studio 项目

#### 3.1 添加库引用

在 `RimeWithWeasel` 项目中：
1. 右键项目 → 属性 → 链接器 → 输入 → 附加依赖项
2. 添加：`ime_weasel_integration.lib`

#### 3.2 添加包含目录

1. 右键项目 → 属性 → C/C++ → 常规 → 附加包含目录
2. 添加：`$(SolutionDir)\..\src\platform\windows`
3. 添加：`$(SolutionDir)\..\src`

### 4. 复制配置文件

将 `src/platform/windows/rime_config/` 目录下的配置文件复制到 Weasel 的数据目录：
- 用户目录：`%APPDATA%\Rime\`
- 共享目录：`<安装目录>\data\`

需要复制的文件：
- `default.custom.yaml`
- `ime_pinyin.schema.yaml`
- `ime_pinyin.dict.yaml`
- `weasel.custom.yaml`

### 5. 构建和测试

1. 在 Visual Studio 中打开 `deps/weasel/weasel.sln`
2. 选择 Release 配置
3. 构建整个解决方案
4. 运行 WeaselServer.exe 测试

## 功能验证

### 基础输入功能
- [ ] 输入拼音显示候选词
- [ ] 数字键选择候选词
- [ ] 空格键选择首选
- [ ] 回车键提交原文
- [ ] Esc 键清空输入
- [ ] Backspace 退格编辑

### 用户词频功能
- [ ] 选择候选词后词频增加
- [ ] 高频词排在前面
- [ ] 重启后词频保留

### 自动学词功能
- [ ] 连续选择单字形成词组
- [ ] 新词组出现在候选列表

### 中英文切换
- [ ] Shift 键切换模式
- [ ] 大写字母开头临时英文
- [ ] 状态图标正确显示

## 故障排除

### 候选词不显示用户高频词
1. 检查 SQLite 数据库是否正确创建
2. 检查 `ime_data.db` 文件位置
3. 查看日志文件

### 词频不保存
1. 检查数据库写入权限
2. 确认 `OnCandidateSelected` 被正确调用

### 自动学词不工作
1. 确认 `enableAutoLearn` 配置为 true
2. 检查连续选择是否被正确记录

### 托盘图标不更新
1. 确认 `WeaselTrayExtension` 已初始化
2. 检查 `SetCurrentMode` 是否被正确调用
3. 确认 `OnUpdateUI` 回调已设置

## 托盘图标扩展集成

### 6. 集成托盘图标扩展

在 `WeaselServerApp.cpp` 中添加托盘图标扩展：

#### 6.1 添加头文件

```cpp
#include "platform/windows/weasel_tray_ext.h"

using ime::platform::windows::WeaselTrayExtension;
```

#### 6.2 在 Run 函数中初始化

```cpp
int WeaselServerApp::Run() {
    // ... 原有代码 ...

    // 初始化托盘图标扩展
    WeaselTrayExtension::Instance().Initialize();

    // 设置菜单命令回调
    WeaselTrayExtension::Instance().SetMenuCommandCallback(
        [this](UINT menuId) {
            // 处理扩展菜单命令
            if (menuId == ID_WEASELTRAY_ENABLE_ASCII) {
                WeaselTrayExtension::Instance().SetCurrentMode(
                    ime::platform::windows::TrayInputMode::English);
            } else if (menuId == ID_WEASELTRAY_DISABLE_ASCII) {
                WeaselTrayExtension::Instance().SetCurrentMode(
                    ime::platform::windows::TrayInputMode::Chinese);
            }
        }
    );

    // ... 原有代码 ...
}
```

#### 6.3 在托盘图标刷新时同步状态

在 `WeaselTrayIcon::Refresh()` 中添加：

```cpp
void WeaselTrayIcon::Refresh() {
    // ... 原有代码 ...

    // 同步到扩展层
    auto& ext = WeaselTrayExtension::Instance();
    if (ext.IsInitialized()) {
        if (m_status.ascii_mode) {
            ext.SetCurrentMode(ime::platform::windows::TrayInputMode::English);
        } else {
            ext.SetCurrentMode(ime::platform::windows::TrayInputMode::Chinese);
        }

        // 更新提示文本
        SetTooltipText(ext.GetTooltipText().c_str());
    }
}
```

#### 6.4 处理扩展菜单命令

在 `WeaselServerApp::SetupMenuHandlers()` 中添加扩展菜单处理：

```cpp
void WeaselServerApp::SetupMenuHandlers() {
    // ... 原有代码 ...

    // 扩展菜单处理
    m_server.AddMenuHandler(ID_TRAY_EXT_CHINESE_MODE, [this]() {
        WeaselTrayExtension::Instance().HandleMenuCommand(ID_TRAY_EXT_CHINESE_MODE);
        return true;
    });

    m_server.AddMenuHandler(ID_TRAY_EXT_ENGLISH_MODE, [this]() {
        WeaselTrayExtension::Instance().HandleMenuCommand(ID_TRAY_EXT_ENGLISH_MODE);
        return true;
    });

    m_server.AddMenuHandler(ID_TRAY_EXT_ABOUT, [this]() {
        WeaselTrayExtension::Instance().HandleMenuCommand(ID_TRAY_EXT_ABOUT);
        return true;
    });
}
```

## 相关文件

- `weasel_integration.h/cpp` - 集成层主接口
- `weasel_handler_ext.h/cpp` - Weasel 处理器扩展
- `weasel_tray_ext.h/cpp` - 托盘图标扩展
- `rime_config/` - RIME 配置文件

## 扩展菜单 ID

扩展菜单项使用以下 ID（从 50000 开始，避免与 Weasel 原有 ID 冲突）：

| ID | 值 | 说明 |
|----|-----|------|
| ID_TRAY_EXT_BASE | 50000 | 基础 ID |
| ID_TRAY_EXT_TOGGLE_MODE | 50001 | 切换模式 |
| ID_TRAY_EXT_CHINESE_MODE | 50002 | 中文模式 |
| ID_TRAY_EXT_ENGLISH_MODE | 50003 | 英文模式 |
| ID_TRAY_EXT_ABOUT | 50010 | 关于对话框 |

## 图标资源

如需自定义图标，可以替换以下资源：

| 资源 ID | 说明 |
|---------|------|
| IDI_ZH | 中文模式图标 |
| IDI_EN | 英文模式图标 |
| IDI_RELOAD | 部署中图标 |

图标文件位置：`deps/weasel/WeaselServer/`
