//////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/physics/RCS/phmterr.h 1.4 1999/07/19 17:45:21 JON Exp $
//
// Physics moving terrain header
//

#ifndef __PHMTERR_H
#define __PHMTERR_H

#include <objtype.h>

EXTERN void ResetMovingTerrain();
EXTERN void RefreshMovingTerrain();

EXTERN void PostLoadMovingTerrain();

EXTERN void UpdateMovingTerrainVelocity(ObjID objID, ObjID next_node, float speed);

EXTERN void ActivateMovingTerrain(ObjID objID);
EXTERN void DeactivateMovingTerrain(ObjID objID);

EXTERN void UpdateMovingTerrain(int ms);

EXTERN BOOL IsMovingTerrain(ObjID objID);

#endif // __PHMTERR_H
