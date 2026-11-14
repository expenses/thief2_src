



#include <lg.h>
#include <arqapi.h>
#include <arqstrrd.h>
#include <lgassert.h>

STDMETHODIMP cAsyncStreamReader::QueryInterface(REFIID riid, void **ppvObj)
{
    if (!IsEqualOrIUnknownGUID(riid, IID_IAsyncReadFulfiller)) {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    } else {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }
}

STDMETHODIMP_(ULONG) cAsyncStreamReader::AddRef()
{
    return __m_ulRefs.AddRef();
}

STDMETHODIMP_(ULONG) cAsyncStreamReader::Release()
{
    if (__m_ulRefs.Release() != 0) {
        return __m_ulRefs;
    }
    OnFinalRelease();
    return 0;
}

STDMETHODIMP cAsyncStreamReader::DoFulfill(const sARQRequest *, sARQResult *pResult) {
    AssertMsg(m_StreamRequest.pBufOut && (m_StreamRequest.flags & 1) == 0, "Dynamic ARQ requests not yet supported");
    if (m_StreamRequest.offset != -1) {
        LARGE_INTEGER li;
        li.QuadPart = m_StreamRequest.offset;
        m_StreamRequest.pSource->Seek(li, STREAM_SEEK_SET, NULL);
    }
    m_StreamRequest.pSource->Read(m_StreamRequest.pBufOut, m_StreamRequest.length, NULL);
    pResult->result = 0;
    pResult->buffer = m_StreamRequest.pBufOut;
    pResult->length = m_StreamRequest.length;
    pResult->flags = 0;
    pResult->streamIndex = 0;
    if (m_StreamRequest.pSource)
        m_StreamRequest.pSource->Release();
    m_StreamRequest.pSource = NULL;
    return S_OK;
}

STDMETHODIMP cAsyncStreamReader::DoKill(const sARQRequest *, BOOL)
{
    if (m_StreamRequest.pSource)
        m_StreamRequest.pSource->Release();
    m_StreamRequest.pSource = NULL;
    return S_OK;
}

#line 69