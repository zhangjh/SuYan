# 素言（SuYan） (跨平台中英文输入法)

基于 RIME 开源引擎二次开发的跨平台中英文输入法。

## 项目结构

```
.
├── deps/                    # 第三方依赖 (Git submodules)
│   ├── librime/            # RIME 核心引擎
│   ├── weasel/             # Windows 前端 (小狼毫)
│   ├── squirrel/           # macOS 前端 (鼠须管)
│   └── rime-ice/           # 雾凇拼音词库 (基础词库)
├── src/                     # 项目源代码
│   ├── core/               # 核心扩展模块
│   │   ├── storage/        # 本地扩展存储 (SQLite)
│   │   ├── frequency/      # 用户词频管理
│   │   └── sync/           # 云端同步模块
│   ├── platform/           # 平台适配层
│   │   ├── windows/        # Windows 特定代码
│   │   └── macos/          # macOS 特定代码
│   └── common/             # 跨平台公共代码
├── tests/                   # 测试代码
│   ├── unit/               # 单元测试
│   ├── property/           # 属性测试
│   └── data/               # 测试数据 (测试词库)
├── scripts/                 # 构建和工具脚本
├── docs/                    # 文档
└── cloud/                   # 云端服务代码 (Go)
```

## 构建要求

### Windows
- Visual Studio 2019 或更高版本
- CMake 3.16+
- Git

### macOS
- Xcode 12 或更高版本
- CMake 3.16+
- Git

## 快速开始

```bash
# 克隆仓库并初始化子模块
git clone --recursive <repository-url>
cd cross-platform-ime

# 或者在已克隆的仓库中初始化子模块
git submodule update --init --recursive

# 构建 (参见 scripts/ 目录下的构建脚本)
```

## 许可证

本项目基于 RIME 开源项目二次开发，遵循相应的开源许可证。
