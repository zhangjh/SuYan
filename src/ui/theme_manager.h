/**
 * ThemeManager - 主题管理器
 *
 * 负责加载和管理候选词窗口的主题配置。
 * 支持内置主题、YAML 主题文件加载、系统深色模式监听。
 */

#ifndef SUYAN_UI_THEME_MANAGER_H
#define SUYAN_UI_THEME_MANAGER_H

#include <QObject>
#include <QString>
#include <QColor>
#include <QMap>
#include <QStringList>

namespace suyan {

/**
 * Theme - 主题配置结构
 *
 * 包含候选词窗口的所有视觉配置项。
 */
struct Theme {
    QString name;                   // 主题名称

    // 背景配置
    QColor backgroundColor;         // 背景颜色
    int backgroundOpacity = 95;     // 背景不透明度 (0-100)
    int borderRadius = 8;           // 圆角半径
    QColor borderColor;             // 边框颜色
    int borderWidth = 1;            // 边框宽度

    // 文字配置
    QString fontFamily;             // 字体族
    int fontSize = 16;              // 字体大小
    QColor textColor;               // 普通候选词颜色
    QColor highlightTextColor;      // 高亮候选词文字颜色
    QColor highlightBackColor;      // 高亮候选词背景颜色
    QColor preeditColor;            // 拼音颜色
    QColor labelColor;              // 序号颜色
    QColor commentColor;            // 注释颜色

    // 间距配置
    int candidateSpacing = 8;       // 候选词间距
    int padding = 10;               // 内边距

    /**
     * 检查主题是否有效
     */
    bool isValid() const {
        return !name.isEmpty() && backgroundColor.isValid() && textColor.isValid();
    }

    /**
     * 创建默认浅色主题
     */
    static Theme defaultLight();

    /**
     * 创建默认深色主题
     */
    static Theme defaultDark();

    // 内置主题名称常量
    static constexpr const char* NAME_LIGHT = "浅色";
    static constexpr const char* NAME_DARK = "深色";
};

/**
 * ThemeManager - 主题管理器类
 *
 * 职责：
 * 1. 加载内置主题和 YAML 主题文件
 * 2. 管理主题列表
 * 3. 监听系统深色模式变化
 * 4. 提供主题切换通知
 */
class ThemeManager : public QObject {
    Q_OBJECT

public:
    /**
     * 获取单例实例
     */
    static ThemeManager& instance();

    // 禁止拷贝和移动
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;
    ThemeManager(ThemeManager&&) = delete;
    ThemeManager& operator=(ThemeManager&&) = delete;

    /**
     * 初始化主题管理器
     *
     * @param themesDir 主题文件目录（可选，用于加载自定义主题）
     * @return 是否成功
     */
    bool initialize(const QString& themesDir = QString());

    /**
     * 从 ConfigManager 同步主题配置
     *
     * 读取 ConfigManager 中的主题设置并应用。
     * 需要在 ConfigManager 初始化之后调用。
     */
    void syncFromConfigManager();

    /**
     * 检查是否已初始化
     */
    bool isInitialized() const { return initialized_; }

    // ========== 主题管理 ==========

    /**
     * 加载内置主题
     */
    void loadBuiltinThemes();

    /**
     * 从 YAML 文件加载主题
     *
     * @param filePath YAML 文件路径
     * @return 是否成功
     */
    bool loadThemeFromFile(const QString& filePath);

    /**
     * 从目录加载所有主题文件
     *
     * @param dirPath 目录路径
     * @return 加载成功的主题数量
     */
    int loadThemesFromDirectory(const QString& dirPath);

    /**
     * 获取所有主题名称列表
     */
    QStringList getThemeNames() const;

    /**
     * 获取指定名称的主题
     *
     * @param name 主题名称
     * @return 主题配置，如果不存在返回默认主题
     */
    Theme getTheme(const QString& name) const;

    /**
     * 检查主题是否存在
     */
    bool hasTheme(const QString& name) const;

    // ========== 当前主题 ==========

    /**
     * 设置当前主题
     *
     * @param name 主题名称
     */
    void setCurrentTheme(const QString& name);

    /**
     * 获取当前主题
     */
    Theme getCurrentTheme() const;

    /**
     * 获取当前主题名称
     */
    QString getCurrentThemeName() const { return currentThemeName_; }

    // ========== 系统深色模式 ==========

    /**
     * 设置是否跟随系统深色模式
     *
     * @param follow 是否跟随
     */
    void setFollowSystemTheme(bool follow);

    /**
     * 获取是否跟随系统深色模式
     */
    bool isFollowSystemTheme() const { return followSystem_; }

    /**
     * 检查系统当前是否为深色模式
     */
    bool isSystemDarkMode() const;

    /**
     * 手动刷新系统深色模式状态
     * 通常由系统事件触发，也可手动调用
     */
    void refreshSystemTheme();

signals:
    /**
     * 主题变更信号
     *
     * @param theme 新的主题配置
     */
    void themeChanged(const Theme& theme);

    /**
     * 系统深色模式变更信号
     *
     * @param isDark 是否为深色模式
     */
    void systemDarkModeChanged(bool isDark);

private:
    ThemeManager();
    ~ThemeManager();

    // 内部方法
    Theme parseThemeYaml(const QString& filePath);
    void setupSystemThemeMonitor();
    void setupConfigManagerConnection();
    void applySystemTheme();

    // 成员变量
    bool initialized_ = false;
    QString themesDir_;
    QMap<QString, Theme> themes_;
    QString currentThemeName_;
    bool followSystem_ = true;
    bool cachedSystemDarkMode_ = false;

    // ConfigManager 连接
    QMetaObject::Connection configConnection_;

    // 内置主题名称常量（内部使用）
    static constexpr const char* THEME_LIGHT = "浅色";
    static constexpr const char* THEME_DARK = "深色";
    static constexpr const char* THEME_AUTO = "auto";
};

} // namespace suyan

#endif // SUYAN_UI_THEME_MANAGER_H
