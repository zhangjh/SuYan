#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <msctf.h>
#include <atomic>
#include <string>
#include "ipc_client.h"
#include "langbar_button.h"

namespace suyan {

extern const CLSID CLSID_SuYanTextService;
extern const GUID GUID_SuYanProfile;

class TSFTextService : public ITfTextInputProcessorEx,
                       public ITfThreadMgrEventSink,
                       public ITfKeyEventSink,
                       public ITfCompositionSink,
                       public ITfEditSession {
public:
    TSFTextService();
    ~TSFTextService();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ITfTextInputProcessor
    STDMETHODIMP Activate(ITfThreadMgr* pThreadMgr, TfClientId tfClientId) override;
    STDMETHODIMP Deactivate() override;

    // ITfTextInputProcessorEx
    STDMETHODIMP ActivateEx(ITfThreadMgr* pThreadMgr, TfClientId tfClientId, DWORD dwFlags) override;

    // ITfThreadMgrEventSink
    STDMETHODIMP OnInitDocumentMgr(ITfDocumentMgr* pDocMgr) override;
    STDMETHODIMP OnUninitDocumentMgr(ITfDocumentMgr* pDocMgr) override;
    STDMETHODIMP OnSetFocus(ITfDocumentMgr* pDocMgrFocus, ITfDocumentMgr* pDocMgrPrevFocus) override;
    STDMETHODIMP OnPushContext(ITfContext* pContext) override;
    STDMETHODIMP OnPopContext(ITfContext* pContext) override;

    // ITfKeyEventSink
    STDMETHODIMP OnSetFocus(BOOL fForeground) override;
    STDMETHODIMP OnTestKeyDown(ITfContext* pContext, WPARAM wParam, LPARAM lParam, BOOL* pfEaten) override;
    STDMETHODIMP OnTestKeyUp(ITfContext* pContext, WPARAM wParam, LPARAM lParam, BOOL* pfEaten) override;
    STDMETHODIMP OnKeyDown(ITfContext* pContext, WPARAM wParam, LPARAM lParam, BOOL* pfEaten) override;
    STDMETHODIMP OnKeyUp(ITfContext* pContext, WPARAM wParam, LPARAM lParam, BOOL* pfEaten) override;
    STDMETHODIMP OnPreservedKey(ITfContext* pContext, REFGUID rguid, BOOL* pfEaten) override;

    // ITfCompositionSink
    STDMETHODIMP OnCompositionTerminated(TfEditCookie ecWrite, ITfComposition* pComposition) override;

    // ITfEditSession
    STDMETHODIMP DoEditSession(TfEditCookie ec) override;

    // Accessors
    TfClientId getClientId() const { return m_clientId; }
    ITfThreadMgr* getThreadMgr() const { return m_threadMgr; }

    // Composition management
    void startComposition(ITfContext* pContext, TfEditCookie ec);
    void endComposition(ITfContext* pContext, TfEditCookie ec, bool clear);
    void setCompositionPosition(const RECT& rc);
    bool isComposing() const { return m_composition != nullptr; }
    ITfComposition* getComposition() const { return m_composition; }

private:
    bool initThreadMgrEventSink();
    void uninitThreadMgrEventSink();
    bool initKeyEventSink();
    void uninitKeyEventSink();
    bool initLangBarButton();
    void uninitLangBarButton();

    static void onMenuCallback(UINT menuId);

    void processKeyEvent(WPARAM wParam, LPARAM lParam, BOOL* pfEaten);
    void updateComposition(ITfContext* pContext);
    void updateCompositionWindow(ITfContext* pContext, TfEditCookie ec);
    void requestUpdateCompositionWindow(ITfContext* pContext);
    void abortComposition();

    LONG m_refCount;
    ITfThreadMgr* m_threadMgr;
    TfClientId m_clientId;
    DWORD m_threadMgrEventSinkCookie;
    ITfKeystrokeMgr* m_keystrokeMgr;
    bool m_activated;

    IPCClient m_ipc;
    LangBarButton* m_langBarButton;
    ITfLangBarItemMgr* m_langBarItemMgr;

    ITfComposition* m_composition;
    ITfContext* m_editSessionContext;
    bool m_composingOnServer;
    std::wstring m_commitText;

    bool m_testKeyDownPending;

    static TSFTextService* s_instance;
};

class TSFTextServiceFactory : public IClassFactory {
public:
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;
    STDMETHODIMP CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObj) override;
    STDMETHODIMP LockServer(BOOL fLock) override;
};

extern TSFTextServiceFactory g_classFactory;
extern std::atomic<LONG> g_dllRefCount;

void DllAddRef();
void DllRelease();

}  // namespace suyan
