///////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/ai/RCS/aicbridl.h 1.3 1999/04/16 17:02:57 JON Exp $
//
//
//

#ifndef __AICBRIDL_H
#define __AICBRIDL_H

#include <aicbrnew.h>
#include <aicbrmod.h>

#pragma once
#pragma pack(4)

////////////////////////////////////////////////////////////////////////////////
//
// Notes: Idle must guarantee to always be applicable and pre-conditions TRUE
//

class cAIRangedIdle: public cAIRangedMode
{
public:
   cAIRangedIdle(cAINewRangedSubcombat* pOwner);

   virtual eAIRangedModeID GetModeID(void) const;
   virtual const char* GetName(void) const;

   virtual int SuggestApplicability(void);
   virtual int GetPriority(void);
   virtual cAIAction* SuggestAction(void);

private:
};

/////////////////////////////////////////////////////////

inline cAIRangedIdle::cAIRangedIdle(cAINewRangedSubcombat* pOwner): cAIRangedMode(pOwner) {}

inline eAIRangedModeID cAIRangedIdle::GetModeID(void) const {return kAIRC_IdleMode;}
inline const char* cAIRangedIdle::GetName(void) const {return "Idle";}

inline int cAIRangedIdle::SuggestApplicability(void) {return 0;}
inline int cAIRangedIdle::GetPriority(void) {return kAIRC_PriMinimum;}

/////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AICBTRNG_H */


