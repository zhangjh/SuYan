# RIME 词库数据

本目录存放 RIME 输入法所需的词库和配置文件。

## 数据来源

词库数据来自 rime-ice（雾凇拼音）：
https://github.com/iDvel/rime-ice

## 目录结构

```
data/rime/
├── default.yaml              # 默认配置
├── rime_ice.schema.yaml      # 雾凇拼音输入方案
├── rime_ice.dict.yaml        # 词库索引
├── melt_eng.schema.yaml      # 英文混输方案
├── melt_eng.dict.yaml        # 英文混输词库索引
├── symbols_v.yaml            # 符号配置
├── symbols_caps_v.yaml       # 大写符号配置
├── custom_phrase.txt         # 自定义短语
├── cn_dicts/                 # 中文词库目录
│   ├── 8105.dict.yaml        # 通用规范汉字表
│   ├── 41448.dict.yaml       # 大字表
│   ├── base.dict.yaml        # 基础词库
│   ├── ext.dict.yaml         # 扩展词库
│   ├── tencent.dict.yaml     # 腾讯词向量
│   └── others.dict.yaml      # 杂项词库
├── en_dicts/                 # 英文词库目录
│   ├── en.dict.yaml          # 英文词库
│   ├── en_ext.dict.yaml      # 英文扩展词库
│   └── cn_en*.txt            # 中英混输词表
├── opencc/                   # OpenCC 配置
│   ├── emoji.json            # Emoji 配置
│   ├── emoji.txt             # Emoji 词表
│   └── others.txt            # 其他转换
└── lua/                      # Lua 脚本
    ├── autocap_filter.lua    # 自动大写
    ├── calc_translator.lua   # 计算器
    ├── date_translator.lua   # 日期时间
    ├── corrector.lua         # 拼写纠错
    └── ...                   # 其他脚本
```

## 更新词库

如需更新词库，从 rime-ice 仓库下载最新版本：

```bash
git clone --depth 1 https://github.com/iDvel/rime-ice.git /tmp/rime-ice
cp /tmp/rime-ice/default.yaml data/rime/
cp /tmp/rime-ice/rime_ice.schema.yaml data/rime/
cp /tmp/rime-ice/rime_ice.dict.yaml data/rime/
cp -r /tmp/rime-ice/cn_dicts data/rime/
cp -r /tmp/rime-ice/en_dicts data/rime/
cp -r /tmp/rime-ice/opencc data/rime/
cp -r /tmp/rime-ice/lua data/rime/
rm -rf /tmp/rime-ice
```
