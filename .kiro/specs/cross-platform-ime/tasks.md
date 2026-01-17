# Implementation Plan: Cross-Platform IME

## Overview

本实现计划基于 RIME 开源项目（librime + Weasel + Squirrel）进行二次开发，实现跨平台中英文输入法 MVP。

### 一期范围
- 基础拼音输入（简体中文）
- 英文直接输入
- 内置常用词库
- 本地用户词频学习
- Windows 和 macOS 双平台支持

### 二期范围（暂不实现）
- 云端词库下载与自动更新
- 用户自定义词包管理
- 自定义皮肤/主题

## 一期 Tasks

- [x] 1. 项目初始化与环境搭建
  - [x] 1.1 Fork RIME 相关仓库并建立项目结构
    - Fork librime、weasel、squirrel 仓库
    - 建立统一的项目目录结构
    - 配置 Git submodule 管理依赖
    - _Requirements: 7.5_

  - [x] 1.2 配置跨平台构建环境
    - 配置 Windows 构建环境（Visual Studio + CMake）
    - 配置 macOS 构建环境（Xcode + CMake）
    - 编写统一的构建脚本
    - 验证原版 RIME 可以成功编译
    - _Requirements: 7.1, 7.2_

  - [x] 1.3 配置测试框架
    - 集成 Google Test 用于单元测试
    - 集成 RapidCheck 用于属性测试
    - 创建测试词库（约 1000 词条）
    - _Requirements: Testing Strategy_

- [-] 2. 本地扩展存储模块
  - [x] 2.1 实现 SQLite 存储层
    - 创建 SQLite 数据库初始化逻辑
    - 实现 ILocalStorage 接口
    - 实现词库元数据 CRUD 操作
    - 实现应用配置读写操作
    - _Requirements: 4.3, 5.3_

  - [ ]* 2.2 编写存储层属性测试
    - **Property 15: 词频数据持久化往返**
    - **Validates: Requirements 5.3**

  - [x] 2.3 实现用户词频统计模块
    - 实现词频增量更新逻辑
    - 实现按拼音查询高频词
    - 实现词频数据与 RIME 排序的集成
    - _Requirements: 5.1, 5.2_

  - [ ]* 2.4 编写词频模块属性测试
    - **Property 13: 词频更新**
    - **Property 14: 用户词频影响排序**
    - **Validates: Requirements 5.1, 5.2**

- [x] 3. Checkpoint - 存储层验证
  - 确保所有存储层测试通过
  - 验证 SQLite 数据库正确创建和操作
  - 如有问题请向用户确认

- [x] 4. 拼音输入核心功能适配
  - [x] 4.1 配置简体拼音输入方案
    - 配置 RIME 使用雾凇拼音的简体拼音方案
    - 移除繁体中文相关配置
    - 配置默认词库加载顺序
    - _Requirements: 1.1, 4.1, 4.2_

  - [x] 4.2 集成用户词频到候选词排序
    - 实现 CandidateMerger 合并器（用户高频词优先注入策略）
    - 查询 SQLite 用户高频词（frequency >= 3，最多 5 个）
    - 与 librime 候选词合并去重
    - 用户高频词放在列表最前面
    - 实现词频加权算法用于综合排序
    - _Requirements: 1.2, 5.2_

  - [ ]* 4.3 编写拼音解析属性测试
    - **Property 1: 拼音解析正确性**
    - **Property 2: 候选词排序正确性**
    - **Validates: Requirements 1.1, 1.2, 1.8**

  - [x] 4.4 实现候选词选择与提交
    - 验证数字键选择功能
    - 验证空格键提交首选功能
    - 验证 Enter 键提交原文功能
    - 实现选择后词频更新
    - _Requirements: 1.3, 1.4, 1.5, 5.1_

  - [ ]* 4.5 编写候选词选择属性测试
    - **Property 3: 候选词选择正确性**
    - **Property 4: 拼音原文提交**
    - **Validates: Requirements 1.3, 1.4, 1.5**

  - [x] 4.6 实现输入状态管理
    - 验证 Escape 键清空状态
    - 验证 Backspace 键退格编辑
    - 实现输入状态重置逻辑
    - _Requirements: 1.6, 1.7_

  - [ ]* 4.7 编写输入状态属性测试
    - **Property 5: 输入状态重置**
    - **Property 6: 退格编辑正确性**
    - **Validates: Requirements 1.6, 1.7**

- [x] 5. Checkpoint - 拼音输入核心验证
  - 确保所有拼音输入测试通过
  - 手动验证基础拼音输入流程
  - 如有问题请向用户确认

- [x] 6. 中英文模式切换
  - [x] 6.1 实现模式切换逻辑
    - 实现 Shift 键切换中英文模式
    - 实现英文模式下的直接透传
    - 实现模式状态持久化
    - _Requirements: 2.1, 2.2_

  - [x] 6.2 实现临时英文模式
    - 检测大写字母开头输入
    - 实现临时英文模式切换
    - 实现提交后自动恢复中文模式
    - _Requirements: 2.3_

  - [ ]* 6.3 编写模式切换属性测试
    - **Property 7: 中英文模式切换**
    - **Property 8: 临时英文模式**
    - **Validates: Requirements 2.1, 2.2, 2.3, 2.4**

- [-] 7. 候选词窗口功能
  - [x] 7.1 实现候选词分页
    - 实现 Page Up/Page Down 翻页
    - 实现 +/- 键翻页
    - 验证分页边界处理
    - _Requirements: 3.2_

  - [ ]* 7.2 编写分页属性测试
    - **Property 9: 候选词分页正确性**
    - **Validates: Requirements 3.2**

  - [x] 7.3 实现空候选词处理
    - 无匹配时隐藏候选窗口
    - 验证无效拼音输入处理
    - _Requirements: 3.4_

  - [ ]* 7.4 编写空候选词属性测试
    - **Property 10: 空候选词处理**
    - **Validates: Requirements 3.4**

- [x] 8. 多词库管理
  - [x] 8.1 实现多词库加载
    - 实现词库动态加载/卸载
    - 实现多词库合并查询
    - 实现词库优先级管理
    - _Requirements: 4.4, 4.5_

  - [x] 8.2 编写多词库属性测试
    - **Property 11: 多词库合并查询**
    - **Property 12: 词库优先级**
    - **Validates: Requirements 4.4, 4.5**

  - [x] 8.3 实现自动学词功能
    - 检测连续单字输入形成词组
    - 自动添加新词组到用户词库
    - 实现学词频率控制（避免噪音）
    - _Requirements: 5.4_

  - [ ]* 8.4 编写自动学词属性测试
    - **Property 16: 自动学词**
    - **Validates: Requirements 5.4**

- [x] 9. Checkpoint - 本地功能完整验证
  - 确保所有本地功能测试通过
  - 手动验证完整输入流程
  - 如有问题请向用户确认

- [x] 10. Windows 平台适配（Weasel）
  - [x] 10.1 适配 Weasel 前端
    - 集成修改后的 librime
    - 配置简体拼音方案
    - 验证基础输入功能
    - _Requirements: 7.1, 7.3, 7.4_

  - [x] 10.2 实现状态图标和菜单
    - 实现系统托盘图标
    - 实现右键菜单（切换模式、设置）
    - 实现模式切换时图标更新
    - _Requirements: 8.1, 8.2, 8.3_

- [x] 11. macOS 平台适配（Squirrel）
  - [x] 11.1 适配 Squirrel 前端
    - 集成修改后的 librime
    - 配置简体拼音方案
    - 验证基础输入功能
    - _Requirements: 7.2, 7.3, 7.4_

  - [x] 11.2 实现状态图标和菜单
    - 实现菜单栏图标
    - 实现下拉菜单（切换模式、设置）
    - 实现模式切换时图标更新
    - _Requirements: 8.1, 8.2, 8.3_

- [x] 12. Checkpoint - 双平台功能验证
  - 在 Windows 上完整测试所有功能
  - 在 macOS 上完整测试所有功能
  - 验证跨平台行为一致性
  - 如有问题请向用户确认
  - _注：需要等待安装包制作完成后再进行验证_

- [x] 13. 安装包制作
  - [x] 13.1 制作 Windows 安装包
    - 配置 WiX 或 NSIS 打包脚本
    - 实现安装时输入法注册
    - 实现卸载时清理
    - 测试静默安装模式
    - _Requirements: 9.1, 9.3, 9.4, 9.5_

  - [x] 13.2 制作 macOS 安装包
    - 配置 pkgbuild 打包脚本
    - 实现安装时输入法注册
    - 实现卸载时清理
    - 创建 DMG 分发镜像
    - _Requirements: 9.2, 9.3, 9.4_

- [ ] 14. 最终验收
  - 执行完整的端到端测试
  - 验证所有一期需求已实现
  - 确保所有属性测试通过
  - 准备发布文档

---

## 二期 Tasks（暂不实现）

- [ ] 15. 云端词库服务（后端）
  - [ ] 15.1 搭建云端服务项目
    - 创建 Go 项目结构
    - 配置 PostgreSQL 数据库
    - 实现数据库迁移脚本
    - _Requirements: 6.1, 6.7_

  - [ ] 15.2 实现词库版本查询 API
    - 实现 GET /api/v1/dictionaries 接口
    - 实现词库分类查询
    - 编写 API 单元测试
    - _Requirements: 6.1, 6.7_

  - [ ] 15.3 实现词库更新检查 API
    - 实现 POST /api/v1/dictionaries/check-updates 接口
    - 实现版本比较逻辑
    - 支持增量更新判断
    - _Requirements: 6.2_

  - [ ] 15.4 实现词库下载 API
    - 实现 GET /api/v1/dictionaries/{id}/download 接口
    - 集成对象存储（本地文件或 S3）
    - 实现下载鉴权和限流
    - _Requirements: 6.2_

- [ ] 16. 云端同步客户端模块
  - [ ] 16.1 实现版本检查逻辑
    - 实现启动时版本查询
    - 实现版本号比较算法
    - 实现检查间隔控制
    - _Requirements: 6.3, 6.4_

  - [ ]* 16.2 编写版本比较属性测试
    - **Property 17: 版本比较触发更新**
    - **Validates: Requirements 6.4**

  - [ ] 16.3 实现词库下载功能
    - 实现后台静默下载
    - 实现断点续传
    - 实现下载进度跟踪
    - 实现校验和验证
    - _Requirements: 6.4_

  - [ ] 16.4 实现词库热更新
    - 下载完成后自动加载新词库
    - 无需重启输入法
    - 实现更新失败回滚
    - _Requirements: 6.5_

  - [ ] 16.5 实现离线降级处理
    - 网络不可用时静默失败
    - 确保本地功能正常
    - 实现重试策略（指数退避）
    - _Requirements: 6.6_

  - [ ]* 16.6 编写离线降级属性测试
    - **Property 18: 离线降级**
    - **Validates: Requirements 6.6**

- [ ] 17. 集成云端同步到平台
  - [ ] 17.1 集成到 Windows (Weasel)
    - 集成 Cloud Sync 模块到 Weasel
    - 配置启动时版本检查
    - 验证词库自动更新
    - _Requirements: 6.3, 6.4, 6.5_

  - [ ] 17.2 集成到 macOS (Squirrel)
    - 集成 Cloud Sync 模块到 Squirrel
    - 配置启动时版本检查
    - 验证词库自动更新
    - _Requirements: 6.3, 6.4, 6.5_

- [ ] 18. Checkpoint - 云端同步验证
  - 确保云端服务 API 测试通过
  - 确保客户端同步测试通过
  - 手动验证词库更新流程
  - 如有问题请向用户确认

## Notes

- 任务标记 `*` 的为可选测试任务，可根据时间优先级调整
- 每个 Checkpoint 是验证节点，确保阶段性成果稳定后再继续
- 属性测试使用 RapidCheck，每个测试运行 100+ 次迭代
- 一期共 14 个任务，聚焦本地功能和双平台支持
- 二期任务（15-18）为云端词库功能，待一期完成后再启动
