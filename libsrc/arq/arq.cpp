#include <arqapi.h>
#include <arq.h>
#include <arqstrrd.h>

#include <lgassert.h>
#include <aggmemb.h>
#include <gshlguid.h>


inline cARQRequest::cRefCount::cRefCount()
    : ul(1)
{
}

inline cARQRequest::cARQRequest(const sARQRequest &reqInfo)
    : m_ReqInfo(reqInfo),
      m_State(kUnserviced),
      m_pOwningQueue(NULL)
{
    m_ReqInfo.queue = 0;
    m_ReqInfo.pFulfiller->AddRef();
    memset(&m_Result, 0, sizeof(m_Result));
}


inline cAsyncStreamReader::cAsyncStreamReader(const sARQStreamRequest &streamRequest)
    : m_StreamRequest(streamRequest)
{
    m_StreamRequest.pSource->AddRef();
}

inline cAsyncStreamReader::~cAsyncStreamReader()
{
    if (m_StreamRequest.pSource)
        m_StreamRequest.pSource->Release();
    m_StreamRequest.pSource = NULL;
}


//
// cARQFulfillerThread ctor/dtor (ThreadProc lives in arqthred.cpp)
//

inline cARQFulfillerThread::cARQFulfillerThread(cARQRequestQueue &queue)
    : m_RequestQueue(&queue), m_LightPriority(0), m_NormalPriority(0)
{
}

//
// cOuterPointer
//

inline cARQManager::cOuterPointer::cOuterPointer()
    : m_pOuterUnknown(NULL)
{
}

inline void cARQManager::cOuterPointer::Init(IUnknown *pOuter)
{
    m_pOuterUnknown = pOuter;
}

inline cARQManager::cOuterPointer::operator IUnknown *()
{
    return m_pOuterUnknown;
}

//
// cRefCount
//

inline cARQManager::cComplexAggregateControl::cRefCount::cRefCount()
    : ul(1)
{
}

inline unsigned long cARQManager::cComplexAggregateControl::cRefCount::AddRef()
{
    return ++ul;
}

inline unsigned long cARQManager::cComplexAggregateControl::cRefCount::Release()
{
    return --ul;
}

inline cARQManager::cComplexAggregateControl::cRefCount::operator unsigned long()
{
    return ul;
}

//
// cComplexAggregateControl
//

inline cARQManager::cComplexAggregateControl::cComplexAggregateControl()
    : m_pAggregateMember(NULL)
{
}

inline void cARQManager::cComplexAggregateControl::InitControl(cARQManager *pManager)
{
    m_pAggregateMember = pManager;
}

inline void cARQManager::cComplexAggregateControl::OnFinalRelease()
{
    if (m_pAggregateMember) {
        m_pAggregateMember->OnFinalRelease();
    }
}

STDMETHODIMP cARQManager::cComplexAggregateControl::QueryInterface(REFIID riid, void **ppvObj)
{
    if (!IsEqualOrIUnknownGUID(riid, IID_IAggregateMemberControl)) {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    *ppvObj = static_cast<IAggregateMemberControl *>(this);
    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) cARQManager::cComplexAggregateControl::AddRef()
{
    return __m_ulRefs.AddRef();
}

EXTERN tResult LGAPI _AsyncReadQueueCreate(REFIID riid, IAsyncReadQueue **ppQueue, IUnknown *pOuter)
{
    if (!GetPrivateProfileInt("Threading", "Enabled", 1, ".\\lg.ini"))
        return 0;

#line 143
    cARQManager *pMgr = new cARQManager(pOuter);
    if (pMgr)
        return S_OK;

    return E_FAIL;
}

STDMETHODIMP cARQManager::cComplexAggregateControl::Connect()
{
    if (m_pAggregateMember) {
        return m_pAggregateMember->Connect();
    }
    return S_OK;
}

STDMETHODIMP cARQManager::cComplexAggregateControl::PostConnect()
{
    if (m_pAggregateMember) {
        return m_pAggregateMember->PostConnect();
    }
    return S_OK;
}

STDMETHODIMP cARQManager::cComplexAggregateControl::Init()
{
    if (m_pAggregateMember) {
        return m_pAggregateMember->Init();
    }
    return S_OK;
}

STDMETHODIMP cARQManager::cComplexAggregateControl::End()
{
    if (m_pAggregateMember) {
        return m_pAggregateMember->End();
    }
    return S_OK;
}

STDMETHODIMP cARQManager::cComplexAggregateControl::Disconnect()
{
    if (m_pAggregateMember) {
        return m_pAggregateMember->Disconnect();
    }
    return S_OK;
}

STDMETHODIMP_(ULONG) cARQManager::cComplexAggregateControl::Release()
{
    if (__m_ulRefs.Release() != 0) {
        return __m_ulRefs;
    }
    OnFinalRelease();
    return 0;
}

//
// cARQManager
//

cARQManager::cARQManager(IUnknown *pOuterUnknown)
    : m_RequestQueue(0), m_Fulfiller(m_RequestQueue), m_iSuspendCount(0)
{
    sRelativeConstraint constraints[] = {
        { kConstrainAfter, &IID_IGameShell },
        { kNullConstraint, NULL }
    };
    __m_pOuterUnknown.Init(pOuterUnknown);
    __m_AggregateControl.InitControl(this);

    AddToAggregate1(pOuterUnknown, IID_IAsyncReadQueue, this, &__m_AggregateControl, 0x1000, constraints);
    __m_AggregateControl.Release();
}

STDMETHODIMP cARQManager::QueryInterface(REFIID riid, void **ppvObj)
{
    return ((IUnknown *)__m_pOuterUnknown)->QueryInterface(riid, ppvObj);
}

STDMETHODIMP_(ULONG) cARQManager::AddRef()
{
    return ((IUnknown *)__m_pOuterUnknown)->AddRef();
}

STDMETHODIMP_(ULONG) cARQManager::Release()
{
    return ((IUnknown *)__m_pOuterUnknown)->Release();
}

cARQManager::~cARQManager()
{
}

HRESULT cARQManager::Init()
{
    if (!m_Fulfiller.Create()) {
#line 163
        CriticalMsg("Failed to create ARQ fulfiller thread");
        return E_FAIL;
    }
    BalanceThreadPriority();
    return S_OK;
}

HRESULT cARQManager::End()
{
    if (m_Fulfiller.ThreadExists()) {
        m_Fulfiller.CallWorker(0, TRUE);
        m_Fulfiller.WaitForClose(10000);
    }
    return S_OK;
}

STDMETHODIMP cARQManager::QueueRequest(const sARQRequest *pRequest, IAsyncReadControl **ppControl)
{
    BalanceThreadPriority();
    cARQRequest *pReq = new cARQRequest(*pRequest);
    m_RequestQueue.AddRequest(pReq, cARQRequestQueue::kAtPriority);
    if (ppControl) {
        *ppControl = pReq;
    } else {
        pReq->Release();
    }
    return 0;
}

STDMETHODIMP cARQManager::QueueStreamRequest(const sARQStreamRequest *pRequest, IAsyncReadControl **ppControl)
{
    cAsyncStreamReader *pReader = new cAsyncStreamReader(*pRequest);
    sARQRequest arqReq;
    arqReq.pFulfiller = pReader;
    arqReq.queue = pRequest->queue;
    arqReq.priority = pRequest->priority;
    arqReq.dwData[0] = 0;
    arqReq.dwData[1] = 0;
    arqReq.dwData[2] = 0;
    arqReq.dwData[3] = 0;
    arqReq.dwData[4] = 0;
    arqReq.dwData[5] = 0;
    arqReq.pszTrace = pRequest->pszTrace;
    HRESULT hr = QueueRequest(&arqReq, ppControl);
    pReader->Release();
    return hr;
}

STDMETHODIMP cARQManager::GetAsyncStreamer(const sARQStreamerDesc *pDesc, IAsyncStreamer **ppAsyncStreamer)
{
#line 239
    CriticalMsg("Unimplemented function call");
    return E_NOTIMPL;
}

STDMETHODIMP cARQManager::Suspend()
{
    HRESULT hr = 0;
    DWORD dwCount;
    if (m_iSuspendCount == 0) {
        dwCount = m_Fulfiller.Suspend();
        m_Fulfiller.Resume();
#ifndef SHIP
        if (dwCount != 0) {
#line 258
            CriticalMsg("ARQ worker thread suspended preciously but not through approved API");
        }
        else
#else
        if (dwCount == 0)
#endif
        {
            hr = m_Fulfiller.CallWorker(1, TRUE);
            while (m_Fulfiller.Suspend() == 0) {
                m_Fulfiller.Resume();
                Sleep(0);
            }
            m_Fulfiller.Resume();
        }
    }
    ++m_iSuspendCount;
    return hr;
}

STDMETHODIMP cARQManager::Resume()
{





    AssertMsg(m_iSuspendCount >= 1, "Invalid call to resume: not suspended!");
    m_iSuspendCount--;
    if (m_iSuspendCount == 0) {
        if (m_Fulfiller.Resume() != (DWORD)-1) {
            return 0;
        }
    }
#line 289
    CriticalMsg("ARQ thread resume failed!");
    return E_FAIL;
}

STDMETHODIMP_(BOOL) cARQManager::IsStreamerBlocking()
{





    CriticalMsg("Unimplemented function call");
    return FALSE;
}

HRESULT cARQManager::Connect()
{
    return S_OK;
}

HRESULT cARQManager::PostConnect()
{
    return S_OK;
}

STDMETHODIMP cARQManager::FulfillAll()
{
    return FulfillToPriority(0x7fffffff);
}

STDMETHODIMP cARQManager::FulfillToPriority(int priority)
{
    BalanceThreadPriority();
    Suspend();

    cARQRequest *pReq = 0;
    for (;;) {
        if (m_RequestQueue.GetCount() == 0)
            break;
        pReq = m_RequestQueue.GetRequest();
        if (!pReq)
            break;
        if (pReq->GetPriority() > priority)
            break;
        pReq->Fulfill();
        if (pReq)
            pReq->Release();
        pReq = 0;
    }
    if (pReq) {
        m_RequestQueue.AddRequest(pReq, cARQRequestQueue::kAtFront);
        if (pReq)
            pReq->Release();
        pReq = 0;
    }

    Resume();
    return E_FAIL;
}

HRESULT cARQManager::Disconnect()
{
    return S_OK;
}

inline void cARQManager::OnFinalRelease()
{
    delete this;
}

STDMETHODIMP cARQManager::KillAll()
{
    BalanceThreadPriority();
    Suspend();

    cARQRequest *pReq = 0;
    for (;;) {
        if (m_RequestQueue.GetCount() == 0)
            break;
        pReq = m_RequestQueue.GetRequest();
        if (!pReq)
            break;
        pReq->Kill(TRUE);
        if (pReq)
            pReq->Release();
        pReq = 0;
    }

    Resume();
    return E_FAIL;
}

void cARQManager::BalanceThreadPriority()
{
    int nPriority = GetThreadPriority(GetCurrentThread());
    if (m_Fulfiller.GetPriority() != nPriority) {
        m_Fulfiller.SetPriority(nPriority);
        m_Fulfiller.SetPrioirities(nPriority, nPriority);
    }
}


//
// cAsyncStreamReader
//

