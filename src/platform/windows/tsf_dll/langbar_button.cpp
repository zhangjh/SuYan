#include "langbar_button.h"
#include "tsf_text_service.h"
#include "logger.h"
#include <strsafe.h>
#include <olectl.h>

namespace suyan {

// {C3D4E5F6-A7B8-9012-CDEF-234567890ABC}
const GUID GUID_LangBarButton = {
    0xc3d4e5f6, 0xa7b8, 0x9012, {0xcd, 0xef, 0x23, 0x45, 0x67, 0x89, 0x0a, 0xbc}
};

extern HINSTANCE g_hInstance;

LangBarButton::LangBarButton()
    : m_refCount(1)
    , m_isChineseMode(true)
    , m_iconChinese(nullptr)
    , m_iconEnglish(nullptr)
    , m_isShown(true)
    , m_menuCallback(nullptr)
    , m_nextCookie(1) {
    log::debug("LangBarButton: Constructor");
    loadIcons();
}

LangBarButton::~LangBarButton() {
    log::debug("LangBarButton: Destructor");
    if (m_iconChinese) {
        DestroyIcon(m_iconChinese);
    }
    if (m_iconEnglish) {
        DestroyIcon(m_iconEnglish);
    }
    for (auto& entry : m_sinks) {
        if (entry.sink) {
            entry.sink->Release();
        }
    }
}

STDMETHODIMP LangBarButton::QueryInterface(REFIID riid, void** ppvObj) {
    if (!ppvObj) {
        return E_INVALIDARG;
    }

    *ppvObj = nullptr;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfLangBarItem) ||
        IsEqualIID(riid, IID_ITfLangBarItemButton)) {
        *ppvObj = static_cast<ITfLangBarItemButton*>(this);
    } else if (IsEqualIID(riid, IID_ITfSource)) {
        *ppvObj = static_cast<ITfSource*>(this);
    } else {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) LangBarButton::AddRef() {
    return InterlockedIncrement(&m_refCount);
}

STDMETHODIMP_(ULONG) LangBarButton::Release() {
    LONG count = InterlockedDecrement(&m_refCount);
    if (count == 0) {
        delete this;
    }
    return count;
}

// ITfLangBarItem
STDMETHODIMP LangBarButton::GetInfo(TF_LANGBARITEMINFO* pInfo) {
    if (!pInfo) {
        return E_INVALIDARG;
    }

    pInfo->clsidService = CLSID_SuYanTextService;
    pInfo->guidItem = GUID_LangBarButton;
    pInfo->dwStyle = TF_LBI_STYLE_BTN_BUTTON | TF_LBI_STYLE_BTN_MENU;
    pInfo->ulSort = 0;
    StringCchCopyW(pInfo->szDescription, ARRAYSIZE(pInfo->szDescription), L"素言输入法");

    return S_OK;
}

STDMETHODIMP LangBarButton::GetStatus(DWORD* pdwStatus) {
    if (!pdwStatus) {
        return E_INVALIDARG;
    }

    *pdwStatus = m_isShown ? 0 : TF_LBI_STATUS_HIDDEN;
    return S_OK;
}

STDMETHODIMP LangBarButton::Show(BOOL fShow) {
    m_isShown = (fShow != FALSE);
    notifySinkUpdate(TF_LBI_STATUS);
    return S_OK;
}

STDMETHODIMP LangBarButton::GetTooltipString(BSTR* pbstrToolTip) {
    if (!pbstrToolTip) {
        return E_INVALIDARG;
    }

    const wchar_t* tooltip = m_isChineseMode ? L"素言输入法 - 中文模式" : L"素言输入法 - 英文模式";
    *pbstrToolTip = SysAllocString(tooltip);

    return *pbstrToolTip ? S_OK : E_OUTOFMEMORY;
}

// ITfLangBarItemButton
STDMETHODIMP LangBarButton::OnClick(TfLBIClick click, POINT pt, const RECT* prcArea) {
    (void)pt;
    (void)prcArea;

    if (click == TF_LBI_CLK_LEFT) {
        if (m_menuCallback) {
            m_menuCallback(MenuToggleMode);
        }
    }

    return S_OK;
}

STDMETHODIMP LangBarButton::InitMenu(ITfMenu* pMenu) {
    if (!pMenu) {
        return E_INVALIDARG;
    }

    const wchar_t* modeText = m_isChineseMode ? L"切换到英文 (Shift)" : L"切换到中文 (Shift)";
    pMenu->AddMenuItem(MenuToggleMode, 0, nullptr, nullptr, modeText, 
                       static_cast<ULONG>(wcslen(modeText)), nullptr);

    pMenu->AddMenuItem(MenuToggleLayout, 0, nullptr, nullptr, L"切换横排/竖排", 
                       static_cast<ULONG>(wcslen(L"切换横排/竖排")), nullptr);

    pMenu->AddMenuItem(0, TF_LBMENUF_SEPARATOR, nullptr, nullptr, nullptr, 0, nullptr);

    pMenu->AddMenuItem(MenuSettings, TF_LBMENUF_GRAYED, nullptr, nullptr, L"设置...", 
                       static_cast<ULONG>(wcslen(L"设置...")), nullptr);

    pMenu->AddMenuItem(0, TF_LBMENUF_SEPARATOR, nullptr, nullptr, nullptr, 0, nullptr);

    pMenu->AddMenuItem(MenuAbout, 0, nullptr, nullptr, L"关于素言", 
                       static_cast<ULONG>(wcslen(L"关于素言")), nullptr);

    return S_OK;
}

STDMETHODIMP LangBarButton::OnMenuSelect(UINT wID) {
    log::debug("LangBarButton: OnMenuSelect, id=%u", wID);

    if (wID == MenuAbout) {
        MessageBoxW(nullptr, 
                    L"素言输入法\n版本 1.0.0\n\n基于 RIME 引擎",
                    L"关于素言",
                    MB_OK | MB_ICONINFORMATION);
        return S_OK;
    }

    if (m_menuCallback) {
        m_menuCallback(wID);
    }

    return S_OK;
}

STDMETHODIMP LangBarButton::GetIcon(HICON* phIcon) {
    if (!phIcon) {
        return E_INVALIDARG;
    }

    HICON sourceIcon = m_isChineseMode ? m_iconChinese : m_iconEnglish;
    if (sourceIcon) {
        *phIcon = CopyIcon(sourceIcon);
    } else {
        *phIcon = nullptr;
    }

    return S_OK;
}

STDMETHODIMP LangBarButton::GetText(BSTR* pbstrText) {
    if (!pbstrText) {
        return E_INVALIDARG;
    }

    const wchar_t* text = m_isChineseMode ? L"中" : L"英";
    *pbstrText = SysAllocString(text);

    return *pbstrText ? S_OK : E_OUTOFMEMORY;
}

// ITfSource
STDMETHODIMP LangBarButton::AdviseSink(REFIID riid, IUnknown* punk, DWORD* pdwCookie) {
    if (!punk || !pdwCookie) {
        return E_INVALIDARG;
    }

    if (!IsEqualIID(riid, IID_ITfLangBarItemSink)) {
        return CONNECT_E_CANNOTCONNECT;
    }

    ITfLangBarItemSink* sink = nullptr;
    HRESULT hr = punk->QueryInterface(IID_ITfLangBarItemSink, reinterpret_cast<void**>(&sink));
    if (FAILED(hr)) {
        return hr;
    }

    SinkEntry entry;
    entry.cookie = m_nextCookie++;
    entry.sink = sink;
    m_sinks.push_back(entry);

    *pdwCookie = entry.cookie;
    log::debug("LangBarButton: AdviseSink, cookie=%lu", entry.cookie);

    return S_OK;
}

STDMETHODIMP LangBarButton::UnadviseSink(DWORD dwCookie) {
    for (auto it = m_sinks.begin(); it != m_sinks.end(); ++it) {
        if (it->cookie == dwCookie) {
            if (it->sink) {
                it->sink->Release();
            }
            m_sinks.erase(it);
            log::debug("LangBarButton: UnadviseSink, cookie=%lu", dwCookie);
            return S_OK;
        }
    }

    return CONNECT_E_NOCONNECTION;
}

// Public methods
void LangBarButton::updateIcon(bool isChineseMode) {
    if (m_isChineseMode == isChineseMode) {
        return;
    }

    m_isChineseMode = isChineseMode;
    log::debug("LangBarButton: updateIcon, isChineseMode=%d", isChineseMode);
    notifySinkUpdate(TF_LBI_ICON | TF_LBI_TEXT | TF_LBI_TOOLTIP);
}

void LangBarButton::setMenuCallback(void (*callback)(UINT menuId)) {
    m_menuCallback = callback;
}

// Private methods
void LangBarButton::loadIcons() {
    m_iconChinese = static_cast<HICON>(LoadImageW(
        g_hInstance,
        MAKEINTRESOURCEW(101),
        IMAGE_ICON,
        16, 16,
        LR_DEFAULTCOLOR
    ));

    if (!m_iconChinese) {
        log::warning("LangBarButton: Failed to load icon, error=%lu", GetLastError());
    }

    m_iconEnglish = m_iconChinese ? CopyIcon(m_iconChinese) : nullptr;
}

void LangBarButton::notifySinkUpdate(DWORD dwFlags) {
    for (auto& entry : m_sinks) {
        if (entry.sink) {
            entry.sink->OnUpdate(dwFlags);
        }
    }
}

}  // namespace suyan
