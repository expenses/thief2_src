///////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/shock/RCS/shkaidth.h 1.1 1999/06/04 18:55:33 JON Exp $
//
//

#ifndef __SHKAIDTH_H
#define __SHKAIDTH_H

#include <aibasabl.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIQuickDeath
//

class cAIQuickDeath : public cAIAbility
{
public:   
   // Standard component methods
   STDMETHOD_(const char *, GetName)();
   STDMETHOD_(void, Init)();

   // Notifications
   STDMETHOD_(void, OnDeath)(const sDamageMsg * pMsg);
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__SHKAIDTH_H */
