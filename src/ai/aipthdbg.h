///////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/ai/RCS/aipthdbg.h 1.9 1998/11/03 16:24:47 TOML Exp $
//
//
//

#ifndef __AIPTHDBG_H
#define __AIPTHDBG_H

#include <dynarray.h>

#pragma once

#ifndef SHIP

F_DECLARE_INTERFACE(IAIPath);

// and these are for debug stuff only...
extern tAIPathCellID gDrawOneCell;
extern tAIPathCellID AIDebugMagicLocatorCell;
extern BOOL AIDebugCenters;

//void AIPathFindTest();
extern void AIPathFindDrawPath(IAIPath * pPath);

// draw pathfinding database, both cells and links, depending on
// status of flags below
extern "C" void AIPathFindDrawDB();
extern void AIPathDrawCellsToggle();
extern void AIPathDrawLinksToggle();

extern void AIPathFindSpewZone();
extern void AIPathFindSpewZones();

extern int g_AIPathDBDrawZone;
extern int g_AIPathDBDrawRoom;

void ValidateAllCellSpaces(float height, BOOL linkedOnly, cDynArray<tAIPathCellID> * pFailures);
void ValidateAllCellRooms(BOOL linkedOnly, cDynArray<tAIPathCellID> * pFailures);

#endif

#endif /* !__AIPTHDBG_H */
