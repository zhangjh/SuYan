# IME Pinyin 安装包构建指南

本目录包含 Windows 和 macOS 平台的安装包构建脚本和配置文件。

## 目录结构

```
installer/
├── README.md                    # 本文档
├── windows/                     # Windows 安装包
│   ├── install.nsi             # NSIS 安装脚本
│   ├── build-installer.bat     # 构建脚本
│   ├── silent-install.bat      # 静默安装脚本
│   ├── silent-uninstall.bat    # 静默卸载脚本
│   ├── LICENSE.txt             # 许可证文件
│   ├── README.txt              # 说明文件
│   └── resources/              # 资源文件（图标等）
└── macos/                       # macOS 安装包
    ├── package/                 # 打包脚本
    │   ├── make_package.sh     # PKG 构建脚本
    │   ├── sign_and_notarize.sh # 签名和公证脚本
    │   ├── common.sh           # 公共函数
    │   ├── PackageInfo         # 包信息
    │   └── IMEPinyin-component.plist
    ├── scripts/                 # 安装脚本
    │   ├── preinstall          # 安装前脚本
    │   └── postinstall         # 安装后脚本
    └── resources/               # 资源文件
        └── IMEPinyin.entitlements
```

## 子模块说明

项目使用 Git 子模块管理依赖。**注意：子模块代码不提交到本仓库，构建时需要初始化。**

### 子模块列表

| 子模块 | 路径 | 说明 | 构建时是否需要 |
|--------|------|------|----------------|
| librime | deps/librime | RIME 核心引擎 | ✅ 需要源码头文件 |
| weasel | deps/weasel | Windows 前端 | ✅ 需要源码构建 |
| squirrel | deps/squirrel | macOS 前端 | ✅ 需要源码构建 |
| rime-ice | deps/rime-ice | 雾凇拼音词库 | ✅ 需要词库数据 |

### 初始化子模块

首次克隆项目后，**必须**初始化子模块：

```bash
# 初始化所有子模块
git submodule update --init --recursive

# 或使用项目脚本
./scripts/init-submodules.sh
```

### 关于 librime

本项目使用 RIME 官方预编译的 librime 库文件（包含 lua、octagram、predict 插件），但仍需要 librime 源码中的头文件用于编译 Squirrel。

**为什么需要 librime 子模块？**
- Squirrel 的 Swift 代码需要引用 `rime/key_table.h` 等内部头文件
- 这些头文件不包含在预编译包中，只存在于源码中
- 因此必须初始化 `deps/librime` 子模块

预编译包版本：
- librime: 1.16.0
- 包含插件: librime-lua, librime-octagram, librime-predict

如需自定义 librime，请参考 [librime 构建指南](https://github.com/rime/librime/blob/master/README.md)。

---

## macOS 安装包

### 系统要求

- macOS 13.0+ (Ventura)
- Xcode 15.0+ (含 Command Line Tools)

### 前置依赖

构建前需要安装以下依赖：

```bash
# 1. 安装 Xcode Command Line Tools
xcode-select --install

# 2. 安装 curl（通常已预装）
# 用于下载预编译的 librime 包
```

### 构建步骤

#### 完整构建（推荐）

```bash
# 从项目根目录执行
./scripts/build-macos-installer.sh all
```

这将自动完成：
1. 下载预编译的 librime（含插件）
2. 下载 Sparkle 框架
3. 初始化 plum 配置管理器
4. 构建 Squirrel.app
5. 创建 PKG 安装包

#### 分步构建

```bash
# 1. 下载依赖（librime、Sparkle、plum）
./scripts/build-macos-installer.sh deps

# 2. 构建应用
./scripts/build-macos-installer.sh app

# 3. 创建安装包
./scripts/build-macos-installer.sh package

# 清理构建产物
./scripts/build-macos-installer.sh clean
```

### 签名和公证

发布前需要签名和公证：

```bash
# 设置 Developer ID
export DEV_ID="Your Name (TEAMID)"

# 构建并签名
./scripts/build-macos-installer.sh all sign
```

### 输出文件

- `output/macos/IMEPinyin-{version}.pkg`

### 下载的依赖

构建脚本会自动下载以下文件到 `deps/squirrel/download/`：

| 文件 | 大小 | 说明 |
|------|------|------|
| rime-{hash}-macOS-universal.tar.bz2 | ~4MB | librime 预编译包 |
| rime-deps-{hash}-macOS-universal.tar.bz2 | ~3MB | OpenCC 数据 |
| Sparkle-{version}.tar.xz | ~13MB | 自动更新框架 |

这些文件会被缓存，重复构建时不会重新下载。

---

## Windows 安装包

### 系统要求

- Windows 10/11
- Visual Studio 2019/2022（含 C++ 工作负载）

### 前置依赖

```batch
:: 1. 安装 NSIS 3.08+
:: 下载地址: https://nsis.sourceforge.io/

:: 2. 安装 Boost 库
:: 设置环境变量 BOOST_ROOT

:: 3. 安装 CMake 3.20+
```

### 构建步骤

#### 完整构建

```batch
cd scripts
build-windows-installer.bat all
```

#### 仅构建安装包

```batch
cd installer\windows
build-installer.bat 1.0.0 0
```

### 静默安装

```batch
# 静默安装
silent-install.bat ime-pinyin-1.0.0-installer.exe

# 指定安装目录
silent-install.bat ime-pinyin-1.0.0-installer.exe /D=C:\IME

# 静默卸载
silent-uninstall.bat
```

### 输出文件

- `output/windows/ime-pinyin-{version}-installer.exe`

---

## 版本号管理

### macOS

```bash
export IME_VERSION=1.0.0
./scripts/build-macos-installer.sh
```

### Windows

```batch
set IME_VERSION=1.0.0
set IME_BUILD=1
scripts\build-windows-installer.bat
```

---

## 安装包功能

### Windows

- ✅ 自动检测系统架构 (x86/x64/ARM64)
- ✅ 支持升级安装（自动备份用户数据）
- ✅ 自动注册输入法到系统
- ✅ 创建开始菜单快捷方式
- ✅ 支持静默安装模式
- ✅ 完整卸载（清理注册表和文件）
- ✅ 多语言支持（简体中文、繁体中文、英文）

### macOS

- ✅ 支持 macOS 13.0+ (Ventura)
- ✅ 支持 Intel 和 Apple Silicon (Universal Binary)
- ✅ 自动注册输入法到系统
- ✅ 包含 RIME 插件（lua、octagram、predict）
- ✅ 预构建 RIME 数据和 OpenCC
- ✅ 支持代码签名和公证
- ✅ 包含卸载脚本

---

## 故障排除

### macOS

**问题**: 下载依赖失败
**解决**: 检查网络连接，或手动下载文件到 `deps/squirrel/download/`

**问题**: Xcode 构建失败
**解决**: 确保 Xcode 版本 >= 15.0，运行 `xcode-select --install`

**问题**: 签名失败
**解决**: 确保 Developer ID 证书已安装到钥匙串

**问题**: 公证失败
**解决**: 配置 notarytool：
```bash
xcrun notarytool store-credentials "Your Name (TEAMID)" \
    --apple-id "your@email.com" \
    --team-id "TEAMID" \
    --password "app-specific-password"
```

### Windows

**问题**: NSIS 找不到
**解决**: 确保 NSIS 安装在默认路径，或设置 `NSIS_PATH` 环境变量

**问题**: 构建失败
**解决**: 确保从 Developer Command Prompt 运行脚本

---

## 相关文档

- [RIME 官方文档](https://rime.im/docs/)
- [librime Releases](https://github.com/rime/librime/releases)
- [Squirrel 项目](https://github.com/rime/squirrel)
- [Weasel 项目](https://github.com/rime/weasel)
- [NSIS 文档](https://nsis.sourceforge.io/Docs/)
