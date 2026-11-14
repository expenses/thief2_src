#pragma once

#include <arqapi.h>

class cAsyncStreamReader : public IAsyncReadFulfiller
{
public:
    class cRefCount
    {
    public:
        cRefCount() : ul(1) {}
        ULONG AddRef() {return ++ul;}
        ULONG Release() {return --ul;}
        operator ULONG() {return ul;}
    private:
        ULONG ul;
    };

    cAsyncStreamReader(const sARQStreamRequest &streamRequest);

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);
    STDMETHOD(DoFulfill)(THIS_ const sARQRequest *, sARQResult *);
    STDMETHOD(DoKill)(THIS_ const sARQRequest *, BOOL fDiscard);
    virtual ~cAsyncStreamReader();
    void OnFinalRelease() { delete this; }

    cRefCount __m_ulRefs;
    sARQStreamRequest m_StreamRequest;
};
