# librime 预编译库

本目录存放 librime 预编译库文件，不包含源码。

## 目录结构

```
deps/librime/
├── include/          # 头文件
│   └── rime_api.h
├── lib/              # 库文件
│   └── librime.dylib (macOS)
│   └── rime.dll      (Windows, Phase 2)
└── README.md
```

## 获取方式

从 librime 官方 release 下载：
https://github.com/rime/librime/releases

macOS 选择 `rime-*-macOS.tar.bz2`，解压后将文件放到对应目录。

## 版本要求

- 最低版本：1.8.0
- 推荐版本：最新稳定版
