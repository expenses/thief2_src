#include <arq.h>
#include <arqapi.h>
#include <thrdtool.h>

#include <lg.h>

inline unsigned long cARQRequest::cRefCount::AddRef()
{
    return ++ul;
}

inline unsigned long cARQRequest::cRefCount::Release()
{
    return --ul;
}

inline cARQRequest::cRefCount::operator unsigned long()
{
    return ul;
}

STDMETHODIMP cARQRequest::QueryInterface(REFIID riid, void **ppvObj)
{
    if (!IsEqualOrIUnknownGUID(riid, IID_IAsyncReadControl))
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    *ppvObj = this;
    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) cARQRequest::AddRef()
{
    return __m_ulRefs.AddRef();
}

STDMETHODIMP_(ULONG) cARQRequest::Release()
{
    if (__m_ulRefs.Release())
        return (ULONG)__m_ulRefs;

    OnFinalRelease();
    return 0;
}

cARQRequest::~cARQRequest()
{
    if (m_ReqInfo.pFulfiller)
        m_ReqInfo.pFulfiller->Release();
    m_ReqInfo.pFulfiller = NULL;
}

STDMETHODIMP_(BOOL) cARQRequest::IsFulfilled()
{
    return m_State == kComplete;
}

STDMETHODIMP cARQRequest::Fulfill()
{
    cAutoLock autoLock(m_Lock);
    AddRef();
    int retVal;

    switch (m_State)
    {
    case kUnserviced:
    case kQueued:
        if (m_pOwningQueue)
        {
            cThreadLock *queueLock = &m_pOwningQueue->m_Lock;
            queueLock->Lock();
            if (m_pOwningQueue)
                m_pOwningQueue->RemoveRequest(this);
            queueLock->Unlock();
#line 77
            AssertMsg(!m_pOwningQueue, "RemoveRequest() failed");
        }

        m_State = kInProgress;

        retVal = m_ReqInfo.pFulfiller->DoFulfill(&m_ReqInfo, &m_Result);

        if (m_ReqInfo.pFulfiller)
            m_ReqInfo.pFulfiller->Release();
        m_ReqInfo.pFulfiller = NULL;

        switch (retVal)
        {
        case S_OK:
        case E_OUTOFMEMORY:
        case E_FAIL:
        case E_ACCESSDENIED:
            break;
        default:
            retVal = E_FAIL;
        }

        m_State = kComplete;
        break;

    case kInProgress:



        CriticalMsg("Should not be able to use request while in progress");
        retVal = S_OK;
        break;

    case kComplete:
        retVal = S_OK;
        break;

    case kKilled:
        retVal = E_FAIL;
        break;
    default:
        CriticalMsg("Unknown ARQ request state");
        retVal = E_FAIL;
        break;
    }

    Release();
    return retVal;
}

STDMETHODIMP cARQRequest::Kill(BOOL fDiscard)
{
    cAutoLock autoLock(m_Lock);
    AddRef();
    int retVal;

    switch (m_State)
    {
    case kUnserviced:
    case kQueued:
        if (m_pOwningQueue)
        {
            cThreadLock *queueLock = &m_pOwningQueue->m_Lock;
            queueLock->Lock();
            m_pOwningQueue->RemoveRequest(this);
            queueLock->Unlock();




#line 153
            AssertMsg(!m_pOwningQueue, "RemoveRequest() failed");
        }

        retVal = m_ReqInfo.pFulfiller->DoKill(&m_ReqInfo, fDiscard);

        if (m_ReqInfo.pFulfiller)
            m_ReqInfo.pFulfiller->Release();
        m_ReqInfo.pFulfiller = NULL;

        if (retVal != E_FAIL && retVal != S_OK)
            retVal = E_FAIL;

        m_Result.result = S_FALSE;
        m_Result.flags |= kARQKilled;
        m_State = kKilled;
        break;

    case kInProgress:
    case kComplete:
        retVal = E_FAIL;
        break;

    case kKilled:
        retVal = S_OK;
        break;

    default:
#line 191
        CriticalMsg("Unknown ARQ request state");
        retVal = E_FAIL;
        break;
    }

    Release();
    return retVal;
}

STDMETHODIMP cARQRequest::GetResult(sARQResult *pResult)
{
    cAutoLock autoLock(m_Lock);
    AddRef();
    int retVal;

    switch (m_State)
    {
    case kUnserviced:
    case kQueued:
    case kInProgress:
        retVal = S_FALSE;
        break;

    case kComplete:
    case kKilled:
        memcpy(pResult, &m_Result, sizeof(*pResult));
        retVal = S_OK;
        break;

    default:
#line 229
        CriticalMsg("Unknown ARQ request state");
        retVal = E_FAIL;
        break;
    }

    Release();
    return retVal;
}

inline void cARQRequest::OnFinalRelease()
{
    delete this;
}

STDMETHODIMP_(BOOL) cARQRequest::IsFinished()
{
    return m_State == kKilled || m_State == kComplete;
}
