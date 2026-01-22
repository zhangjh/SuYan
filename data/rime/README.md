# RIME 词库数据

本目录存放 RIME 输入法所需的词库和配置文件。

## 获取方式

从 rime-ice（雾凇拼音）下载：
https://github.com/iDvel/rime-ice

需要的文件：
- `default.yaml` - 默认配置
- `rime_ice.schema.yaml` - 输入方案
- `rime_ice.dict.yaml` - 词库索引
- `cn_dicts/` - 中文词库目录
- `en_dicts/` - 英文词库目录
- `opencc/` - OpenCC 配置（简繁转换）

## 目录结构

```
data/rime/
├── default.yaml
├── rime_ice.schema.yaml
├── rime_ice.dict.yaml
├── cn_dicts/
│   ├── base.dict.yaml
│   ├── ext.dict.yaml
│   └── ...
├── en_dicts/
│   └── en.dict.yaml
└── opencc/
    └── ...
```
