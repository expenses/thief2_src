// $Header: r:/prj/cam/src/shock/RCS/shkvhot.h 1.3 1999/04/29 18:14:36 JON Exp $

#ifndef __SHKVHOT_H
#define __SHKVHOT_H

#include <matrixs.h>
#include <objtype.h>

// Get the location of a vhot, taking into account joint rotations
EXTERN void VHotGetRaw(mxs_vector *pLoc, ObjID objID, int vhotNum);
// Get the location of a vhot, taking into account joint rotations
EXTERN void VHotGetLoc(mxs_vector *pLoc, ObjID objID, int vhotNum);
// Get the raw vhot offset
EXTERN void VHotGetOffset(mxs_vector *pOffset, ObjID objID, int vhotNum);

#endif // __SHKVHOT_H