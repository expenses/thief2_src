///////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/shock/RCS/shkaitbs.h 1.3 1999/06/04 18:57:59 JON Exp $
//
//
//

#ifndef __SHKAITBS_H
#define __SHKAITBS_H

#include <aibasbhv.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAITurretBehaviorSet
//

class cAITurretBehaviorSet : public cAIBehaviorSet
{
public:
   //
   // Find out the behavior set name
   //
   STDMETHOD_(const char *, GetName)();

protected:
   virtual void CreateNonAbilityComponents(cAIComponentPtrs * pComponents);
   virtual void CreateNonCombatAbilities(cAIComponentPtrs * pComponents);
   virtual void CreateCombatAbilities(cAIComponentPtrs * pComponents);
   virtual void CreateGenericAbilities(cAIComponentPtrs * pComponents);
};

#pragma pack()
#endif /* !__SHKAITBS_H */






