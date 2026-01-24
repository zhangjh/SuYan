# librime 预编译库

## 版本信息

- 版本: 1.16.1
- 下载日期: 2026-01-22
- 来源: https://github.com/rime/librime/releases/tag/1.16.1
- 文件: rime-de4700e-macOS-universal.tar.bz2

## 目录结构

```
deps/librime/
├── include/          # 头文件
│   ├── rime_api.h
│   ├── rime_api_deprecated.h
│   ├── rime_api_stdbool.h
│   └── rime_levers_api.h
├── lib/              # 库文件
│   ├── librime.dylib
│   ├── librime.1.dylib
│   └── librime.1.16.1.dylib
└── README.md
```

## 使用方式

CMake 中已配置 `librime` imported target，直接链接即可：

```cmake
target_link_libraries(your_target PRIVATE librime)
```

## 更新方法

1. 从 https://github.com/rime/librime/releases 下载最新 macOS 预编译包
2. 解压头文件到 `include/`
3. 解压库文件到 `lib/`
4. 更新本文档的版本信息
