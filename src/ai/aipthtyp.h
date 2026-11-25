/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aipthtyp.h,v 1.4 2000/03/01 13:02:57 bfarquha Exp $
//
//

#ifndef __AIPTHTYP_H
#define __AIPTHTYP_H

#pragma once

extern BOOL g_fAIPathFindInited;

///////////////////////////////////////////////////////////////////////////////

typedef unsigned tAIPathCellID;
typedef unsigned tAIPathVertexID;
typedef unsigned tAIPathCell2CellLinkID;
typedef unsigned tAIPathCell2VertexLinkID;

typedef unsigned tAIPathCellIDPacked;
typedef unsigned tAIPathVertexIDPacked;
typedef unsigned tAIPathCell2CellLinkIDPacked;
typedef unsigned tAIPathCell2VertexLinkIDPacked;

typedef ushort   tAIPathZone;

typedef ushort   tAIPathOkBits;

typedef enum
{
   kAIZone_Normal = 0,
   kAIZone_NormalLVL,
   kAIZone_HighStrike,
   kAIZone_HighStrikeLVL,
   kAIZone_Num
} eAIPathZoneType;

///////////////////////////////////////////////////////////////////////////////

#endif /* !__AIPTHTYP_H */
