// $Header: r:/prj/cam/src/dark/RCS/drkscrs.h 1.5 1999/10/13 14:46:50 BFarquha Exp $
#pragma once
#ifndef __DRKSCRS_H
#define __DRKSCRS_H

#include <scrptbas.h>
#include <uigame.h>

////////////////////////////////////////////////////////////
// DARK GAME SCRIPT SERVICE
//




DECLARE_SCRIPT_SERVICE(DarkGame,0x1b4)
{

   //
   // Kill the player
   //
   STDMETHOD(KillPlayer)() PURE;

   //
   // End the mission
   //
   STDMETHOD(EndMission)() PURE;

   //
   // Fade out the screen
   STDMETHOD(FadeToBlack)(float time) PURE;

   // For now, this just notes the object as being found, so it's "secret" bit is reset.
   STDMETHOD(FoundObject)(ObjID obj) PURE;
};


#endif // __DRKSCRS_H
