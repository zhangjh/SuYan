# Requirements Document

## Introduction

本项目旨在开发一款跨平台（Windows + macOS）中英文输入法，基于 RIME 开源引擎二次开发。一期 MVP 聚焦于基础拼音输入功能和云端词库自动更新，二期将扩展自定义词库、皮肤、语音输入和账号体系。

## Glossary

- **IME_Core**: 输入法核心引擎，基于 librime 封装，负责拼音解析、候选词生成、词频学习等核心功能
- **Platform_Adapter**: 平台适配层，负责与操作系统输入法框架交互（Windows IME Framework / macOS Input Method Kit）
- **Candidate_Window**: 候选词窗口，显示拼音输入的候选词列表供用户选择
- **Local_Dictionary**: 本地词库，包含内置词库和从云端下载的词库，存储在用户本地
- **Cloud_Dictionary_Service**: 云端词库服务，提供官方词库的版本管理和下载服务
- **User_Frequency_Data**: 用户词频数据，记录用户的输入习惯用于候选词排序优化
- **Dictionary_Package**: 词包，一组词条的集合，可独立分发和更新

## Scope

### 一期 MVP 范围

- 基础拼音输入（简体中文）
- 英文直接输入
- 内置常用词库
- 本地用户词频学习
- 云端词库下载与自动更新
- Windows 和 macOS 双平台支持

### 二期范围（本文档仅记录，不作为一期需求）

- 用户自定义词包管理
- 自定义皮肤/主题
- 语音输入支持
- 用户登录体系
- 复制粘贴板

---

## Requirements

### Requirement 1: 拼音输入

**User Story:** 作为用户，我希望通过输入拼音来输入中文，以便快速准确地输入中文内容。

#### Acceptance Criteria

1. WHEN 用户在任意文本输入框中输入拼音字母 THEN IME_Core SHALL 实时解析拼音并在 Candidate_Window 中显示对应的候选词列表
2. WHEN 用户输入的拼音可以匹配多个汉字或词组 THEN IME_Core SHALL 按照词频和用户习惯对候选词进行排序
3. WHEN 用户按下数字键（1-9）或点击候选词 THEN IME_Core SHALL 将选中的候选词提交到当前输入框
4. WHEN 用户按下空格键 THEN IME_Core SHALL 提交当前排序第一的候选词
5. WHEN 用户按下 Enter 键 THEN IME_Core SHALL 将当前输入的拼音原文提交到输入框
6. WHEN 用户按下 Escape 键 THEN IME_Core SHALL 清空当前输入状态并关闭 Candidate_Window
7. WHEN 用户按下 Backspace 键 THEN IME_Core SHALL 删除最后一个输入的拼音字符并更新候选词列表
8. WHEN 拼音输入超过单个汉字的长度 THEN IME_Core SHALL 支持整句输入并提供词组和短语候选

---

### Requirement 2: 英文输入

**User Story:** 作为用户，我希望能够直接输入英文，以便在中英文混合场景下流畅输入。

#### Acceptance Criteria

1. WHEN 用户切换到英文输入模式 THEN IME_Core SHALL 将所有键盘输入直接传递到应用程序
2. WHEN 用户按下 Shift 键 THEN IME_Core SHALL 在中文和英文输入模式之间切换
3. WHEN 用户在中文模式下输入以大写字母开头的内容 THEN IME_Core SHALL 自动切换到临时英文模式直到输入完成
4. WHILE 处于英文输入模式 THEN Candidate_Window SHALL 不显示

---

### Requirement 3: 候选词窗口

**User Story:** 作为用户，我希望看到清晰的候选词列表，以便快速选择正确的词。

#### Acceptance Criteria

1. WHEN 用户开始输入拼音 THEN Candidate_Window SHALL 在光标附近显示
2. WHEN 候选词数量超过单页显示数量 THEN Candidate_Window SHALL 支持翻页（Page Up/Page Down 或 +/- 键）
3. WHEN 用户移动光标到其他位置 THEN Candidate_Window SHALL 跟随光标位置更新
4. WHEN 候选词列表为空 THEN Candidate_Window SHALL 不显示
5. THE Candidate_Window SHALL 显示每个候选词对应的选择快捷键（1-9）
6. THE Candidate_Window SHALL 在 Windows 和 macOS 上使用各自的系统原生 UI 风格

---

### Requirement 4: 本地词库管理

**User Story:** 作为用户，我希望输入法内置常用词库，以便开箱即用地输入常见词汇。

#### Acceptance Criteria

1. THE Local_Dictionary SHALL 包含至少 50 万条常用简体中文词条（基于 rime-ice 雾凇拼音词库）
2. THE Local_Dictionary SHALL 包含日常用语、网络流行语、常见行业术语等分类词条
3. WHEN 输入法首次安装 THEN Local_Dictionary SHALL 自动加载 rime-ice 内置词库
4. THE IME_Core SHALL 支持同时加载多个 Dictionary_Package 并合并查询结果
5. WHEN 多个 Dictionary_Package 包含相同词条 THEN IME_Core SHALL 使用最高优先级词包的词频数据

---

### Requirement 5: 用户词频学习

**User Story:** 作为用户，我希望输入法能学习我的输入习惯，以便常用词排在前面减少选择次数。

#### Acceptance Criteria

1. WHEN 用户选择某个候选词 THEN IME_Core SHALL 更新该词在 User_Frequency_Data 中的使用频率
2. WHEN 生成候选词列表 THEN IME_Core SHALL 结合 Local_Dictionary 词频和 User_Frequency_Data 进行综合排序
3. THE User_Frequency_Data SHALL 持久化存储在本地，重启后保留
4. WHEN 用户连续输入形成新的词组 THEN IME_Core SHALL 自动学习该词组并添加到用户词库

---

### Requirement 6: 云端词库服务

**User Story:** 作为用户，我希望输入法能自动获取最新的官方词库更新，以便使用最新的词汇。

#### Acceptance Criteria

1. THE Cloud_Dictionary_Service SHALL 提供词库版本查询接口
2. THE Cloud_Dictionary_Service SHALL 提供词库下载接口，支持增量更新和全量下载
3. WHEN 输入法启动时 THEN IME_Core SHALL 向 Cloud_Dictionary_Service 查询词库版本
4. WHEN 检测到云端词库版本高于本地版本 THEN IME_Core SHALL 在后台静默下载更新
5. WHEN 词库下载完成 THEN IME_Core SHALL 自动加载新词库，无需重启输入法
6. IF 云端服务不可用 THEN IME_Core SHALL 继续使用本地词库正常工作
7. THE Cloud_Dictionary_Service SHALL 支持按词包类型分类（基础词库、行业词库等）

---

### Requirement 7: 平台适配

**User Story:** 作为用户，我希望在 Windows 和 macOS 上获得一致的输入体验。

#### Acceptance Criteria

1. THE Platform_Adapter SHALL 在 Windows 上通过 TSF (Text Services Framework) 与系统集成
2. THE Platform_Adapter SHALL 在 macOS 上通过 Input Method Kit 与系统集成
3. WHEN 用户在任意应用程序中激活输入法 THEN Platform_Adapter SHALL 正确接收键盘事件
4. WHEN 用户提交文字 THEN Platform_Adapter SHALL 正确将文字传递给当前焦点应用
5. THE IME_Core SHALL 在 Windows 和 macOS 上共享相同的核心逻辑代码
6. WHEN 系统语言或区域设置变化 THEN Platform_Adapter SHALL 正确响应并调整行为

---

### Requirement 8: 输入法状态管理

**User Story:** 作为用户，我希望能够方便地切换输入法状态和查看当前状态。

#### Acceptance Criteria

1. THE Platform_Adapter SHALL 在系统托盘/菜单栏显示输入法状态图标
2. WHEN 用户点击状态图标 THEN Platform_Adapter SHALL 显示输入法菜单（切换中英文、设置等）
3. WHEN 输入模式切换 THEN 状态图标 SHALL 实时更新以反映当前模式
4. THE Platform_Adapter SHALL 支持通过系统快捷键切换输入法（Windows: Ctrl+Space, macOS: Cmd+Space 等系统默认快捷键）

---

### Requirement 9: 安装与部署

**User Story:** 作为用户，我希望能够简单地安装和卸载输入法。

#### Acceptance Criteria

1. THE 安装程序 SHALL 在 Windows 上提供标准 MSI/EXE 安装包
2. THE 安装程序 SHALL 在 macOS 上提供标准 PKG/DMG 安装包
3. WHEN 安装完成 THEN 输入法 SHALL 自动注册到系统输入法列表
4. WHEN 用户卸载输入法 THEN 安装程序 SHALL 清理所有相关文件和注册表/系统配置
5. THE 安装程序 SHALL 支持静默安装模式（用于企业部署）
