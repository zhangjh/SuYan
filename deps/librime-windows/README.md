# Windows 版 librime 预编译库

本目录用于存放 Windows 平台的 librime 预编译库。

## 下载说明

从 [librime releases](https://github.com/rime/librime/releases) 下载最新的 Windows 预编译包。

### 下载步骤

1. 访问 https://github.com/rime/librime/releases
2. 找到最新版本（当前为 1.16.1）
3. 下载 `rime-*.Windows.zip` 或 `rime-*-msvc.zip` 文件
4. 解压到本目录

### 目录结构

解压后应包含以下结构：

```
deps/librime-windows/
├── include/
│   ├── rime_api.h
│   ├── rime_api_deprecated.h
│   ├── rime_api_stdbool.h
│   └── rime_levers_api.h
├── lib/
│   └── rime.lib          # 导入库
├── bin/
│   └── rime.dll          # 动态链接库
└── README.md             # 本文件
```

### 注意事项

- 确保下载的是 MSVC 编译版本（与项目编译器匹配）
- 如果使用 x64 架构，下载 x64 版本
- 如果使用 x86 架构，下载 x86 版本

## 版本信息

- 推荐版本: 1.16.1 或更新
- 编译器: MSVC 2019/2022
- 架构: x64 (推荐)

## 验证安装

安装完成后，运行以下命令验证：

```powershell
# 检查文件是否存在
Test-Path deps/librime-windows/include/rime_api.h
Test-Path deps/librime-windows/lib/rime.lib
Test-Path deps/librime-windows/bin/rime.dll
```
