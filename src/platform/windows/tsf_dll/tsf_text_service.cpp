#include "tsf_text_service.h"
#include "edit_session.h"
#include "logger.h"
#include "../../../shared/ipc_protocol.h"
#include <ctffunc.h>
#include <vector>

namespace suyan {

const CLSID CLSID_SuYanTextService = {
    0xa1b2c3d4, 0xe5f6, 0x7890, {0xab, 0xcd, 0xef, 0x12, 0x34, 0x56, 0x78, 0x90}
};

const GUID GUID_SuYanProfile = {
    0xb2c3d4e5, 0xf6a7, 0x8901, {0xbc, 0xde, 0xf1, 0x23, 0x45, 0x67, 0x89, 0x01}
};

std::atomic<LONG> g_dllRefCount(0);
TSFTextServiceFactory g_classFactory;
TSFTextService* TSFTextService::s_instance = nullptr;

void DllAddRef() { g_dllRefCount++; }
void DllRelease() { g_dllRefCount--; }

TSFTextService::TSFTextService()
    : m_refCount(1)
    , m_threadMgr(nullptr)
    , m_clientId(TF_CLIENTID_NULL)
    , m_threadMgrEventSinkCookie(TF_INVALID_COOKIE)
    , m_keystrokeMgr(nullptr)
    , m_activated(false)
    , m_langBarButton(nullptr)
    , m_langBarItemMgr(nullptr)
    , m_composition(nullptr)
    , m_editSessionContext(nullptr)
    , m_composingOnServer(false)
    , m_testKeyDownPending(false) {
    DllAddRef();
    s_instance = this;
}

TSFTextService::~TSFTextService() {
    if (m_composition) {
        m_composition->Release();
        m_composition = nullptr;
    }
    if (s_instance == this) {
        s_instance = nullptr;
    }
    DllRelease();
}

STDMETHODIMP TSFTextService::QueryInterface(REFIID riid, void** ppvObj) {
    if (!ppvObj) return E_INVALIDARG;
    *ppvObj = nullptr;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfTextInputProcessor) ||
        IsEqualIID(riid, IID_ITfTextInputProcessorEx)) {
        *ppvObj = static_cast<ITfTextInputProcessorEx*>(this);
    } else if (IsEqualIID(riid, IID_ITfThreadMgrEventSink)) {
        *ppvObj = static_cast<ITfThreadMgrEventSink*>(this);
    } else if (IsEqualIID(riid, IID_ITfKeyEventSink)) {
        *ppvObj = static_cast<ITfKeyEventSink*>(this);
    } else if (IsEqualIID(riid, IID_ITfCompositionSink)) {
        *ppvObj = static_cast<ITfCompositionSink*>(this);
    } else if (IsEqualIID(riid, IID_ITfEditSession)) {
        *ppvObj = static_cast<ITfEditSession*>(this);
    } else {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) TSFTextService::AddRef() {
    return InterlockedIncrement(&m_refCount);
}

STDMETHODIMP_(ULONG) TSFTextService::Release() {
    LONG count = InterlockedDecrement(&m_refCount);
    if (count == 0) delete this;
    return count;
}

STDMETHODIMP TSFTextService::Activate(ITfThreadMgr* pThreadMgr, TfClientId tfClientId) {
    return ActivateEx(pThreadMgr, tfClientId, 0);
}

STDMETHODIMP TSFTextService::ActivateEx(ITfThreadMgr* pThreadMgr, TfClientId tfClientId, DWORD dwFlags) {
    (void)dwFlags;
    if (m_activated) return S_OK;

    m_threadMgr = pThreadMgr;
    m_threadMgr->AddRef();
    m_clientId = tfClientId;

    if (!m_ipc.connect()) {
        log::error("TSFTextService: Failed to connect to IPC server");
    }

    if (!initThreadMgrEventSink()) {
        Deactivate();
        return E_FAIL;
    }

    if (!initKeyEventSink()) {
        Deactivate();
        return E_FAIL;
    }

    initLangBarButton();
    m_activated = true;
    return S_OK;
}

STDMETHODIMP TSFTextService::Deactivate() {
    if (!m_activated) return S_OK;

    abortComposition();
    uninitLangBarButton();
    uninitKeyEventSink();
    uninitThreadMgrEventSink();
    m_ipc.disconnect();

    if (m_threadMgr) {
        m_threadMgr->Release();
        m_threadMgr = nullptr;
    }

    m_clientId = TF_CLIENTID_NULL;
    m_activated = false;
    return S_OK;
}

bool TSFTextService::initThreadMgrEventSink() {
    ITfSource* source = nullptr;
    HRESULT hr = m_threadMgr->QueryInterface(IID_ITfSource, reinterpret_cast<void**>(&source));
    if (FAILED(hr)) return false;

    hr = source->AdviseSink(IID_ITfThreadMgrEventSink,
                            static_cast<ITfThreadMgrEventSink*>(this),
                            &m_threadMgrEventSinkCookie);
    source->Release();
    return SUCCEEDED(hr);
}

void TSFTextService::uninitThreadMgrEventSink() {
    if (m_threadMgrEventSinkCookie == TF_INVALID_COOKIE) return;

    ITfSource* source = nullptr;
    if (SUCCEEDED(m_threadMgr->QueryInterface(IID_ITfSource, reinterpret_cast<void**>(&source)))) {
        source->UnadviseSink(m_threadMgrEventSinkCookie);
        source->Release();
    }
    m_threadMgrEventSinkCookie = TF_INVALID_COOKIE;
}

bool TSFTextService::initKeyEventSink() {
    HRESULT hr = m_threadMgr->QueryInterface(IID_ITfKeystrokeMgr,
                                              reinterpret_cast<void**>(&m_keystrokeMgr));
    if (FAILED(hr)) return false;

    hr = m_keystrokeMgr->AdviseKeyEventSink(m_clientId,
                                             static_cast<ITfKeyEventSink*>(this),
                                             TRUE);
    if (FAILED(hr)) {
        m_keystrokeMgr->Release();
        m_keystrokeMgr = nullptr;
        return false;
    }
    return true;
}

void TSFTextService::uninitKeyEventSink() {
    if (!m_keystrokeMgr) return;
    m_keystrokeMgr->UnadviseKeyEventSink(m_clientId);
    m_keystrokeMgr->Release();
    m_keystrokeMgr = nullptr;
}

bool TSFTextService::initLangBarButton() {
    HRESULT hr = m_threadMgr->QueryInterface(IID_ITfLangBarItemMgr,
                                              reinterpret_cast<void**>(&m_langBarItemMgr));
    if (FAILED(hr)) return false;

    m_langBarButton = new LangBarButton();
    m_langBarButton->setMenuCallback(onMenuCallback);

    hr = m_langBarItemMgr->AddItem(m_langBarButton);
    if (FAILED(hr)) {
        m_langBarButton->Release();
        m_langBarButton = nullptr;
        m_langBarItemMgr->Release();
        m_langBarItemMgr = nullptr;
        return false;
    }
    return true;
}

void TSFTextService::uninitLangBarButton() {
    if (m_langBarItemMgr && m_langBarButton) {
        m_langBarItemMgr->RemoveItem(m_langBarButton);
    }
    if (m_langBarButton) {
        m_langBarButton->Release();
        m_langBarButton = nullptr;
    }
    if (m_langBarItemMgr) {
        m_langBarItemMgr->Release();
        m_langBarItemMgr = nullptr;
    }
}

void TSFTextService::onMenuCallback(UINT menuId) {
    if (!s_instance) return;
    if (menuId == MenuToggleMode && s_instance->m_ipc.isConnected()) {
        s_instance->m_ipc.toggleMode();
        bool isChineseMode = s_instance->m_ipc.queryMode();
        if (s_instance->m_langBarButton) {
            s_instance->m_langBarButton->updateIcon(isChineseMode);
        }
    }
}

STDMETHODIMP TSFTextService::OnInitDocumentMgr(ITfDocumentMgr* pDocMgr) {
    (void)pDocMgr;
    return S_OK;
}

STDMETHODIMP TSFTextService::OnUninitDocumentMgr(ITfDocumentMgr* pDocMgr) {
    (void)pDocMgr;
    return S_OK;
}

STDMETHODIMP TSFTextService::OnSetFocus(ITfDocumentMgr* pDocMgrFocus, ITfDocumentMgr* pDocMgrPrevFocus) {
    (void)pDocMgrPrevFocus;
    if (pDocMgrFocus) {
        m_ipc.focusIn();
    } else {
        m_ipc.focusOut();
        abortComposition();
    }
    return S_OK;
}

STDMETHODIMP TSFTextService::OnPushContext(ITfContext* pContext) {
    (void)pContext;
    return S_OK;
}

STDMETHODIMP TSFTextService::OnPopContext(ITfContext* pContext) {
    (void)pContext;
    return S_OK;
}

STDMETHODIMP TSFTextService::OnSetFocus(BOOL fForeground) {
    if (fForeground) {
        m_ipc.focusIn();
    } else {
        m_ipc.focusOut();
        abortComposition();
    }
    return S_OK;
}

void TSFTextService::processKeyEvent(WPARAM wParam, LPARAM lParam, BOOL* pfEaten) {
    (void)lParam;
    
    if (!m_ipc.isConnected()) {
        *pfEaten = FALSE;
        return;
    }

    uint32_t vk = static_cast<uint32_t>(wParam);
    uint32_t mod = ipc::ModNone;
    if (GetKeyState(VK_SHIFT) & 0x8000) mod |= ipc::ModShift;
    if (GetKeyState(VK_CONTROL) & 0x8000) mod |= ipc::ModControl;
    if (GetKeyState(VK_MENU) & 0x8000) mod |= ipc::ModAlt;

    std::wstring commitText;
    bool processed = m_ipc.processKey(vk, mod, commitText);
    
    m_commitText = commitText;
    m_composingOnServer = processed && commitText.empty();
    *pfEaten = processed ? TRUE : FALSE;
}

STDMETHODIMP TSFTextService::OnTestKeyDown(ITfContext* pContext, WPARAM wParam, LPARAM lParam, BOOL* pfEaten) {
    if (m_testKeyDownPending) {
        *pfEaten = TRUE;
        return S_OK;
    }
    processKeyEvent(wParam, lParam, pfEaten);
    updateComposition(pContext);
    if (*pfEaten) m_testKeyDownPending = true;
    return S_OK;
}

STDMETHODIMP TSFTextService::OnKeyDown(ITfContext* pContext, WPARAM wParam, LPARAM lParam, BOOL* pfEaten) {
    if (m_testKeyDownPending) {
        m_testKeyDownPending = false;
        *pfEaten = TRUE;
    } else {
        processKeyEvent(wParam, lParam, pfEaten);
        updateComposition(pContext);
    }
    return S_OK;
}

STDMETHODIMP TSFTextService::OnTestKeyUp(ITfContext* pContext, WPARAM wParam, LPARAM lParam, BOOL* pfEaten) {
    (void)pContext;
    (void)wParam;
    (void)lParam;
    m_testKeyDownPending = false;
    *pfEaten = FALSE;
    return S_OK;
}

STDMETHODIMP TSFTextService::OnKeyUp(ITfContext* pContext, WPARAM wParam, LPARAM lParam, BOOL* pfEaten) {
    (void)pContext;
    (void)wParam;
    (void)lParam;
    m_testKeyDownPending = false;
    *pfEaten = FALSE;
    return S_OK;
}

STDMETHODIMP TSFTextService::OnPreservedKey(ITfContext* pContext, REFGUID rguid, BOOL* pfEaten) {
    (void)pContext;
    (void)rguid;
    *pfEaten = FALSE;
    return S_OK;
}

STDMETHODIMP TSFTextService::OnCompositionTerminated(TfEditCookie ecWrite, ITfComposition* pComposition) {
    (void)ecWrite;
    (void)pComposition;
    if (m_composition) {
        m_composition->Release();
        m_composition = nullptr;
    }
    return S_OK;
}

void TSFTextService::updateComposition(ITfContext* pContext) {
    if (!pContext) return;
    
    m_editSessionContext = pContext;
    
    HRESULT hr;
    pContext->RequestEditSession(m_clientId, 
                                 static_cast<ITfEditSession*>(this),
                                 TF_ES_ASYNCDONTCARE | TF_ES_READWRITE, &hr);
    
    requestUpdateCompositionWindow(pContext);
}

STDMETHODIMP TSFTextService::DoEditSession(TfEditCookie ec) {
    if (!m_editSessionContext) return E_FAIL;

    if (!m_commitText.empty()) {
        if (!isComposing()) {
            startComposition(m_editSessionContext, ec);
        }
        
        if (isComposing()) {
            ITfRange* pRange = nullptr;
            if (SUCCEEDED(m_composition->GetRange(&pRange)) && pRange) {
                pRange->SetText(ec, 0, m_commitText.c_str(), static_cast<LONG>(m_commitText.length()));
                pRange->Collapse(ec, TF_ANCHOR_END);
                
                TF_SELECTION sel;
                sel.range = pRange;
                sel.style.ase = TF_AE_NONE;
                sel.style.fInterimChar = FALSE;
                m_editSessionContext->SetSelection(ec, 1, &sel);
                
                pRange->Release();
            }
            endComposition(m_editSessionContext, ec, false);
        }
        m_commitText.clear();
    }

    if (m_composingOnServer && !isComposing()) {
        startComposition(m_editSessionContext, ec);
    } else if (!m_composingOnServer && isComposing()) {
        endComposition(m_editSessionContext, ec, true);
    }

    return S_OK;
}


void TSFTextService::startComposition(ITfContext* pContext, TfEditCookie ec) {
    ITfInsertAtSelection* pInsertAtSelection = nullptr;
    if (FAILED(pContext->QueryInterface(IID_ITfInsertAtSelection, 
                                        reinterpret_cast<void**>(&pInsertAtSelection)))) {
        return;
    }

    ITfRange* pRange = nullptr;
    if (FAILED(pInsertAtSelection->InsertTextAtSelection(ec, TF_IAS_QUERYONLY, nullptr, 0, &pRange))) {
        pInsertAtSelection->Release();
        return;
    }

    ITfContextComposition* pContextComposition = nullptr;
    if (FAILED(pContext->QueryInterface(IID_ITfContextComposition,
                                        reinterpret_cast<void**>(&pContextComposition)))) {
        pRange->Release();
        pInsertAtSelection->Release();
        return;
    }

    ITfComposition* pComposition = nullptr;
    HRESULT hr = pContextComposition->StartComposition(ec, pRange, 
                                                       static_cast<ITfCompositionSink*>(this),
                                                       &pComposition);
    if (SUCCEEDED(hr) && pComposition) {
        m_composition = pComposition;
        pRange->SetText(ec, TF_ST_CORRECTION, L" ", 1);
        pRange->Collapse(ec, TF_ANCHOR_START);
        
        TF_SELECTION sel;
        sel.range = pRange;
        sel.style.ase = TF_AE_NONE;
        sel.style.fInterimChar = FALSE;
        pContext->SetSelection(ec, 1, &sel);
    }

    pContextComposition->Release();
    pRange->Release();
    pInsertAtSelection->Release();
}

void TSFTextService::endComposition(ITfContext* pContext, TfEditCookie ec, bool clear) {
    (void)pContext;
    
    if (!m_composition) return;

    if (clear) {
        ITfRange* pRange = nullptr;
        if (SUCCEEDED(m_composition->GetRange(&pRange)) && pRange) {
            pRange->SetText(ec, 0, L"", 0);
            pRange->Release();
        }
    }

    m_composition->EndComposition(ec);
    m_composition->Release();
    m_composition = nullptr;
}

void TSFTextService::updateCompositionWindow(ITfContext* pContext, TfEditCookie ec) {
    if (!pContext) return;

    ITfContextView* pContextView = nullptr;
    if (FAILED(pContext->GetActiveView(&pContextView)) || !pContextView) return;

    RECT rc = {};
    BOOL fClipped = FALSE;
    bool gotPosition = false;

    if (m_composition) {
        ITfRange* pRange = nullptr;
        if (SUCCEEDED(m_composition->GetRange(&pRange)) && pRange) {
            pRange->Collapse(ec, TF_ANCHOR_START);
            if (SUCCEEDED(pContextView->GetTextExt(ec, pRange, &rc, &fClipped))) {
                if (rc.left != 0 || rc.top != 0) {
                    gotPosition = true;
                }
            }
            pRange->Release();
        }
    }

    if (!gotPosition) {
        TF_SELECTION selection;
        ULONG fetched = 0;
        if (SUCCEEDED(pContext->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &selection, &fetched)) && fetched > 0) {
            if (SUCCEEDED(pContextView->GetTextExt(ec, selection.range, &rc, &fClipped))) {
                if (rc.left != 0 || rc.top != 0) {
                    gotPosition = true;
                }
            }
            selection.range->Release();
        }
    }

    pContextView->Release();

    if (gotPosition) {
        setCompositionPosition(rc);
    }
}

void TSFTextService::requestUpdateCompositionWindow(ITfContext* pContext) {
    if (!pContext) return;

    ITfContextView* pContextView = nullptr;
    if (FAILED(pContext->GetActiveView(&pContextView)) || !pContextView) return;

    GetTextExtentEditSession* pEditSession = new GetTextExtentEditSession(this, pContext, pContextView);
    pContextView->Release();

    if (pEditSession) {
        HRESULT hr;
        pContext->RequestEditSession(m_clientId, pEditSession, TF_ES_ASYNCDONTCARE | TF_ES_READ, &hr);
        pEditSession->Release();
    }
}

void TSFTextService::setCompositionPosition(const RECT& rc) {
    int16_t x = static_cast<int16_t>(rc.left);
    int16_t y = static_cast<int16_t>(rc.top);
    int16_t w = static_cast<int16_t>(rc.right - rc.left);
    int16_t h = static_cast<int16_t>(rc.bottom - rc.top);
    m_ipc.updateCursor(x, y, w, h > 0 ? h : 20);
}

void TSFTextService::abortComposition() {
    if (m_composition) {
        m_composition->Release();
        m_composition = nullptr;
    }
    m_composingOnServer = false;
    m_commitText.clear();
}

STDMETHODIMP TSFTextServiceFactory::QueryInterface(REFIID riid, void** ppvObj) {
    if (!ppvObj) return E_INVALIDARG;
    *ppvObj = nullptr;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory)) {
        *ppvObj = static_cast<IClassFactory*>(this);
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) TSFTextServiceFactory::AddRef() {
    DllAddRef();
    return 2;
}

STDMETHODIMP_(ULONG) TSFTextServiceFactory::Release() {
    DllRelease();
    return 1;
}

STDMETHODIMP TSFTextServiceFactory::CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObj) {
    if (!ppvObj) return E_INVALIDARG;
    *ppvObj = nullptr;
    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    TSFTextService* service = new TSFTextService();
    if (!service) return E_OUTOFMEMORY;

    HRESULT hr = service->QueryInterface(riid, ppvObj);
    service->Release();
    return hr;
}

STDMETHODIMP TSFTextServiceFactory::LockServer(BOOL fLock) {
    if (fLock) DllAddRef();
    else DllRelease();
    return S_OK;
}

}  // namespace suyan
