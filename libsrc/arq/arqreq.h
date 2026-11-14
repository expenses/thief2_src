#line 134
#pragma once

inline void cARQRequest::SetOwningQueue(cARQRequestQueue *pOwningQueue)
{

    AssertMsg(!pOwningQueue || m_State == kUnserviced, "Can only queue unserviced requests");

    m_pOwningQueue = pOwningQueue;
    m_State = kQueued;
}
