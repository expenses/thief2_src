///////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/ai/RCS/aiwr.h 1.3 1999/03/10 12:59:15 ccarollo Exp $
//
//

#ifndef __AIWR_H
#define __AIWR_H

#include <objtype.h>
#include <wrtype.h>

float AIGetObjectLighting(ObjID id);

BOOL AIIsInWorld(ObjID id);

enum eAIRaycastFlags
{
   kAIR_ZeroEpsilon = 0x01,
   kAIR_NoHintWarn  = 0x02,
   kAIR_VsObjects   = 0x04,
};

BOOL AIRaycast(const Location *start_loc, Location *end_loc,
              Location *hit_loc, unsigned flags = 0);


#endif /* !__AIWR_H */
