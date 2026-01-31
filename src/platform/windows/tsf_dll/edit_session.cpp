#include "edit_session.h"
#include "tsf_text_service.h"
#include "logger.h"

namespace suyan {

EditSession::EditSession(TSFTextService* pTextService, ITfContext* pContext)
    : m_textService(pTextService)
    , m_context(pContext)
    , m_refCount(1) {
    if (m_context) {
        m_context->AddRef();
    }
}

EditSession::~EditSession() {
    if (m_context) {
        m_context->Release();
    }
}

STDMETHODIMP EditSession::QueryInterface(REFIID riid, void** ppvObject) {
    if (!ppvObject) {
        return E_INVALIDARG;
    }

    *ppvObject = nullptr;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfEditSession)) {
        *ppvObject = static_cast<ITfEditSession*>(this);
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) EditSession::AddRef() {
    return InterlockedIncrement(&m_refCount);
}

STDMETHODIMP_(ULONG) EditSession::Release() {
    LONG count = InterlockedDecrement(&m_refCount);
    if (count == 0) {
        delete this;
    }
    return count;
}

GetTextExtentEditSession::GetTextExtentEditSession(
    TSFTextService* pTextService,
    ITfContext* pContext,
    ITfContextView* pContextView)
    : EditSession(pTextService, pContext)
    , m_contextView(pContextView) {
    if (m_contextView) {
        m_contextView->AddRef();
    }
}

GetTextExtentEditSession::~GetTextExtentEditSession() {
    if (m_contextView) {
        m_contextView->Release();
    }
}

STDMETHODIMP GetTextExtentEditSession::DoEditSession(TfEditCookie ec) {
    if (!m_context || !m_contextView || !m_textService) {
        return E_FAIL;
    }

    RECT rc = {};
    BOOL fClipped = FALSE;
    bool gotPosition = false;

    // 优先使用composition range（像Weasel一样）
    ITfComposition* pComposition = m_textService->getComposition();
    if (pComposition) {
        ITfRange* pRange = nullptr;
        if (SUCCEEDED(pComposition->GetRange(&pRange)) && pRange) {
            pRange->Collapse(ec, TF_ANCHOR_START);
            if (SUCCEEDED(m_contextView->GetTextExt(ec, pRange, &rc, &fClipped))) {
                if (rc.left != 0 || rc.top != 0) {
                    gotPosition = true;
                }
            }
            pRange->Release();
        }
    }

    // 如果没有composition或获取失败，使用selection
    if (!gotPosition) {
        TF_SELECTION selection;
        ULONG fetched = 0;
        
        if (SUCCEEDED(m_context->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &selection, &fetched)) && fetched > 0) {
            if (SUCCEEDED(m_contextView->GetTextExt(ec, selection.range, &rc, &fClipped))) {
                if (rc.left != 0 || rc.top != 0) {
                    gotPosition = true;
                }
            }
            selection.range->Release();
        }
    }

    // 如果还是没有，使用InsertAtSelection
    if (!gotPosition) {
        ITfInsertAtSelection* pInsertAtSelection = nullptr;
        if (SUCCEEDED(m_context->QueryInterface(IID_ITfInsertAtSelection, 
                                               reinterpret_cast<void**>(&pInsertAtSelection)))) {
            ITfRange* pRange = nullptr;
            if (SUCCEEDED(pInsertAtSelection->InsertTextAtSelection(ec, TF_IAS_QUERYONLY, 
                                                                     nullptr, 0, &pRange)) && pRange) {
                if (SUCCEEDED(m_contextView->GetTextExt(ec, pRange, &rc, &fClipped))) {
                    if (rc.left != 0 || rc.top != 0) {
                        gotPosition = true;
                    }
                }
                pRange->Release();
            }
            pInsertAtSelection->Release();
        }
    }

    // 最后的fallback：使用GUITHREADINFO
    if (!gotPosition) {
        HWND hwnd = nullptr;
        if (SUCCEEDED(m_contextView->GetWnd(&hwnd)) && hwnd) {
            GUITHREADINFO gti = {};
            gti.cbSize = sizeof(gti);
            if (GetGUIThreadInfo(0, &gti) && gti.hwndCaret) {
                rc.left = gti.rcCaret.left;
                rc.top = gti.rcCaret.top;
                rc.right = gti.rcCaret.right;
                rc.bottom = gti.rcCaret.bottom;
                
                POINT pt = { rc.left, rc.bottom };
                ClientToScreen(gti.hwndCaret, &pt);
                rc.left = pt.x;
                rc.bottom = pt.y;
                rc.right = pt.x + (gti.rcCaret.right - gti.rcCaret.left);
                rc.top = pt.y - (gti.rcCaret.bottom - gti.rcCaret.top);
                
                gotPosition = true;
            }
        }
    }

    if (gotPosition) {
        m_textService->setCompositionPosition(rc);
        return S_OK;
    }

    return E_FAIL;
}

}  // namespace suyan
