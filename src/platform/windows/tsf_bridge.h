/**
 * TSFBridge - TSF 文本服务实现
 * Task 1.1: Windows 平台目录结构
 * 
 * 实现 TSF 所需的 COM 接口
 */

#ifndef SUYAN_WINDOWS_TSF_BRIDGE_H
#define SUYAN_WINDOWS_TSF_BRIDGE_H

#ifdef _WIN32

#include <windows.h>
#include <msctf.h>
#include <atomic>
#include <string>

namespace suyan {

// 前向声明
class InputEngine;
class CandidateWindow;
class WindowsBridge;

// ============================================
// CLSID 和 GUID 声明
// ============================================

// {A1B2C3D4-E5F6-7890-ABCD-EF1234567890}
// 素言输入法 CLSID
extern const CLSID CLSID_SuYanTextService;

// {B2C3D4E5-F6A7-8901-BCDE-F12345678901}
// 素言输入法语言配置 GUID
extern const GUID GUID_SuYanProfile;

// 语言 ID: 简体中文
constexpr LANGID SUYAN_LANGID = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);

// ============================================
// TSFBridge 类
// ============================================

/**
 * TSFBridge - TSF 文本服务实现
 * 
 * 实现 TSF 所需的 COM 接口：
 * - IUnknown: COM 基础接口
 * - ITfTextInputProcessor: 文本输入处理器
 * - ITfKeyEventSink: 键盘事件接收器
 * - ITfCompositionSink: 输入组合接收器
 * - ITfDisplayAttributeProvider: 显示属性提供器
 */
class TSFBridge : public ITfTextInputProcessor,
                  public ITfKeyEventSink,
                  public ITfCompositionSink,
                  public ITfDisplayAttributeProvider {
public:
    TSFBridge();
    virtual ~TSFBridge();

    // ========== IUnknown ==========
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ========== ITfTextInputProcessor ==========
    STDMETHODIMP Activate(ITfThreadMgr* pThreadMgr, TfClientId tfClientId) override;
    STDMETHODIMP Deactivate() override;

    // ========== ITfKeyEventSink ==========
    STDMETHODIMP OnSetFocus(BOOL fForeground) override;
    STDMETHODIMP OnTestKeyDown(ITfContext* pContext, WPARAM wParam, 
                               LPARAM lParam, BOOL* pfEaten) override;
    STDMETHODIMP OnTestKeyUp(ITfContext* pContext, WPARAM wParam, 
                             LPARAM lParam, BOOL* pfEaten) override;
    STDMETHODIMP OnKeyDown(ITfContext* pContext, WPARAM wParam, 
                           LPARAM lParam, BOOL* pfEaten) override;
    STDMETHODIMP OnKeyUp(ITfContext* pContext, WPARAM wParam, 
                         LPARAM lParam, BOOL* pfEaten) override;
    STDMETHODIMP OnPreservedKey(ITfContext* pContext, REFGUID rguid, 
                                BOOL* pfEaten) override;

    // ========== ITfCompositionSink ==========
    STDMETHODIMP OnCompositionTerminated(TfEditCookie ecWrite, 
                                         ITfComposition* pComposition) override;

    // ========== ITfDisplayAttributeProvider ==========
    STDMETHODIMP EnumDisplayAttributeInfo(IEnumTfDisplayAttributeInfo** ppEnum) override;
    STDMETHODIMP GetDisplayAttributeInfo(REFGUID guid, 
                                         ITfDisplayAttributeInfo** ppInfo) override;

    // ========== 组件访问 ==========
    void setInputEngine(InputEngine* engine) { inputEngine_ = engine; }
    void setCandidateWindow(CandidateWindow* window) { candidateWindow_ = window; }
    void setWindowsBridge(WindowsBridge* bridge) { windowsBridge_ = bridge; }

    InputEngine* getInputEngine() const { return inputEngine_; }
    CandidateWindow* getCandidateWindow() const { return candidateWindow_; }
    WindowsBridge* getWindowsBridge() const { return windowsBridge_; }

    // ========== 状态访问 ==========
    ITfThreadMgr* getThreadMgr() const { return threadMgr_; }
    TfClientId getClientId() const { return clientId_; }
    ITfContext* getCurrentContext() const { return currentContext_; }

    // ========== 输入组合管理 ==========
    HRESULT startComposition(ITfContext* pContext);
    HRESULT endComposition();
    bool isComposing() const { return composition_ != nullptr; }

    // ========== 文本操作 ==========
    HRESULT commitText(const std::wstring& text);
    HRESULT updatePreedit(const std::wstring& preedit, int caretPos);
    HRESULT clearPreedit();

    // ========== 激活状态 ==========
    bool isActivated() const { return activated_; }

private:
    // 键码转换
    int convertVirtualKeyToRime(WPARAM vk, LPARAM lParam);
    int convertModifiers();

    // 初始化和清理
    HRESULT initKeySink();
    HRESULT uninitKeySink();
    HRESULT initThreadMgrSink();
    HRESULT uninitThreadMgrSink();

    // 更新候选词窗口位置
    void updateCandidateWindowPosition();

    // 处理 Shift 键切换模式
    void handleShiftKeyRelease();

    // COM 引用计数
    std::atomic<ULONG> refCount_{1};

    // TSF 对象
    ITfThreadMgr* threadMgr_ = nullptr;
    TfClientId clientId_ = TF_CLIENTID_NULL;
    ITfContext* currentContext_ = nullptr;
    ITfComposition* composition_ = nullptr;
    ITfCompositionSink* compositionSink_ = nullptr;
    DWORD threadMgrSinkCookie_ = TF_INVALID_COOKIE;
    DWORD textEditSinkCookie_ = TF_INVALID_COOKIE;
    DWORD keySinkCookie_ = TF_INVALID_COOKIE;

    // 激活状态
    bool activated_ = false;

    // Shift 键状态跟踪（用于中英文切换）
    bool shiftKeyPressed_ = false;
    bool otherKeyPressedWithShift_ = false;
    DWORD shiftPressTime_ = 0;

    // 组件引用
    InputEngine* inputEngine_ = nullptr;
    CandidateWindow* candidateWindow_ = nullptr;
    WindowsBridge* windowsBridge_ = nullptr;
};

// ============================================
// COM 类工厂
// ============================================

class TSFBridgeFactory : public IClassFactory {
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // IClassFactory
    STDMETHODIMP CreateInstance(IUnknown* pUnkOuter, REFIID riid, 
                                void** ppvObj) override;
    STDMETHODIMP LockServer(BOOL fLock) override;
};

// ============================================
// DLL 导出函数
// ============================================

extern "C" {
    STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv);
    STDAPI DllCanUnloadNow();
    STDAPI DllRegisterServer();
    STDAPI DllUnregisterServer();
}

// ============================================
// DLL 模块管理
// ============================================

/**
 * 获取 DLL 模块句柄
 */
HMODULE GetModuleHandle();

/**
 * 设置 DLL 模块句柄（在 DllMain 中调用）
 */
void SetModuleHandle(HMODULE hModule);

/**
 * 增加 DLL 引用计数
 */
void DllAddRef();

/**
 * 减少 DLL 引用计数
 */
void DllRelease();

} // namespace suyan

#endif // _WIN32

#endif // SUYAN_WINDOWS_TSF_BRIDGE_H
