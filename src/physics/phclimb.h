///////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/physics/RCS/phclimb.h 1.4 1998/07/21 11:33:32 CCAROLLO Exp $
//
// Physics Climbing API
//

#ifndef __PHCLIMB_H
#define __PHCLIMB_H

#include <matrixs.h>
#include <objtype.h>

#ifdef __cplusplus
class cPhysModel;
#else
typedef void cPhysModel;
#endif

EXTERN BOOL PhysObjIsClimbing(ObjID objID);
EXTERN BOOL PhysObjIsMantling(ObjID objID);

EXTERN void BreakClimb(ObjID objID, BOOL jumping, BOOL jump_thru);
EXTERN BOOL CheckClimb(ObjID objID);

EXTERN void BreakMantle(ObjID objID);
EXTERN BOOL CheckMantle(ObjID objID);

EXTERN void UpdateMantling(cPhysModel *pModel, mxs_real dt);

EXTERN void GetClimbingDir(ObjID objID, mxs_vector *dir);


#endif
