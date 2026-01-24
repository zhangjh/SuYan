#!/bin/bash
# 生成状态栏图标
# 中文模式：显示 "中"
# 英文模式：显示 "A"

OUTPUT_DIR="resources/icons/generated/status"
mkdir -p "$OUTPUT_DIR"

# 检查是否有 ImageMagick
if ! command -v convert &> /dev/null; then
    echo "ImageMagick not found, creating placeholder icons..."
    
    # 创建简单的 SVG 图标作为占位符
    cat > "$OUTPUT_DIR/chinese.svg" << 'EOF'
<svg xmlns="http://www.w3.org/2000/svg" width="22" height="22" viewBox="0 0 22 22">
  <text x="11" y="16" font-family="PingFang SC" font-size="14" font-weight="500" text-anchor="middle" fill="#000000">中</text>
</svg>
EOF

    cat > "$OUTPUT_DIR/english.svg" << 'EOF'
<svg xmlns="http://www.w3.org/2000/svg" width="22" height="22" viewBox="0 0 22 22">
  <text x="11" y="16" font-family="SF Pro" font-size="14" font-weight="500" text-anchor="middle" fill="#000000">A</text>
</svg>
EOF

    echo "Created SVG placeholder icons in $OUTPUT_DIR"
    exit 0
fi

# 使用 ImageMagick 生成 PNG 图标
# 中文模式图标
convert -size 22x22 xc:transparent \
    -font "PingFang-SC-Regular" -pointsize 14 \
    -fill black -gravity center -annotate 0 "中" \
    "$OUTPUT_DIR/chinese.png"

convert -size 44x44 xc:transparent \
    -font "PingFang-SC-Regular" -pointsize 28 \
    -fill black -gravity center -annotate 0 "中" \
    "$OUTPUT_DIR/chinese@2x.png"

# 英文模式图标
convert -size 22x22 xc:transparent \
    -font "SF-Pro-Text-Regular" -pointsize 14 \
    -fill black -gravity center -annotate 0 "A" \
    "$OUTPUT_DIR/english.png"

convert -size 44x44 xc:transparent \
    -font "SF-Pro-Text-Regular" -pointsize 28 \
    -fill black -gravity center -annotate 0 "A" \
    "$OUTPUT_DIR/english@2x.png"

echo "Generated status icons in $OUTPUT_DIR"
