///////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/physics/RCS/physcast.h 1.7 1999/06/18 16:39:56 JON Exp $
//
// Raycasting through physics objects
//

#ifndef __PHYSCAST_H
#define __PHYSCAST_H

struct mxs_vector;
struct Location;

enum ePhysCollisionTest
{
   kCollideNone        = 0x0000,
   kCollideSphere      = 0x0001,
   kCollideSphereHat   = 0x0002,
   kCollideOBB         = 0x0004,
   kCollideEdgeTrigger = 0x0008,

   kCollideTerrain     = 0x0100,

   kCollideInvalid     = 0x1000,

   kCollideAllPhys     = kCollideSphere | kCollideSphereHat | kCollideOBB | kCollideEdgeTrigger,
   kCollideAll         = kCollideAllPhys | kCollideTerrain,
};

class cPhysModel;

typedef BOOL (* tPhyscastObjTest)(ObjID obj, const cPhysModel* pPhysModel, void* pData);

// Returns one of ePhysCollisionTest, based on what was hit
// Note: this function does not deal properly with casts against terrain with radius>0
// It will do a regular raycast instead in this case (Warning has been removed though).
EXTERN int PhysRaycast(Location &start, Location &end, Location *hit, ObjID *hit_obj, 
                       mxs_real radius, int filter = kCollideAll);

EXTERN void PhysRaycastSetObjlist(ObjID *obj_list, int num_objects);
EXTERN void PhysRaycastClearObjlist();

EXTERN void PhysRaycastSetCallback(tPhyscastObjTest callback, void* pData);
EXTERN void PhysRaycastClearCallback(void);

#endif