///////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/shock/RCS/shkpick.h 1.1 1998/12/19 13:43:57 JON Exp $
//

#ifndef __SHKPICK_H
#define __SHKPICK_H

#include <objtype.h>

EXTERN void ShockPickWeighObject(ObjID obj, float pickDistSquared, ObjID* pPickObj, float* pBestPickWeight);

#endif // __SHKPICK_H