#pragma once

#include <msctf.h>

namespace suyan {

class TSFTextService;

class EditSession : public ITfEditSession {
public:
    EditSession(TSFTextService* pTextService, ITfContext* pContext);
    virtual ~EditSession();

    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    virtual STDMETHODIMP DoEditSession(TfEditCookie ec) = 0;

protected:
    TSFTextService* m_textService;
    ITfContext* m_context;

private:
    LONG m_refCount;
};

class GetTextExtentEditSession : public EditSession {
public:
    GetTextExtentEditSession(TSFTextService* pTextService, 
                             ITfContext* pContext,
                             ITfContextView* pContextView);
    ~GetTextExtentEditSession();

    STDMETHODIMP DoEditSession(TfEditCookie ec) override;

private:
    ITfContextView* m_contextView;
};

}  // namespace suyan
