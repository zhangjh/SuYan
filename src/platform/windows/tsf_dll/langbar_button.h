#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <msctf.h>
#include <vector>

namespace suyan {

// {C3D4E5F6-A7B8-9012-CDEF-234567890ABC}
extern const GUID GUID_LangBarButton;

enum MenuItemId : UINT {
    MenuToggleMode = 1,
    MenuToggleLayout = 2,
    MenuSettings = 3,
    MenuAbout = 4,
};

class LangBarButton : public ITfLangBarItemButton,
                      public ITfSource {
public:
    LangBarButton();
    ~LangBarButton();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ITfLangBarItem
    STDMETHODIMP GetInfo(TF_LANGBARITEMINFO* pInfo) override;
    STDMETHODIMP GetStatus(DWORD* pdwStatus) override;
    STDMETHODIMP Show(BOOL fShow) override;
    STDMETHODIMP GetTooltipString(BSTR* pbstrToolTip) override;

    // ITfLangBarItemButton
    STDMETHODIMP OnClick(TfLBIClick click, POINT pt, const RECT* prcArea) override;
    STDMETHODIMP InitMenu(ITfMenu* pMenu) override;
    STDMETHODIMP OnMenuSelect(UINT wID) override;
    STDMETHODIMP GetIcon(HICON* phIcon) override;
    STDMETHODIMP GetText(BSTR* pbstrText) override;

    // ITfSource
    STDMETHODIMP AdviseSink(REFIID riid, IUnknown* punk, DWORD* pdwCookie) override;
    STDMETHODIMP UnadviseSink(DWORD dwCookie) override;

    // Public methods
    void updateIcon(bool isChineseMode);
    void setMenuCallback(void (*callback)(UINT menuId));

private:
    void loadIcons();
    void notifySinkUpdate(DWORD dwFlags);

    LONG m_refCount;
    bool m_isChineseMode;
    HICON m_iconChinese;
    HICON m_iconEnglish;
    bool m_isShown;

    void (*m_menuCallback)(UINT menuId);

    struct SinkEntry {
        DWORD cookie;
        ITfLangBarItemSink* sink;
    };
    std::vector<SinkEntry> m_sinks;
    DWORD m_nextCookie;
};

}  // namespace suyan
