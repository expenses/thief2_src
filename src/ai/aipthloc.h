///////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/ai/RCS/aipthloc.h 1.4 1998/11/06 22:53:26 TOML Exp $
//
// AI Path cell locators/spatial queries
//

#ifndef __AIPTHLOC_H
#define __AIPTHLOC_H

#include <aitype.h>
#include <aipthtyp.h>

struct sAIPathCell;

///////////////////////////////////////////////////////////////////////////////

tAIPathCellID AILocateBestLocoCell(const mxs_vector & fromLoc, 
                                   const tAIPathCellID fromCell, 
                                   const mxs_vector & to);


///////////////////////////////////////

enum eAIFindCellFlags
{
   kAIFC_NoExhaustive = 0x01,
   kAIFC_NoRooms      = 0x02
};

tAIPathCellID __fastcall AIFindClosestCell(const mxs_vector & location,
                                           tAIPathCellID    hintCell,
                                           ObjID objID = -1, 
                                           unsigned flags = 0);

///////////////////////////////////////

BOOL AIFindClosestConnectedCell(tAIPathCellID      from, 
                                const mxs_vector & fromLoc,
                                const mxs_vector & toLoc,
                                tAIPathCellID *    pResultCell,
                                mxs_vector *       pClosestLoc);

///////////////////////////////////////

const mxs_vector & AIGetCellCenter(const tAIPathCellID cell, mxs_vector * pCenter);
const mxs_vector & AIGetCellCenter(const tAIPathCellID cell);

///////////////////////////////////////

BOOL IsPtInCellXYPlane(const mxs_vector &p, const sAIPathCell * pCell);

///////////////////////////////////////

float AIGetZAtXYOnCell(const mxs_vector & pt, tAIPathCellID cellID);

///////////////////////////////////////

unsigned AIGetCellZone(tAIPathCellID cell);

///////////////////////////////////////////////////////////////////////////////

#endif /* !__AIPTHLOC_H */
