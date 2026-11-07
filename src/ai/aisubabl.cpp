/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#ifdef EDITOR
#line 1 "r:\\prj\\thief2\\skup\\thief2\\src\\ai\\aisubabl.cpp"
#else
#line 1 "s:\\prj\\thief2\\src\\ai\\aisubabl.cpp"
#endif
#line 1
///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aisubabl.cpp,v 1.3 2000/02/19 12:45:44 toml Exp $
//
//
//

#include <aisubabl.h>
#include <aigoal.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

///////////////////////////////////////////////////////////////////////////////

BOOL AISubabilityIsOwnerLosingControl(const IAIAbility * pAbility, const cAIGoal * pPrevious, const cAIGoal * pGoal)
{
   return ((!pGoal || pGoal->pOwner != (IAIAbility *)pAbility) && (pPrevious && pPrevious->pOwner == (IAIAbility *)pAbility));
}

///////////////////////////////////////////////////////////////////////////////
