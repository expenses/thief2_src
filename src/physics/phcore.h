///////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/physics/RCS/phcore.h 1.22 1999/02/09 09:37:48 JON Exp $
//
// Core physics module
//

#ifndef __PHCORE_H
#define __PHCORE_H

#include <matrixs.h>

#include <phystyp2.h>
#include <phsubmod.h>
#include <phdyn.h>
#include <phclsn.h>
#include <phlistyp.h>

class  cPhysModels;
class  cPhysClsns;
struct IPhysSphereProperty;
struct IPhysOBBProperty;
struct IPhysModelProperty;
struct IPhysModelProperty;
class  cPhysModel;

class cSubscribeService;
class cPhysListeners;

///////////////////////////////////////////////////////////////////////////////
//
// Globals
//

EXTERN cPhysModels & g_PhysModels;
EXTERN cPhysClsns &  g_PhysClsns;

EXTERN int g_PhysVersion;
EXTERN const int  g_PhysCurrentVersion;

EXTERN cSubscribeService *pPhysSubscribeService;
EXTERN cPhysListeners* g_pPhysListeners;

///////////////////////////////////////////////////////////////////////////////

EXTERN void PhysEmergencyStop(cPhysModel * pModel, tPhysSubModId subModId, mxs_real dt);

extern ePhysClsnResult DetermineCollisionResult(cPhysClsn * pClsn);
extern void CheckModelObjectCollisions(cPhysModel * pModel, mxs_real t0, mxs_real dt, BOOL checkTest, tPhysSubModId subModId = -1);
extern void BounceSpheres(const sPhysSubModInst & instance1, cPhysDynData * pDynamics1,
                          const sPhysSubModInst & instance2, cPhysDynData * pDynamics2,
                          mxs_real time);

extern BOOL UpdateCreaturePhysics(ObjID objID, mxs_vector *new_pos);

extern void ConstrainFromTerrain(cPhysModel * pModel, tPhysSubModId i);
extern BOOL ConstrainFromObjects(cPhysModel * pModel, tPhysSubModId i);

extern void WakeUpContacts(cPhysModel *pModel);

extern BOOL PressurePlateCallback(ObjID objID);

///////////////////////////////////////////////////////////////////////////////

#endif /* !__PHCORE_H */




