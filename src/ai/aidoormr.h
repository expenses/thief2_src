///////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/ai/RCS/aidoormr.h 1.4 1998/10/30 12:27:16 TOML Exp $
//
//
//

#ifndef __AIDOORMR_H
#define __AIDOORMR_H

#include <aibasmrg.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitDoorMovReg(IAIManager *);
BOOL AITermDoorMovReg();

///////////////////////////////////////////////////////////////////////////////

class cAIDoorMovReg : public cAIMoveRegulator
{
public: 
   cAIDoorMovReg();
   
   STDMETHOD_(const char *, GetName)();
   STDMETHOD_(BOOL, AssistGoal)(const sAIMoveGoal & goal, sAIMoveGoal * pResultGoal);

   STDMETHOD_(BOOL, WantsInterrupt)();
private:
   cAITimer m_DoorLookPeriod;
   cAITimer m_DoorWait;
   BOOL     m_fWaiting;
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIDOORMR_H */
