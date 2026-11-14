#pragma once

#include <lg.h>
#include <arqapi.h>
#include <arqqueue.h>
#include <arqthred.h>
#include <aggmemb.h>

class cARQRequestQueue;

class cARQRequest : public IAsyncReadControl, public cDListNode<cARQRequest, 1>
{
public:
    struct cRefCount
    {
        unsigned int ul;
        cRefCount();
        unsigned long AddRef();
        unsigned long Release();
        operator unsigned long();
    };

    cARQRequest(const sARQRequest &reqInfo);

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);
    STDMETHOD_(BOOL, IsFulfilled)(THIS);
    STDMETHOD(Fulfill)(THIS);
    STDMETHOD(Kill)(THIS_ BOOL fDiscard);
    STDMETHOD(GetResult)(THIS_ sARQResult *pResult);
    STDMETHOD_(BOOL, IsFinished)(THIS);

    virtual ~cARQRequest();

    int GetPriority() const { return m_ReqInfo.priority; }
    int GetQueueID() const;
    void SetOwningQueue(cARQRequestQueue *pOwningQueue);

    void OnFinalRelease();

    enum eState
    {
        kUnserviced = 0,
        kQueued = 1,
        kInProgress = 2,
        kComplete = 3,
        kKilled = 4
    };

    cRefCount __m_ulRefs;
    sARQRequest m_ReqInfo;
    sARQResult m_Result;
    cThreadLock m_Lock;
    volatile int m_State;
    cARQRequestQueue *m_pOwningQueue;
};

class cARQManager : public IAsyncReadQueue
{
public:
    struct cOuterPointer
    {
        cOuterPointer();
        void Init(IUnknown *pOuter);
        operator IUnknown *();

        IUnknown *m_pOuterUnknown;
    };

    struct cComplexAggregateControl : IAggregateMemberControl
    {
        struct cRefCount
        {
            cRefCount();
            unsigned long AddRef();
            unsigned long Release();
            operator unsigned long();

            unsigned int ul;
        };

        cComplexAggregateControl();
        void InitControl(cARQManager *pManager);
        void OnFinalRelease();

        // IAggregateMemberControl
        STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();
        STDMETHOD(Connect)();
        STDMETHOD(PostConnect)();
        STDMETHOD(Init)();
        STDMETHOD(End)();
        STDMETHOD(Disconnect)();

        cRefCount __m_ulRefs;
        cARQManager *m_pAggregateMember;
    };

public:
    cARQManager(IUnknown *pOuterUnknown);
    virtual ~cARQManager();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IAsyncReadQueue
    STDMETHOD(QueueRequest)(const sARQRequest *pRequest, IAsyncReadControl **ppControl);
    STDMETHOD(QueueStreamRequest)(const sARQStreamRequest *pRequest, IAsyncReadControl **ppControl);
    STDMETHOD(GetAsyncStreamer)(const sARQStreamerDesc *pDesc, IAsyncStreamer **ppAsyncStreamer);
    STDMETHOD(Suspend)();
    STDMETHOD(Resume)();
    STDMETHOD_(BOOL, IsStreamerBlocking)();
    STDMETHOD(FulfillAll)();
    STDMETHOD(FulfillToPriority)(int priority);
    STDMETHOD(KillAll)();

    void BalanceThreadPriority();

    HRESULT Connect();
    HRESULT PostConnect();
    HRESULT Init();
    HRESULT End();
    HRESULT Disconnect();
    void OnFinalRelease();

private:
    cOuterPointer __m_pOuterUnknown;
    cComplexAggregateControl __m_AggregateControl;
    cARQRequestQueue m_RequestQueue;
    cARQFulfillerThread m_Fulfiller;
    int m_iSuspendCount;
};
