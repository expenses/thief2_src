///////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/ai/RCS/aicamera.h 1.3 1999/05/29 21:55:08 JON Exp $
//
// AI Camera Ability
//
// A camera is a device that scans an arc and broadcasts link messages down all
// attached AICamera links when it is alerted. The data on the link is used to
// derive a message & multi-parm data.

#ifndef __AICAMERA_H
#define __AICAMERA_H

#include <aidev.h>
#include <aiactjsc.h>

// I have to include all this just to get to the definition of the linkkind class
#include <comtools.h>
#include <scrptsrv.h>
#include <linkscpt.h>

#pragma once
#pragma pack(4)

class cAICamera : public cAIDevice
{
public:
   // Create, specify which links to alert & what message to broadcast
   cAICamera(void);
   ~cAICamera();

   // Standard component methods
   STDMETHOD_(const char *, GetName)();

   // Goals
   STDMETHOD (SuggestGoal)(cAIGoal * pPrevious, cAIGoal ** ppNew);

   // Ability flow
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);

   // Notification
   STDMETHOD_(void, OnAlertness)(ObjID source,
                                 eAIAwareLevel previous, 
                                 eAIAwareLevel current,
                                 const sAIAlertness * pRaw);

private:
   void BroadcastSwitches(void);
};

// link data
struct sAICameraLinkData
{
   char m_msg[32];      // script message
   char m_data[3][32]; // multi-parm args
};

// Init/term
BOOL AIInitCameraAbility(IAIManager *);
BOOL AITermCameraAbility(void);

#pragma pack()

#endif /* !__AICAMERA_H */
