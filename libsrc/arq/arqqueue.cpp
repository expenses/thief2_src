#include <lg.h>
#include <arq.h>
#include <arqqueue.h>
#include <thrdtool.h>

inline int cARQRequest::GetQueueID() const
{
    return m_ReqInfo.queue;
}

#include <arqreq.h>

inline void cARQRequestQueue::WaitPut()
{
    if (!(m_flags & 1))
        m_PutSemaphore.Wait(0xFFFFFFFF);
}

inline void cARQRequestQueue::ReleasePut()
{
    if (!(m_flags & 1))
        m_PutSemaphore.Release(1, NULL);
}

cARQRequestQueue::cARQRequestQueue(unsigned int queueID)
    : m_Lock(),
      m_AvailabilitySignal(TRUE),
      m_PutSemaphore(32, 32),
      m_cEntries(0),
      m_queueID(queueID),
      m_flags(0)
{
}

cARQRequestQueue::~cARQRequestQueue()
{
}

int cARQRequestQueue::AddRequest(cARQRequest *pReq, eAddFlags flags)
{
#line 56
    AssertMsg(pReq, "Invalid request add");
    AssertMsg(pReq->GetQueueID() == m_queueID, "Attempt to add request to wrong queue");
    WaitPut();
    m_Lock.Lock();
    pReq->AddRef();
    switch (flags)
    {
        case kAtPriority:
        {
            cARQRequest *pScan = GetLast();
            while (pScan)
            {
                if (pReq->GetPriority() >= pScan->GetPriority())
                    break;
                pScan = pScan->GetPrevious();
            }
            if (pScan)
            {
                InsertAfter(pScan, pReq);
                break;
            }
        }
        case kAtFront:
            Prepend(pReq);
            break;
        case kAtRear:
            Append(pReq);
            break;
        default:
#line 97
            CriticalMsg("Unknown flag");
            break;
    }
    pReq->SetOwningQueue(this);
    if (++m_cEntries == 1)
        m_AvailabilitySignal.Set();
    m_Lock.Unlock();
    return 0;
}

int cARQRequestQueue::RemoveRequest(cARQRequest *pReq)
{
#line 122
    AssertMsg(pReq, "Invalid request remove");
    AssertMsg(pReq->GetQueueID() == m_queueID, "Attempt to remove request from wrong queue");
    m_Lock.Lock();
    Remove(pReq);
    pReq->SetOwningQueue(NULL);
    pReq->Release();
    --m_cEntries;
    ReleasePut();
    if (!m_cEntries)
        m_AvailabilitySignal.Reset();
    m_Lock.Unlock();
    return 1;
}

cARQRequest *cARQRequestQueue::GetRequest()
{
    m_Lock.Lock();
    cARQRequest *pReturn = GetFirst();
    if (pReturn)
    {
        pReturn->AddRef();
        RemoveRequest(pReturn);
    }
    m_Lock.Unlock();
    return pReturn;
}
