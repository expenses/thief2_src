/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#ifdef EDITOR
#line 1 "r:\\prj\\thief2\\skup\\thief2\\src\\dark\\daicam.cpp"
#else
#line 1 "s:\\prj\\thief2\\src\\dark\\daicam.cpp"
#endif
#line 1
///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/dark/daicam.cpp,v 1.1 1998/08/31 12:17:40 TOML Exp $
//
// Camera Behavior Set
//

#include <lg.h>

#include <aicamera.h>
#include <aidbgcmp.h>
#include <aisound.h>
#include <aisnsdev.h>

#include <daicam.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIDarkCameraBehaviorSet
//

STDMETHODIMP_(const char *) cAIDarkCameraBehaviorSet::GetName()
{
   return "DarkCamera";
}

////////////////////////////////////

void cAIDarkCameraBehaviorSet::CreateNonAbilityComponents(cAIComponentPtrs * pComponents)
{
   // Debugging/development tools
   pComponents->Append(new cAIFlowDebugger);

   // Enactors
   pComponents->Append(new cAISoundEnactor);

   // Senses
   pComponents->Append(new cAIDeviceSenses);

}

///////////////////////////////////////

void cAIDarkCameraBehaviorSet::CreateNonCombatAbilities(cAIComponentPtrs * pComponents)
{
}

///////////////////////////////////////

void cAIDarkCameraBehaviorSet::CreateCombatAbilities(cAIComponentPtrs * pComponents)
{
   pComponents->Append(new cAICamera);
}

///////////////////////////////////////////////////////////////////////////////
