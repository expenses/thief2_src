#pragma once

#include <thrdtool.h>

class cARQRequestQueue;

class cARQFulfillerThread : public cWorkerThread
{
public:
    cARQFulfillerThread(cARQRequestQueue &queue);

    unsigned long ThreadProc();

    void SetPrioirities(int light, int normal)
    {
        m_LightPriority = light;
        m_NormalPriority = normal;
    }

    cARQRequestQueue *m_RequestQueue;
    int m_LightPriority;
    int m_NormalPriority;
};
