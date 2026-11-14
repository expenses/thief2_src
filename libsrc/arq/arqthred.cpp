#include <arqapi.h>
#include <arq.h>
#include <lgassert.h>

unsigned long cARQFulfillerThread::ThreadProc()
{
    HANDLE hEvents[2];

    const unsigned nLightThreshold = 5;

    int currentPriority = m_LightPriority;
    SetPriority(currentPriority);

    hEvents[0] = GetCallHandle();
    hEvents[1] = m_RequestQueue->GetAvailabilitySignalHandle();

    int fStop = 0;
    while (!fStop) {
        DWORD dwWait = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
        if (dwWait == WAIT_FAILED)
            break;

        switch (dwWait) {
        case WAIT_OBJECT_0:
            switch (GetCallParam()) {
            case 0:
                Reply(0);
                fStop = 1;
                break;
            case 1:
                Reply(0);
                Suspend();
                break;
            default:
#line 66
                CriticalMsg("Unknown call to ARQ fulfiller");
                Reply(E_FAIL);
                break;
            }
            break;
        case WAIT_OBJECT_0 + 1:
        {
            m_RequestQueue->m_Lock.Lock();
            int nPriority;
            if (m_RequestQueue->GetCount() >= nLightThreshold)
                nPriority = m_NormalPriority;
            else
                nPriority = m_LightPriority;
            if (nPriority != currentPriority) {
                currentPriority = nPriority;
                SetPriority(nPriority);
            }
            cARQRequest *pReq = m_RequestQueue->GetRequest();
            m_RequestQueue->m_Lock.Unlock();
            if (pReq) {
                pReq->Fulfill();
                pReq->Release();
            }
            break;
        }
        default:
#line 113
            CriticalMsg("There was nothing to do!");
            break;
        }
    }
    return 0;
}