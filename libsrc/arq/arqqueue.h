#pragma once

#include <thrdtool.h>
#include <dlist.h>
#include <arqapi.h>

class cARQRequest;

class cARQRequestQueue : public cDList<cARQRequest, 1>
{
public:
    enum eAddFlags
    {
        kAtPriority,
        kAtFront,
        kAtRear,
    };

    cARQRequestQueue(unsigned int queueID);
    ~cARQRequestQueue();

    unsigned int GetCount() { cAutoLock lock(m_Lock); return m_cEntries; }

    int AddRequest(cARQRequest *pReq, eAddFlags flags);
    int RemoveRequest(cARQRequest *pReq);
    cARQRequest *GetRequest();

    unsigned int GetMax() { return m_queueID; }

    HANDLE GetAvailabilitySignalHandle() { return m_AvailabilitySignal; }

    void WaitPut();
    void ReleasePut();

    cThreadLock m_Lock;
    cThreadEvent m_AvailabilitySignal;
    cThreadSemaphore m_PutSemaphore;
    unsigned int m_cEntries;
    unsigned int m_queueID;
    unsigned int m_flags;
};
