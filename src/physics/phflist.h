///////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/physics/RCS/phflist.h 1.8 1998/10/26 23:51:44 CCAROLLO Exp $
//
// Physics force-list related functions
//

#ifndef __PHFLIST_H
#define __PHFLIST_H

#include <objtype.h>

#include <matrix.h>
#include <matrixs.h>

#include <phystyp2.h>

///////////////////////////////////////////////////////////////////////////////

enum eForceListResult_
{
   kFLR_Success           = 0x0000,
   kFLR_TerrainCollision  = 0x0001,
   kFLR_ObjectCollision   = 0x0002,
   kFLR_MadeNonPhysical   = 0x0004,
};
typedef int eForceListResult;

///////////////////////////////////////////////////////////////////////////////
//
// API: Force list (C-callable)
//

EXTERN eForceListResult PhysApplyForceList(ObjID objID, tPhysSubModId subModId, sPhysForce *flist, int nforces, 
                           mxs_real dt, mxs_vector *start_loc, mxs_vector *end_loc, 
                           mxs_vector *force, mxs_real *ctime, BOOL terrain_check, BOOL object_check);


#endif






