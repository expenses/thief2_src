///////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/ai/RCS/aiprutil.h 1.6 1998/08/16 11:43:15 CCAROLLO Exp $
//
// @TBD (toml 06-09-98): these should be moved into their corresponding
// ability files?
//

#ifndef __AIPRUTL_H
#define __AIPRUTL_H

#include <aiprops.h>

#pragma once
#pragma pack(4)

struct sAISoundType;
F_DECLARE_INTERFACE(IAISoundTypeProperty);

///////////////////////////////////////////////////////////////////////////////

void AIInitUtilProps();
void AITermUtilProps();

///////////////////////////////////////////////////////////////////////////////
//
// UTILITY PROPERTIES
//

////////////////////////////////////////
//
// Sound types
//

#define PROP_AI_SNDTYPE "AI_SndType"

EXTERN IAISoundTypeProperty * g_pAISoundTypeProperty;

#define AIGetSoundType(obj) AIGetProperty(g_pAISoundTypeProperty, (obj), (sAISoundType *)NULL)

///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: IAISoundTypeProperty
//

#undef INTERFACE
#define INTERFACE IAISoundTypeProperty
DECLARE_PROPERTY_INTERFACE(IAISoundTypeProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAISoundType *);
};

////////////////////////////////////////
//
// Knockout stimuli
//

#define PROP_STIM_KNOCKOUT "StimKO"

EXTERN IBoolProperty * g_pStimKnockoutProperty;

#define AIGetStimKnockout(obj) AIGetProperty(g_pStimKnockoutProperty, (obj), FALSE)

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIPRUTL_H */



