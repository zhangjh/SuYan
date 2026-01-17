# RIME 配置文件说明

本目录包含跨平台输入法的 RIME 配置文件。

## 文件结构

```
data/rime/
├── default.custom.yaml     # 默认配置定制
├── ime_pinyin.schema.yaml  # 简体拼音输入方案
├── ime_pinyin.dict.yaml    # 简体拼音词库配置
├── melt_eng.schema.yaml    # 英文输入方案
├── melt_eng.dict.yaml      # 英文词库配置
└── README.md               # 本文件
```

## 配置说明

### 输入方案 (ime_pinyin)

- 基于 rime-ice (雾凇拼音) 定制
- 仅支持简体中文，移除繁体相关功能
- 支持超级简拼和常见拼写纠错
- 支持中英混输

### 词库加载顺序

1. `cn_dicts/8105` - 通用规范汉字表 8105 字
2. `cn_dicts/base` - 基础词库
3. `cn_dicts/ext` - 扩展词库
4. `cn_dicts/tencent` - 腾讯词向量词库
5. `cn_dicts/others` - 其他词汇

### 按键绑定

| 按键 | 功能 |
|------|------|
| 空格 | 上屏首选候选词 |
| 回车 | 上屏原始拼音 |
| Esc | 取消输入 |
| 退格 | 删除最后一个字符 |
| 1-9 | 选择对应候选词 |
| -/= | 翻页 |
| Page Up/Down | 翻页 |
| Shift | 切换中英文模式 |

## 部署说明

这些配置文件需要与 rime-ice 的词库文件配合使用：

1. 将 `deps/rime-ice/cn_dicts/` 复制到 RIME 用户目录
2. 将 `deps/rime-ice/en_dicts/` 复制到 RIME 用户目录
3. 将本目录的配置文件复制到 RIME 用户目录
4. 执行 RIME 部署
