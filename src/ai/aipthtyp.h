///////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/ai/RCS/aipthtyp.h 1.3 1998/09/05 11:01:12 CCAROLLO Exp $
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

typedef ushort   tAIPathCellIDPacked;
typedef ushort   tAIPathVertexIDPacked;
typedef ushort   tAIPathCell2CellLinkIDPacked;
typedef ushort   tAIPathCell2VertexLinkIDPacked;

typedef ushort   tAIPathZone;

typedef uchar    tAIPathOkBits;

///////////////////////////////////////////////////////////////////////////////

#endif /* !__AIPTHTYP_H */
