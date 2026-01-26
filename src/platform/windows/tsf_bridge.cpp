/**
 * TSFBridge 实现
 * Task 4: TSFBridge COM 基础实现
 * Task 5: TSFBridge 核心接口实现
 * 
 * 实现 TSF COM 组件的核心功能
 */

#ifdef _WIN32

#include "tsf_bridge.h"
#include "windows_bridge.h"
#include "key_converter.h"
#include "input_engine.h"
#include "candidate_window.h"
#include <ctffunc.h>
#include <strsafe.h>
#include <QPoint>

namespace suyan {

// ============================================
// 全局变量
// ============================================

// DLL 模块句柄
static HMODULE g_hModule = nullptr;

// DLL 引用计数（用于 DllCanUnloadNow）
static LONG g_dllRefCount = 0;

// 服务器锁定计数
static LONG g_serverLockCount = 0;

// 获取 DLL 模块句柄
HMODULE GetModuleHandle() {
    return g_hModule;
}

// 设置 DLL 模块句柄（在 DllMain 中调用）
void SetModuleHandle(HMODULE hModule) {
    g_hModule = hModule;
}

// 增加 DLL 引用计数
void DllAddRef() {
    InterlockedIncrement(&g_dllRefCount);
}

// 减少 DLL 引用计数
void DllRelease() {
    InterlockedDecrement(&g_dllRefCount);
}

// ============================================
// CLSID 和 GUID 定义
// ============================================

// {A1B2C3D4-E5F6-7890-ABCD-EF1234567890}
const CLSID CLSID_SuYanTextService = {
    0xA1B2C3D4, 0xE5F6, 0x7890,
    { 0xAB, 0xCD, 0xEF, 0x12, 0x34, 0x56, 0x78, 0x90 }
};

// {B2C3D4E5-F6A7-8901-BCDE-F12345678901}
const GUID GUID_SuYanProfile = {
    0xB2C3D4E5, 0xF6A7, 0x8901,
    { 0xBC, 0xDE, 0xF1, 0x23, 0x45, 0x67, 0x89, 0x01 }
};

// ============================================
// GUID 字符串转换辅助函数
// ============================================

// 将 GUID 转换为注册表格式字符串 {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
static bool GuidToString(const GUID& guid, wchar_t* buffer, size_t bufferSize) {
    return SUCCEEDED(StringCchPrintfW(buffer, bufferSize,
        L"{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
        guid.Data1, guid.Data2, guid.Data3,
        guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
        guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]));
}

// ============================================
// 注册表操作辅助函数
// ============================================

// 创建注册表键并设置默认值
static HRESULT CreateRegKeyAndSetValue(HKEY hKeyRoot, const wchar_t* subKey, 
                                        const wchar_t* valueName, const wchar_t* value) {
    HKEY hKey = nullptr;
    LONG result = RegCreateKeyExW(hKeyRoot, subKey, 0, nullptr, 
                                   REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    if (result != ERROR_SUCCESS) {
        return HRESULT_FROM_WIN32(result);
    }

    if (value) {
        result = RegSetValueExW(hKey, valueName, 0, REG_SZ, 
                                 reinterpret_cast<const BYTE*>(value),
                                 static_cast<DWORD>((wcslen(value) + 1) * sizeof(wchar_t)));
    }

    RegCloseKey(hKey);
    return HRESULT_FROM_WIN32(result);
}

// 递归删除注册表键
static HRESULT DeleteRegKey(HKEY hKeyRoot, const wchar_t* subKey) {
    // 使用 RegDeleteTreeW 递归删除（Windows Vista+）
    LONG result = RegDeleteTreeW(hKeyRoot, subKey);
    if (result == ERROR_FILE_NOT_FOUND) {
        return S_OK; // 键不存在，视为成功
    }
    return HRESULT_FROM_WIN32(result);
}

// ============================================
// TSFBridge 实现
// ============================================

TSFBridge::TSFBridge() = default;

TSFBridge::~TSFBridge() {
    Deactivate();
}

// IUnknown
STDMETHODIMP TSFBridge::QueryInterface(REFIID riid, void** ppvObj) {
    if (!ppvObj) {
        return E_INVALIDARG;
    }

    *ppvObj = nullptr;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfTextInputProcessor)) {
        *ppvObj = static_cast<ITfTextInputProcessor*>(this);
    } else if (IsEqualIID(riid, IID_ITfKeyEventSink)) {
        *ppvObj = static_cast<ITfKeyEventSink*>(this);
    } else if (IsEqualIID(riid, IID_ITfCompositionSink)) {
        *ppvObj = static_cast<ITfCompositionSink*>(this);
    } else if (IsEqualIID(riid, IID_ITfDisplayAttributeProvider)) {
        *ppvObj = static_cast<ITfDisplayAttributeProvider*>(this);
    } else {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) TSFBridge::AddRef() {
    return ++refCount_;
}

STDMETHODIMP_(ULONG) TSFBridge::Release() {
    ULONG count = --refCount_;
    if (count == 0) {
        delete this;
    }
    return count;
}

// ITfTextInputProcessor
/**
 * Activate - 激活输入法
 * Task 5.1: 实现 ITfTextInputProcessor
 * 
 * 当用户选择此输入法时由 TSF 调用。
 * 职责：
 * 1. 保存 ITfThreadMgr 和 TfClientId
 * 2. 初始化键盘事件接收器
 * 3. 激活 InputEngine
 * 
 * Requirements: 1.1, 1.5
 */
STDMETHODIMP TSFBridge::Activate(ITfThreadMgr* pThreadMgr, TfClientId tfClientId) {
    if (!pThreadMgr) {
        return E_INVALIDARG;
    }
    
    // 保存 TSF 对象
    threadMgr_ = pThreadMgr;
    threadMgr_->AddRef();
    clientId_ = tfClientId;
    
    // 初始化键盘事件接收器
    HRESULT hr = initKeySink();
    if (FAILED(hr)) {
        // 清理并返回错误
        threadMgr_->Release();
        threadMgr_ = nullptr;
        clientId_ = TF_CLIENTID_NULL;
        return hr;
    }
    
    // 激活 InputEngine
    if (inputEngine_) {
        inputEngine_->activate();
    }
    
    // 更新 WindowsBridge 的 TSFBridge 引用
    if (windowsBridge_) {
        windowsBridge_->setTSFBridge(this);
    }
    
    activated_ = true;
    
    return S_OK;
}

/**
 * Deactivate - 停用输入法
 * Task 5.1: 实现 ITfTextInputProcessor
 * 
 * 当用户切换到其他输入法时由 TSF 调用。
 * 职责：
 * 1. 清理键盘事件接收器
 * 2. 结束当前 composition
 * 3. 释放 TSF 资源
 * 4. 停用 InputEngine
 * 
 * Requirements: 1.1, 1.5
 */
STDMETHODIMP TSFBridge::Deactivate() {
    activated_ = false;
    
    // 结束当前 composition
    if (composition_) {
        endComposition();
    }
    
    // 清理键盘事件接收器
    uninitKeySink();
    
    // 停用 InputEngine
    if (inputEngine_) {
        inputEngine_->deactivate();
    }
    
    // 隐藏候选词窗口
    if (candidateWindow_) {
        candidateWindow_->hideWindow();
    }
    
    // 释放 TSF 资源
    if (threadMgr_) {
        threadMgr_->Release();
        threadMgr_ = nullptr;
    }
    clientId_ = TF_CLIENTID_NULL;
    currentContext_ = nullptr;
    
    return S_OK;
}

// ITfKeyEventSink
/**
 * OnSetFocus - 处理焦点变化
 * Task 5.2: 实现 ITfKeyEventSink
 * 
 * 当输入焦点变化时由 TSF 调用。
 * 
 * @param fForeground TRUE 表示获得焦点，FALSE 表示失去焦点
 * Requirements: 1.2
 */
STDMETHODIMP TSFBridge::OnSetFocus(BOOL fForeground) {
    if (fForeground) {
        // 获得焦点
        // 重置 Shift 键状态
        shiftKeyPressed_ = false;
        otherKeyPressedWithShift_ = false;
    } else {
        // 失去焦点
        // 可以选择隐藏候选词窗口
        if (candidateWindow_) {
            candidateWindow_->hideWindow();
        }
    }
    
    return S_OK;
}

/**
 * OnTestKeyDown - 预测试按键按下
 * Task 5.2: 实现 ITfKeyEventSink
 * 
 * TSF 在实际处理按键前调用此方法，询问输入法是否要处理此按键。
 * 返回 pfEaten=TRUE 表示输入法将处理此按键。
 * 
 * @param pContext 当前上下文
 * @param wParam 虚拟键码
 * @param lParam 按键信息
 * @param pfEaten 输出：是否消费此按键
 * Requirements: 2.3
 */
STDMETHODIMP TSFBridge::OnTestKeyDown(ITfContext* pContext, WPARAM wParam, 
                                       LPARAM lParam, BOOL* pfEaten) {
    if (!pfEaten) {
        return E_INVALIDARG;
    }
    
    *pfEaten = FALSE;
    
    if (!inputEngine_ || !activated_) {
        return S_OK;
    }
    
    // 检查是否为修饰键（修饰键不消费）
    if (isModifierKey(wParam)) {
        return S_OK;
    }
    
    // 英文模式下不消费任何按键
    if (inputEngine_->getMode() == InputMode::English) {
        return S_OK;
    }
    
    // 中文模式下，检查是否应该消费此按键
    // 如果正在输入（有 preedit），消费大部分按键
    if (inputEngine_->isComposing()) {
        // 正在输入时，消费字母、数字、标点、功能键等
        if (isCharacterKey(wParam) || isNavigationKey(wParam) ||
            wParam == VK_BACK || wParam == VK_ESCAPE || 
            wParam == VK_RETURN || wParam == VK_SPACE ||
            wParam == VK_PRIOR || wParam == VK_NEXT) {
            *pfEaten = TRUE;
        }
    } else {
        // 未输入时，只消费字母键（开始新的输入）
        if (wParam >= 'A' && wParam <= 'Z') {
            *pfEaten = TRUE;
        }
    }
    
    return S_OK;
}

/**
 * OnTestKeyUp - 预测试按键释放
 * Task 5.2: 实现 ITfKeyEventSink
 * 
 * @param pContext 当前上下文
 * @param wParam 虚拟键码
 * @param lParam 按键信息
 * @param pfEaten 输出：是否消费此按键
 * Requirements: 2.7
 */
STDMETHODIMP TSFBridge::OnTestKeyUp(ITfContext* pContext, WPARAM wParam, 
                                     LPARAM lParam, BOOL* pfEaten) {
    if (!pfEaten) {
        return E_INVALIDARG;
    }
    
    *pfEaten = FALSE;
    
    // Shift 键释放需要特殊处理（用于切换模式）
    if (wParam == VK_SHIFT || wParam == VK_LSHIFT || wParam == VK_RSHIFT) {
        if (shiftKeyPressed_ && !otherKeyPressedWithShift_) {
            // 可能需要切换模式，消费此事件
            *pfEaten = TRUE;
        }
    }
    
    return S_OK;
}

/**
 * OnKeyDown - 处理按键按下
 * Task 5.2: 实现 ITfKeyEventSink
 * 
 * 实际处理按键事件的核心方法。
 * 职责：
 * 1. 转换键码
 * 2. 调用 InputEngine::processKeyEvent
 * 3. 根据返回值设置 pfEaten
 * 4. 更新候选词窗口位置
 * 
 * @param pContext 当前上下文
 * @param wParam 虚拟键码
 * @param lParam 按键信息
 * @param pfEaten 输出：是否消费此按键
 * Requirements: 2.3, 2.4, 2.5, 2.6
 */
STDMETHODIMP TSFBridge::OnKeyDown(ITfContext* pContext, WPARAM wParam, 
                                   LPARAM lParam, BOOL* pfEaten) {
    if (!pfEaten) {
        return E_INVALIDARG;
    }
    
    *pfEaten = FALSE;
    
    if (!inputEngine_ || !activated_) {
        return S_OK;
    }
    
    // 更新当前上下文
    currentContext_ = pContext;
    if (windowsBridge_) {
        windowsBridge_->setContext(pContext);
    }
    
    // 处理 Shift 键状态跟踪
    if (wParam == VK_SHIFT || wParam == VK_LSHIFT || wParam == VK_RSHIFT) {
        shiftKeyPressed_ = true;
        otherKeyPressedWithShift_ = false;
        shiftPressTime_ = GetTickCount();
        return S_OK;  // Shift 键本身不消费
    }
    
    // 如果 Shift 按下期间有其他键按下，标记
    if (shiftKeyPressed_) {
        otherKeyPressedWithShift_ = true;
    }
    
    // 检查是否为其他修饰键（不消费）
    if (isModifierKey(wParam)) {
        return S_OK;
    }
    
    // 转换键码
    int rimeKeyCode = convertVirtualKeyToRime(wParam, lParam);
    int rimeModifiers = convertModifiers();
    
    if (rimeKeyCode == 0) {
        return S_OK;
    }
    
    // 调用 InputEngine 处理按键
    // Requirements: 2.4, 2.5 - 根据 InputEngine 返回值设置 pfEaten
    bool handled = inputEngine_->processKeyEvent(rimeKeyCode, rimeModifiers);
    *pfEaten = handled ? TRUE : FALSE;
    
    // 如果按键被处理，更新候选词窗口位置
    if (handled) {
        updateCandidateWindowPosition();
    }
    
    return S_OK;
}

/**
 * OnKeyUp - 处理按键释放
 * Task 5.2: 实现 ITfKeyEventSink
 * 
 * 主要用于处理 Shift 键释放切换模式。
 * 
 * @param pContext 当前上下文
 * @param wParam 虚拟键码
 * @param lParam 按键信息
 * @param pfEaten 输出：是否消费此按键
 * Requirements: 2.7
 */
STDMETHODIMP TSFBridge::OnKeyUp(ITfContext* pContext, WPARAM wParam, 
                                 LPARAM lParam, BOOL* pfEaten) {
    if (!pfEaten) {
        return E_INVALIDARG;
    }
    
    *pfEaten = FALSE;
    
    if (!inputEngine_ || !activated_) {
        return S_OK;
    }
    
    // 处理 Shift 键释放切换模式
    if (wParam == VK_SHIFT || wParam == VK_LSHIFT || wParam == VK_RSHIFT) {
        if (shiftKeyPressed_ && !otherKeyPressedWithShift_) {
            // 检查按下时间（避免长按）
            DWORD elapsed = GetTickCount() - shiftPressTime_;
            if (elapsed < 500) {  // 500ms 内释放
                handleShiftKeyRelease();
                *pfEaten = TRUE;
            }
        }
        shiftKeyPressed_ = false;
        otherKeyPressedWithShift_ = false;
    }
    
    return S_OK;
}

/**
 * OnPreservedKey - 处理保留键
 * Task 5.2: 实现 ITfKeyEventSink
 * 
 * 处理通过 ITfKeystrokeMgr::PreserveKey 注册的保留键。
 * 当前实现不使用保留键。
 * 
 * @param pContext 当前上下文
 * @param rguid 保留键 GUID
 * @param pfEaten 输出：是否消费此按键
 */
STDMETHODIMP TSFBridge::OnPreservedKey(ITfContext* pContext, REFGUID rguid, 
                                        BOOL* pfEaten) {
    if (pfEaten) {
        *pfEaten = FALSE;
    }
    return S_OK;
}

// ITfCompositionSink
/**
 * OnCompositionTerminated - 处理 composition 被外部终止
 * Task 5.4: 实现 ITfCompositionSink
 * 
 * 当 composition 被应用程序或其他组件终止时由 TSF 调用。
 * 需要清理内部状态。
 * 
 * @param ecWrite 编辑 cookie
 * @param pComposition 被终止的 composition
 * Requirements: 1.3, 3.3
 */
STDMETHODIMP TSFBridge::OnCompositionTerminated(TfEditCookie ecWrite, 
                                                 ITfComposition* pComposition) {
    // 检查是否是我们的 composition
    if (pComposition == composition_) {
        // 释放 composition 引用
        if (composition_) {
            composition_->Release();
            composition_ = nullptr;
        }
        
        // 重置 InputEngine 状态
        if (inputEngine_) {
            inputEngine_->reset();
        }
        
        // 隐藏候选词窗口
        if (candidateWindow_) {
            candidateWindow_->hideWindow();
        }
    }
    
    return S_OK;
}

// ITfDisplayAttributeProvider
/**
 * EnumDisplayAttributeInfo - 枚举显示属性
 * Task 5.5: 实现 ITfDisplayAttributeProvider（可选）
 * 
 * 返回输入法支持的显示属性枚举器。
 * 当前实现返回 E_NOTIMPL，使用系统默认样式。
 * 
 * @param ppEnum 输出：显示属性枚举器
 * Requirements: 4.1
 */
STDMETHODIMP TSFBridge::EnumDisplayAttributeInfo(IEnumTfDisplayAttributeInfo** ppEnum) {
    // 可选实现：返回 E_NOTIMPL 使用系统默认样式
    // 如果需要自定义 preedit 下划线样式，可以在这里实现
    if (ppEnum) {
        *ppEnum = nullptr;
    }
    return E_NOTIMPL;
}

/**
 * GetDisplayAttributeInfo - 获取指定的显示属性
 * Task 5.5: 实现 ITfDisplayAttributeProvider（可选）
 * 
 * 根据 GUID 返回对应的显示属性信息。
 * 当前实现返回 E_NOTIMPL，使用系统默认样式。
 * 
 * @param guid 显示属性 GUID
 * @param ppInfo 输出：显示属性信息
 * Requirements: 4.1
 */
STDMETHODIMP TSFBridge::GetDisplayAttributeInfo(REFGUID guid, 
                                                 ITfDisplayAttributeInfo** ppInfo) {
    // 可选实现：返回 E_NOTIMPL 使用系统默认样式
    if (ppInfo) {
        *ppInfo = nullptr;
    }
    return E_NOTIMPL;
}

// 输入组合管理
/**
 * startComposition - 开始新的输入组合
 * Task 5.4: 实现 ITfCompositionSink
 * 
 * 创建新的 TSF composition，用于管理 preedit 文本。
 * 
 * @param pContext TSF 上下文
 * @return HRESULT
 * Requirements: 1.3
 */
HRESULT TSFBridge::startComposition(ITfContext* pContext) {
    if (!pContext) {
        return E_INVALIDARG;
    }
    
    // 如果已经有 composition，先结束它
    if (composition_) {
        endComposition();
    }
    
    // 获取 ITfContextComposition 接口
    ITfContextComposition* contextComposition = nullptr;
    HRESULT hr = pContext->QueryInterface(IID_ITfContextComposition, 
                                           reinterpret_cast<void**>(&contextComposition));
    if (FAILED(hr) || !contextComposition) {
        return hr;
    }
    
    // 获取当前插入点
    ITfInsertAtSelection* insertAtSelection = nullptr;
    hr = pContext->QueryInterface(IID_ITfInsertAtSelection, 
                                   reinterpret_cast<void**>(&insertAtSelection));
    if (FAILED(hr) || !insertAtSelection) {
        contextComposition->Release();
        return hr;
    }
    
    // 在当前选择位置插入空范围
    ITfRange* range = nullptr;
    hr = insertAtSelection->InsertTextAtSelection(
        windowsBridge_ ? windowsBridge_->getEditCookie() : TF_INVALID_COOKIE,
        TF_IAS_QUERYONLY,
        nullptr,
        0,
        &range
    );
    
    insertAtSelection->Release();
    
    if (FAILED(hr) || !range) {
        contextComposition->Release();
        return hr;
    }
    
    // 开始 composition
    hr = contextComposition->StartComposition(
        windowsBridge_ ? windowsBridge_->getEditCookie() : TF_INVALID_COOKIE,
        range,
        static_cast<ITfCompositionSink*>(this),
        &composition_
    );
    
    range->Release();
    contextComposition->Release();
    
    if (SUCCEEDED(hr) && composition_) {
        currentContext_ = pContext;
    }
    
    return hr;
}

/**
 * endComposition - 结束当前输入组合
 * Task 5.4: 实现 ITfCompositionSink
 * 
 * 结束当前的 TSF composition。
 * 
 * @return HRESULT
 * Requirements: 3.3
 */
HRESULT TSFBridge::endComposition() {
    if (!composition_) {
        return S_OK;
    }
    
    // 结束 composition
    HRESULT hr = composition_->EndComposition(
        windowsBridge_ ? windowsBridge_->getEditCookie() : TF_INVALID_COOKIE
    );
    
    // 释放 composition
    composition_->Release();
    composition_ = nullptr;
    
    return hr;
}

// 文本操作
/**
 * commitText - 提交文本到应用
 * Task 5.4: 实现文本提交
 * 
 * 通过 TSF 将文本提交到当前应用。
 * 
 * @param text 要提交的文本（UTF-16）
 * @return HRESULT
 * Requirements: 3.1, 3.3
 */
HRESULT TSFBridge::commitText(const std::wstring& text) {
    if (text.empty()) {
        return S_OK;
    }
    
    if (!currentContext_) {
        return E_FAIL;
    }
    
    // 如果有 composition，先结束它
    if (composition_) {
        // 获取 composition 的范围
        ITfRange* range = nullptr;
        HRESULT hr = composition_->GetRange(&range);
        
        if (SUCCEEDED(hr) && range) {
            // 设置文本到范围
            TfEditCookie ec = windowsBridge_ ? windowsBridge_->getEditCookie() : TF_INVALID_COOKIE;
            hr = range->SetText(ec, 0, text.c_str(), static_cast<LONG>(text.length()));
            range->Release();
        }
        
        // 结束 composition
        endComposition();
        
        return hr;
    }
    
    // 没有 composition，直接插入文本
    ITfInsertAtSelection* insertAtSelection = nullptr;
    HRESULT hr = currentContext_->QueryInterface(IID_ITfInsertAtSelection, 
                                                  reinterpret_cast<void**>(&insertAtSelection));
    if (FAILED(hr) || !insertAtSelection) {
        return hr;
    }
    
    TfEditCookie ec = windowsBridge_ ? windowsBridge_->getEditCookie() : TF_INVALID_COOKIE;
    ITfRange* range = nullptr;
    hr = insertAtSelection->InsertTextAtSelection(
        ec,
        0,
        text.c_str(),
        static_cast<LONG>(text.length()),
        &range
    );
    
    if (range) {
        range->Release();
    }
    insertAtSelection->Release();
    
    return hr;
}

/**
 * updatePreedit - 更新 preedit 文本
 * Task 5.4: 实现 preedit 更新
 * 
 * 更新 composition 中的 preedit 文本。
 * 
 * @param preedit preedit 文本（UTF-16）
 * @param caretPos 光标位置
 * @return HRESULT
 * Requirements: 4.1, 4.2
 */
HRESULT TSFBridge::updatePreedit(const std::wstring& preedit, int caretPos) {
    if (!currentContext_) {
        return E_FAIL;
    }
    
    // 如果没有 composition，创建一个
    if (!composition_) {
        HRESULT hr = startComposition(currentContext_);
        if (FAILED(hr)) {
            return hr;
        }
    }
    
    if (!composition_) {
        return E_FAIL;
    }
    
    // 获取 composition 的范围
    ITfRange* range = nullptr;
    HRESULT hr = composition_->GetRange(&range);
    if (FAILED(hr) || !range) {
        return hr;
    }
    
    // 设置 preedit 文本
    TfEditCookie ec = windowsBridge_ ? windowsBridge_->getEditCookie() : TF_INVALID_COOKIE;
    hr = range->SetText(ec, TF_ST_CORRECTION, preedit.c_str(), static_cast<LONG>(preedit.length()));
    
    range->Release();
    
    return hr;
}

/**
 * clearPreedit - 清除 preedit 文本
 * Task 5.4: 实现 preedit 清除
 * 
 * 清除当前的 preedit 文本并结束 composition。
 * 
 * @return HRESULT
 * Requirements: 4.3
 */
HRESULT TSFBridge::clearPreedit() {
    if (!composition_) {
        return S_OK;
    }
    
    // 获取 composition 的范围
    ITfRange* range = nullptr;
    HRESULT hr = composition_->GetRange(&range);
    
    if (SUCCEEDED(hr) && range) {
        // 清空文本
        TfEditCookie ec = windowsBridge_ ? windowsBridge_->getEditCookie() : TF_INVALID_COOKIE;
        range->SetText(ec, 0, L"", 0);
        range->Release();
    }
    
    // 结束 composition
    return endComposition();
}

// 私有方法
int TSFBridge::convertVirtualKeyToRime(WPARAM vk, LPARAM lParam) {
    UINT scanCode = (lParam >> 16) & 0xFF;
    bool extended = (lParam & (1 << 24)) != 0;
    return suyan::convertVirtualKeyToRime(vk, scanCode, extended);
}

int TSFBridge::convertModifiers() {
    return suyan::convertModifiersToRime();
}

HRESULT TSFBridge::initKeySink() {
    // Task 5.1: 初始化键盘事件接收器
    // 获取 ITfKeystrokeMgr 接口
    if (!threadMgr_) {
        return E_FAIL;
    }
    
    ITfKeystrokeMgr* keystrokeMgr = nullptr;
    HRESULT hr = threadMgr_->QueryInterface(IID_ITfKeystrokeMgr, 
                                             reinterpret_cast<void**>(&keystrokeMgr));
    if (FAILED(hr) || !keystrokeMgr) {
        return hr;
    }
    
    // 注册键盘事件接收器
    hr = keystrokeMgr->AdviseKeyEventSink(clientId_, 
                                           static_cast<ITfKeyEventSink*>(this), 
                                           TRUE);  // TRUE = 前台接收器
    
    keystrokeMgr->Release();
    
    if (SUCCEEDED(hr)) {
        keySinkCookie_ = 1;  // 标记已注册（实际 cookie 由 TSF 管理）
    }
    
    return hr;
}

HRESULT TSFBridge::uninitKeySink() {
    // Task 5.1: 清理键盘事件接收器
    if (!threadMgr_ || keySinkCookie_ == TF_INVALID_COOKIE) {
        return S_OK;
    }
    
    ITfKeystrokeMgr* keystrokeMgr = nullptr;
    HRESULT hr = threadMgr_->QueryInterface(IID_ITfKeystrokeMgr, 
                                             reinterpret_cast<void**>(&keystrokeMgr));
    if (FAILED(hr) || !keystrokeMgr) {
        return hr;
    }
    
    // 注销键盘事件接收器
    hr = keystrokeMgr->UnadviseKeyEventSink(clientId_);
    
    keystrokeMgr->Release();
    keySinkCookie_ = TF_INVALID_COOKIE;
    
    return hr;
}

HRESULT TSFBridge::initThreadMgrSink() {
    // Task 5.1: 初始化线程管理器事件接收器
    // 当前实现不需要监听线程管理器事件
    // 如果需要监听焦点变化等事件，可以在这里实现
    return S_OK;
}

HRESULT TSFBridge::uninitThreadMgrSink() {
    // Task 5.1: 清理线程管理器事件接收器
    return S_OK;
}

void TSFBridge::updateCandidateWindowPosition() {
    // Task 5.2: 更新候选词窗口位置
    if (!inputEngine_ || !candidateWindow_ || !windowsBridge_) {
        return;
    }
    
    InputState state = inputEngine_->getState();
    
    if (state.isComposing && !state.candidates.empty()) {
        // 获取光标位置并显示候选词窗口
        CursorPosition cursorPos = windowsBridge_->getCursorPosition();
        QPoint pos(cursorPos.x, cursorPos.y + cursorPos.height);
        candidateWindow_->showAt(pos);
    } else if (!state.isComposing) {
        candidateWindow_->hideWindow();
    }
}

/**
 * handleShiftKeyRelease - 处理 Shift 键释放切换模式
 * Task 5.2: 实现 ITfKeyEventSink
 * 
 * 当 Shift 键单独按下并释放时，切换中英文模式。
 * 如果正在输入，先提交原始拼音再切换。
 */
void TSFBridge::handleShiftKeyRelease() {
    if (!inputEngine_) {
        return;
    }
    
    // 如果正在输入中文，提交当前的原始拼音字母
    if (inputEngine_->isComposing()) {
        InputState state = inputEngine_->getState();
        if (!state.rawInput.empty() && windowsBridge_) {
            // 提交原始拼音
            windowsBridge_->commitText(state.rawInput);
        }
        // 重置输入状态
        inputEngine_->reset();
        if (candidateWindow_) {
            candidateWindow_->hideWindow();
        }
    }
    
    // 切换模式
    inputEngine_->toggleMode();
}

// ============================================
// TSFBridgeFactory 实现
// ============================================

STDMETHODIMP TSFBridgeFactory::QueryInterface(REFIID riid, void** ppvObj) {
    if (!ppvObj) {
        return E_INVALIDARG;
    }

    *ppvObj = nullptr;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IClassFactory)) {
        *ppvObj = static_cast<IClassFactory*>(this);
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) TSFBridgeFactory::AddRef() {
    return 1; // 静态工厂，不需要引用计数
}

STDMETHODIMP_(ULONG) TSFBridgeFactory::Release() {
    return 1; // 静态工厂，不需要引用计数
}

STDMETHODIMP TSFBridgeFactory::CreateInstance(IUnknown* pUnkOuter, REFIID riid, 
                                               void** ppvObj) {
    if (!ppvObj) {
        return E_INVALIDARG;
    }

    *ppvObj = nullptr;

    if (pUnkOuter) {
        return CLASS_E_NOAGGREGATION;
    }

    TSFBridge* bridge = new (std::nothrow) TSFBridge();
    if (!bridge) {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = bridge->QueryInterface(riid, ppvObj);
    bridge->Release();

    return hr;
}

STDMETHODIMP TSFBridgeFactory::LockServer(BOOL fLock) {
    if (fLock) {
        InterlockedIncrement(&g_serverLockCount);
    } else {
        InterlockedDecrement(&g_serverLockCount);
    }
    return S_OK;
}

// ============================================
// DLL 导出函数
// ============================================

static TSFBridgeFactory g_factory;

extern "C" {

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv) {
    if (!ppv) {
        return E_INVALIDARG;
    }

    *ppv = nullptr;

    if (!IsEqualCLSID(rclsid, CLSID_SuYanTextService)) {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    return g_factory.QueryInterface(riid, ppv);
}

STDAPI DllCanUnloadNow() {
    return (g_dllRefCount == 0 && g_serverLockCount == 0) ? S_OK : S_FALSE;
}

/**
 * 注册 COM 组件到注册表
 * 
 * 创建以下注册表项：
 * HKCR\CLSID\{CLSID}\
 *   (Default) = "SuYan Input Method"
 *   InprocServer32\
 *     (Default) = <DLL 路径>
 *     ThreadingModel = "Apartment"
 */
static HRESULT RegisterCOMServer() {
    wchar_t clsidStr[64];
    if (!GuidToString(CLSID_SuYanTextService, clsidStr, 64)) {
        return E_FAIL;
    }

    // 获取 DLL 路径
    wchar_t dllPath[MAX_PATH];
    if (GetModuleFileNameW(g_hModule, dllPath, MAX_PATH) == 0) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    HRESULT hr;

    // 创建 CLSID 键
    wchar_t keyPath[256];
    StringCchPrintfW(keyPath, 256, L"CLSID\\%s", clsidStr);
    hr = CreateRegKeyAndSetValue(HKEY_CLASSES_ROOT, keyPath, nullptr, L"SuYan Input Method");
    if (FAILED(hr)) return hr;

    // 创建 InprocServer32 键
    StringCchPrintfW(keyPath, 256, L"CLSID\\%s\\InprocServer32", clsidStr);
    hr = CreateRegKeyAndSetValue(HKEY_CLASSES_ROOT, keyPath, nullptr, dllPath);
    if (FAILED(hr)) return hr;

    // 设置 ThreadingModel
    hr = CreateRegKeyAndSetValue(HKEY_CLASSES_ROOT, keyPath, L"ThreadingModel", L"Apartment");
    if (FAILED(hr)) return hr;

    return S_OK;
}

/**
 * 注销 COM 组件
 */
static HRESULT UnregisterCOMServer() {
    wchar_t clsidStr[64];
    if (!GuidToString(CLSID_SuYanTextService, clsidStr, 64)) {
        return E_FAIL;
    }

    wchar_t keyPath[256];
    StringCchPrintfW(keyPath, 256, L"CLSID\\%s", clsidStr);
    
    return DeleteRegKey(HKEY_CLASSES_ROOT, keyPath);
}

/**
 * 注册 TSF 输入法配置
 * 
 * 使用 ITfInputProcessorProfiles 接口注册输入法
 */
static HRESULT RegisterTSFProfile() {
    HRESULT hr;
    ITfInputProcessorProfiles* pProfiles = nullptr;

    // 创建 ITfInputProcessorProfiles 实例
    hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                          IID_ITfInputProcessorProfiles, reinterpret_cast<void**>(&pProfiles));
    if (FAILED(hr)) {
        return hr;
    }

    // 注册输入法
    hr = pProfiles->Register(CLSID_SuYanTextService);
    if (FAILED(hr)) {
        pProfiles->Release();
        return hr;
    }

    // 获取 DLL 路径（用于图标）
    wchar_t dllPath[MAX_PATH];
    if (GetModuleFileNameW(g_hModule, dllPath, MAX_PATH) == 0) {
        pProfiles->Release();
        return HRESULT_FROM_WIN32(GetLastError());
    }

    // 添加语言配置
    // LANGID: 0x0804 = 简体中文
    hr = pProfiles->AddLanguageProfile(
        CLSID_SuYanTextService,           // CLSID
        SUYAN_LANGID,                     // 语言 ID (简体中文)
        GUID_SuYanProfile,                // 配置 GUID
        L"素言输入法",                     // 显示名称
        static_cast<ULONG>(wcslen(L"素言输入法")),  // 名称长度
        dllPath,                          // 图标文件
        static_cast<ULONG>(wcslen(dllPath)),        // 图标文件路径长度
        0                                 // 图标索引
    );

    pProfiles->Release();
    return hr;
}

/**
 * 注销 TSF 输入法配置
 */
static HRESULT UnregisterTSFProfile() {
    HRESULT hr;
    ITfInputProcessorProfiles* pProfiles = nullptr;

    // 创建 ITfInputProcessorProfiles 实例
    hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                          IID_ITfInputProcessorProfiles, reinterpret_cast<void**>(&pProfiles));
    if (FAILED(hr)) {
        return hr;
    }

    // 注销输入法
    hr = pProfiles->Unregister(CLSID_SuYanTextService);

    pProfiles->Release();
    return hr;
}

/**
 * 注册 TSF 分类（Category）
 * 
 * 将输入法注册到以下分类：
 * - GUID_TFCAT_TIP_KEYBOARD: 键盘输入法
 * - GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER: 显示属性提供器
 */
static HRESULT RegisterTSFCategories() {
    HRESULT hr;
    ITfCategoryMgr* pCategoryMgr = nullptr;

    // 创建 ITfCategoryMgr 实例
    hr = CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER,
                          IID_ITfCategoryMgr, reinterpret_cast<void**>(&pCategoryMgr));
    if (FAILED(hr)) {
        return hr;
    }

    // 注册为键盘输入法
    hr = pCategoryMgr->RegisterCategory(
        CLSID_SuYanTextService,           // CLSID
        GUID_TFCAT_TIP_KEYBOARD,          // 分类 GUID
        CLSID_SuYanTextService            // 项目 GUID
    );
    if (FAILED(hr)) {
        pCategoryMgr->Release();
        return hr;
    }

    // 注册为显示属性提供器（可选，用于 preedit 样式）
    hr = pCategoryMgr->RegisterCategory(
        CLSID_SuYanTextService,
        GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER,
        CLSID_SuYanTextService
    );
    // 这个注册失败不是致命错误，继续执行

    pCategoryMgr->Release();
    return S_OK;
}

/**
 * 注销 TSF 分类
 */
static HRESULT UnregisterTSFCategories() {
    HRESULT hr;
    ITfCategoryMgr* pCategoryMgr = nullptr;

    // 创建 ITfCategoryMgr 实例
    hr = CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER,
                          IID_ITfCategoryMgr, reinterpret_cast<void**>(&pCategoryMgr));
    if (FAILED(hr)) {
        return hr;
    }

    // 注销键盘输入法分类
    pCategoryMgr->UnregisterCategory(
        CLSID_SuYanTextService,
        GUID_TFCAT_TIP_KEYBOARD,
        CLSID_SuYanTextService
    );

    // 注销显示属性提供器分类
    pCategoryMgr->UnregisterCategory(
        CLSID_SuYanTextService,
        GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER,
        CLSID_SuYanTextService
    );

    pCategoryMgr->Release();
    return S_OK;
}

/**
 * DllRegisterServer - 注册 DLL
 * 
 * 执行以下注册步骤：
 * 1. 注册 COM 服务器
 * 2. 注册 TSF 输入法配置
 * 3. 注册 TSF 分类
 */
STDAPI DllRegisterServer() {
    HRESULT hr;

    // 初始化 COM（如果尚未初始化）
    hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool comInitialized = SUCCEEDED(hr);

    // 1. 注册 COM 服务器
    hr = RegisterCOMServer();
    if (FAILED(hr)) {
        if (comInitialized) CoUninitialize();
        return hr;
    }

    // 2. 注册 TSF 输入法配置
    hr = RegisterTSFProfile();
    if (FAILED(hr)) {
        // 回滚 COM 注册
        UnregisterCOMServer();
        if (comInitialized) CoUninitialize();
        return hr;
    }

    // 3. 注册 TSF 分类
    hr = RegisterTSFCategories();
    if (FAILED(hr)) {
        // 回滚之前的注册
        UnregisterTSFProfile();
        UnregisterCOMServer();
        if (comInitialized) CoUninitialize();
        return hr;
    }

    if (comInitialized) CoUninitialize();
    return S_OK;
}

/**
 * DllUnregisterServer - 注销 DLL
 * 
 * 执行以下注销步骤（顺序与注册相反）：
 * 1. 注销 TSF 分类
 * 2. 注销 TSF 输入法配置
 * 3. 注销 COM 服务器
 */
STDAPI DllUnregisterServer() {
    // 初始化 COM（如果尚未初始化）
    HRESULT hrInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool comInitialized = SUCCEEDED(hrInit);

    // 1. 注销 TSF 分类
    UnregisterTSFCategories();

    // 2. 注销 TSF 输入法配置
    UnregisterTSFProfile();

    // 3. 注销 COM 服务器
    HRESULT hr = UnregisterCOMServer();

    if (comInitialized) CoUninitialize();
    return hr;
}

} // extern "C"

} // namespace suyan

#endif // _WIN32
