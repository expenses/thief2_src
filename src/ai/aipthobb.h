///////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/ai/RCS/aipthobb.h 1.3 1999/03/10 12:58:06 ccarollo Exp $
//
// AI OBB pathfinding header
//

#ifndef __AIPTHOBB_H
#define __AIPTHOBB_H

#include <comtools.h>
#include <dynarray.h>
#include <aipathdb.h>

F_DECLARE_INTERFACE(IAIManager);
F_DECLARE_INTERFACE(IBoolProperty);

struct sAIObjCell : public sAIExternCell
{
   sAIExternCell *pAIExternCell;
   ObjID  objID;
   void  *data;
};

////////////////////////////////////////

EXTERN IBoolProperty *g_pObjPathableProperty;

////////////////////////////////////////

EXTERN void AIInitObjectPathable(IAIManager *);
EXTERN void AITermObjectPathable();

EXTERN void AIGetAllMovingTerrainCells(cDynArray<sAIObjCell> *pCellList);
EXTERN void AIGetAllPathableOBBCells(cDynArray<sAIObjCell> *pCellList);

EXTERN void AIGetAllMovingTerrainObjs(cDynArray<ObjID> *pObjList);

////////////////////////////////////////

#endif // __AIPTHOBB_H
