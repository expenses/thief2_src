///////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/ai/RCS/aialert.h 1.9 1998/10/28 11:51:44 TOML Exp $
//
// Alertness/awareness structures & enums
//

#ifndef __AIALERT_H
#define __AIALERT_H

#include <aitype.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// STRUCT: sAIAlertness
//

struct sAIAlertness
{
   sAIAlertness()
    : level(kAIAL_Lowest),
      peak(kAIAL_Lowest)
   {
   }
   
   // Current alertness
   eAIAlertLevel level;
   
   // Current alertness
   eAIAlertLevel peak;
};

///////////////////////////////////////////////////////////////////////////////

struct sAIAlertCap
{
   sAIAlertCap()
    : maxLevel(kAIAL_High),
      minLevel(kAIAL_Lowest),
      minRelax(kAIAL_Low)
   {
   }
   
   // Maximum alerness cap
   eAIAlertLevel maxLevel;

   // Minimum alerness cap
   eAIAlertLevel minLevel;
   
   // Minimum alerness cap
   eAIAlertLevel minRelax;
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIALERT_H */
