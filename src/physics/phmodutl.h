///////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/physics/RCS/phmodutl.h 1.3 1997/08/18 17:43:44 TOML Exp $
//
// Model utility functions
//

#ifndef __PHMODUTL_H
#define __PHMODUTL_H

///////////////////////////////////////////////////////////////////////////////

EXTERN BOOL PhysHasActiveModel(ObjID objID, cPhysModel ** pModel);

///////////////////////////////////////

EXTERN mxs_real PhysGetObjsNorm(cPhysModel * pModel1, tPhysSubModId subModId1,
                                cPhysModel * pModel2, tPhysSubModId subModId2,
                                mxs_vector & normal);

///////////////////////////////////////////////////////////////////////////////

#endif /* !__PHMODUTL_H */
