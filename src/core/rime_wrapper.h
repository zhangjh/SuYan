/**
 * RimeWrapper - librime API 封装
 *
 * 封装 librime C API，提供 C++ 友好的接口。
 * 使用单例模式管理 RIME 引擎生命周期。
 */

#ifndef SUYAN_CORE_RIME_WRAPPER_H
#define SUYAN_CORE_RIME_WRAPPER_H

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include "rime_api.h"

namespace suyan {

/**
 * 候选词结构
 */
struct Candidate {
    std::string text;       // 候选词文本
    std::string comment;    // 注释（如拼音）
    int index;              // 序号 (0-based)
};

/**
 * 候选词菜单信息
 */
struct CandidateMenu {
    std::vector<Candidate> candidates;  // 当前页候选词
    int pageSize;                       // 每页大小
    int pageIndex;                      // 当前页码 (0-based)
    bool isLastPage;                    // 是否最后一页
    int highlightedIndex;               // 高亮候选词索引
    std::string selectKeys;             // 选择键（如 "1234567890"）
};

/**
 * 输入组合信息
 */
struct Composition {
    std::string preedit;    // 预编辑文本（带分隔符的拼音）
    int cursorPos;          // 光标位置
    int selStart;           // 选中起始
    int selEnd;             // 选中结束
};

/**
 * RIME 状态信息
 */
struct RimeState {
    std::string schemaId;       // 当前方案 ID
    std::string schemaName;     // 当前方案名称
    bool isComposing;           // 是否正在输入
    bool isAsciiMode;           // 是否 ASCII 模式
    bool isDisabled;            // 是否禁用
};

/**
 * 通知回调类型
 */
using NotificationCallback = std::function<void(RimeSessionId sessionId,
                                                 const std::string& messageType,
                                                 const std::string& messageValue)>;

/**
 * RimeWrapper - librime 封装类
 *
 * 单例模式，管理 RIME 引擎的初始化、会话和输入处理。
 */
class RimeWrapper {
public:
    /**
     * 获取单例实例
     */
    static RimeWrapper& instance();

    // 禁止拷贝和移动
    RimeWrapper(const RimeWrapper&) = delete;
    RimeWrapper& operator=(const RimeWrapper&) = delete;
    RimeWrapper(RimeWrapper&&) = delete;
    RimeWrapper& operator=(RimeWrapper&&) = delete;

    /**
     * 初始化 RIME 引擎
     *
     * @param userDataDir 用户数据目录（存放用户配置和词库）
     * @param sharedDataDir 共享数据目录（存放预置词库和方案）
     * @param appName 应用名称（用于日志）
     * @return 初始化是否成功
     */
    bool initialize(const std::string& userDataDir,
                    const std::string& sharedDataDir,
                    const std::string& appName = "SuYan");

    /**
     * 关闭 RIME 引擎
     */
    void finalize();

    /**
     * 检查是否已初始化
     */
    bool isInitialized() const { return initialized_; }

    /**
     * 启动维护任务（部署）
     *
     * @param fullCheck 是否完整检查
     * @return 是否成功启动
     */
    bool startMaintenance(bool fullCheck = false);

    /**
     * 等待维护任务完成
     */
    void joinMaintenanceThread();

    /**
     * 检查是否处于维护模式
     */
    bool isMaintenanceMode() const;

    // ========== 会话管理 ==========

    /**
     * 创建新会话
     *
     * @return 会话 ID，0 表示失败
     */
    RimeSessionId createSession();

    /**
     * 销毁会话
     *
     * @param sessionId 会话 ID
     */
    void destroySession(RimeSessionId sessionId);

    /**
     * 检查会话是否存在
     *
     * @param sessionId 会话 ID
     * @return 会话是否存在
     */
    bool findSession(RimeSessionId sessionId) const;

    // ========== 输入处理 ==========

    /**
     * 处理按键事件
     *
     * @param sessionId 会话 ID
     * @param keyCode 键码
     * @param modifiers 修饰键掩码
     * @return 按键是否被处理
     */
    bool processKey(RimeSessionId sessionId, int keyCode, int modifiers);

    /**
     * 模拟按键序列
     *
     * @param sessionId 会话 ID
     * @param keySequence 按键序列（如 "nihao"）
     * @return 是否成功
     */
    bool simulateKeySequence(RimeSessionId sessionId, const std::string& keySequence);

    /**
     * 清除当前输入
     *
     * @param sessionId 会话 ID
     */
    void clearComposition(RimeSessionId sessionId);

    /**
     * 提交当前输入
     *
     * @param sessionId 会话 ID
     * @return 是否有内容提交
     */
    bool commitComposition(RimeSessionId sessionId);

    // ========== 候选词操作 ==========

    /**
     * 获取候选词菜单
     *
     * @param sessionId 会话 ID
     * @return 候选词菜单信息
     */
    CandidateMenu getCandidateMenu(RimeSessionId sessionId);

    /**
     * 选择候选词（当前页）
     *
     * @param sessionId 会话 ID
     * @param index 候选词索引 (0-based，当前页内)
     * @return 是否成功
     */
    bool selectCandidateOnCurrentPage(RimeSessionId sessionId, size_t index);

    /**
     * 选择候选词（全局索引）
     *
     * @param sessionId 会话 ID
     * @param index 候选词全局索引 (0-based)
     * @return 是否成功
     */
    bool selectCandidate(RimeSessionId sessionId, size_t index);

    /**
     * 高亮候选词（不提交）
     *
     * @param sessionId 会话 ID
     * @param index 候选词索引
     * @return 是否成功
     */
    bool highlightCandidate(RimeSessionId sessionId, size_t index);

    /**
     * 翻页
     *
     * @param sessionId 会话 ID
     * @param backward true 向前翻页，false 向后翻页
     * @return 是否成功
     */
    bool changePage(RimeSessionId sessionId, bool backward);

    /**
     * 删除候选词
     *
     * @param sessionId 会话 ID
     * @param index 候选词索引
     * @return 是否成功
     */
    bool deleteCandidate(RimeSessionId sessionId, size_t index);

    // ========== 输出获取 ==========

    /**
     * 获取提交的文本
     *
     * @param sessionId 会话 ID
     * @return 提交的文本，空字符串表示无提交
     */
    std::string getCommitText(RimeSessionId sessionId);

    /**
     * 获取预编辑文本（preedit）
     *
     * @param sessionId 会话 ID
     * @return 组合信息
     */
    Composition getComposition(RimeSessionId sessionId);

    /**
     * 获取原始输入
     *
     * @param sessionId 会话 ID
     * @return 原始输入字符串
     */
    std::string getRawInput(RimeSessionId sessionId);

    /**
     * 获取光标位置
     *
     * @param sessionId 会话 ID
     * @return 光标位置
     */
    size_t getCaretPos(RimeSessionId sessionId);

    /**
     * 设置光标位置
     *
     * @param sessionId 会话 ID
     * @param pos 光标位置
     */
    void setCaretPos(RimeSessionId sessionId, size_t pos);

    // ========== 状态查询 ==========

    /**
     * 获取 RIME 状态
     *
     * @param sessionId 会话 ID
     * @return 状态信息
     */
    RimeState getState(RimeSessionId sessionId);

    /**
     * 获取选项值
     *
     * @param sessionId 会话 ID
     * @param option 选项名
     * @return 选项值
     */
    bool getOption(RimeSessionId sessionId, const std::string& option);

    /**
     * 设置选项值
     *
     * @param sessionId 会话 ID
     * @param option 选项名
     * @param value 选项值
     */
    void setOption(RimeSessionId sessionId, const std::string& option, bool value);

    // ========== 方案管理 ==========

    /**
     * 获取当前方案 ID
     *
     * @param sessionId 会话 ID
     * @return 方案 ID
     */
    std::string getCurrentSchemaId(RimeSessionId sessionId);

    /**
     * 选择方案
     *
     * @param sessionId 会话 ID
     * @param schemaId 方案 ID
     * @return 是否成功
     */
    bool selectSchema(RimeSessionId sessionId, const std::string& schemaId);

    /**
     * 获取可用方案列表
     *
     * @return 方案 ID 和名称的列表
     */
    std::vector<std::pair<std::string, std::string>> getSchemaList();

    // ========== 通知回调 ==========

    /**
     * 设置通知回调
     *
     * @param callback 回调函数
     */
    void setNotificationCallback(NotificationCallback callback);

    // ========== 版本信息 ==========

    /**
     * 获取 librime 版本
     */
    std::string getVersion() const;

private:
    RimeWrapper();
    ~RimeWrapper();

    // 静态通知处理函数
    static void notificationHandler(void* contextObject,
                                    RimeSessionId sessionId,
                                    const char* messageType,
                                    const char* messageValue);

    RimeApi* api_ = nullptr;
    bool initialized_ = false;
    NotificationCallback notificationCallback_;
};

} // namespace suyan

#endif // SUYAN_CORE_RIME_WRAPPER_H
