/**
 * LayoutManager - 布局管理器
 *
 * 负责管理候选词窗口的布局配置。
 * 支持横排和竖排两种布局模式，与 ConfigManager 集成实现配置持久化。
 */

#ifndef SUYAN_UI_LAYOUT_MANAGER_H
#define SUYAN_UI_LAYOUT_MANAGER_H

#include <QObject>
#include "../core/config_manager.h"

namespace suyan {

// 使用 config_manager.h 中定义的 LayoutType 枚举
// enum class LayoutType { Horizontal, Vertical };

/**
 * LayoutManager - 布局管理器类
 *
 * 职责：
 * 1. 管理当前布局类型
 * 2. 与 ConfigManager 集成实现配置读写
 * 3. 提供布局切换通知
 */
class LayoutManager : public QObject {
    Q_OBJECT

public:
    /**
     * 获取单例实例
     */
    static LayoutManager& instance();

    // 禁止拷贝和移动
    LayoutManager(const LayoutManager&) = delete;
    LayoutManager& operator=(const LayoutManager&) = delete;
    LayoutManager(LayoutManager&&) = delete;
    LayoutManager& operator=(LayoutManager&&) = delete;

    /**
     * 初始化布局管理器
     *
     * 从 ConfigManager 加载布局配置。
     * 需要在 ConfigManager 初始化之后调用。
     *
     * @return 是否成功
     */
    bool initialize();

    /**
     * 检查是否已初始化
     */
    bool isInitialized() const { return initialized_; }

    // ========== 布局管理 ==========

    /**
     * 获取当前布局类型
     */
    LayoutType getLayoutType() const { return layoutType_; }

    /**
     * 设置布局类型
     *
     * 会自动保存到 ConfigManager 并发出变更信号。
     *
     * @param type 布局类型
     */
    void setLayoutType(LayoutType type);

    /**
     * 切换布局类型
     *
     * 在横排和竖排之间切换。
     */
    void toggleLayout();

    /**
     * 检查是否为横排布局
     */
    bool isHorizontal() const { return layoutType_ == LayoutType::Horizontal; }

    /**
     * 检查是否为竖排布局
     */
    bool isVertical() const { return layoutType_ == LayoutType::Vertical; }

    // ========== 每页候选词数量 ==========

    /**
     * 获取每页候选词数量
     */
    int getPageSize() const { return pageSize_; }

    /**
     * 设置每页候选词数量
     *
     * @param size 数量（1-10）
     */
    void setPageSize(int size);

signals:
    /**
     * 布局类型变更信号
     *
     * @param type 新的布局类型
     */
    void layoutTypeChanged(LayoutType type);

    /**
     * 每页候选词数量变更信号
     *
     * @param size 新的数量
     */
    void pageSizeChanged(int size);

private:
    LayoutManager();
    ~LayoutManager();

    // 从 ConfigManager 加载配置
    void loadFromConfig();

    // 保存配置到 ConfigManager
    void saveToConfig();

    // 成员变量
    bool initialized_ = false;
    LayoutType layoutType_ = LayoutType::Horizontal;
    int pageSize_ = 9;
};

// ========== 辅助函数 ==========

/**
 * 布局类型转字符串
 */
QString layoutTypeToQString(LayoutType type);

/**
 * 字符串转布局类型
 */
LayoutType qstringToLayoutType(const QString& str);

} // namespace suyan

#endif // SUYAN_UI_LAYOUT_MANAGER_H
