// $Header: r:/prj/cam/src/dark/RCS/picklock.h 1.4 1998/11/01 05:21:29 dc Exp $
// lockpick service, properties, the whole thing...

#pragma once
#ifndef __PICKLOCK_H
#define __PICKLOCK_H

#include <objtype.h>
#include <property.h>
#include <pickbase.h>

#undef INTERFACE
#define INTERFACE IPickCfgProperty
DECLARE_PROPERTY_INTERFACE(IPickCfgProperty)
{
   DECLARE_UNKNOWN_PURE(); 
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(sPickCfg*); 
}; 

#undef INTERFACE
#define INTERFACE IPickStateProperty
DECLARE_PROPERTY_INTERFACE(IPickStateProperty)
{
   DECLARE_UNKNOWN_PURE(); 
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(sPickState*); 
}; 

#define PROP_PICKSRC_NAME   "PickSrc"
#define PROP_PICKCFG_NAME   "PickCfg"
#define PROP_PICKSTATE_NAME "PickState"

// do player hand motion setup/other master control for pick readiness
EXTERN BOOL PickLockReady(ObjID frobber, ObjID pick_obj);
EXTERN BOOL PickLockUnReady(ObjID frobber, ObjID pick_obj);

// fancy, doug-style, hold down, all inclusive actually do some picking, eh?
EXTERN BOOL PickLockStartPicking(ObjID frobber, ObjID pick_obj, ObjID locked_obj);
EXTERN BOOL PickLockFinishPicking(ObjID pick_obj);

// for stats, really
EXTERN int PickLockCountPickableLockedDoors(void);

// for doing a rollyourown incrementally sort of picking
// does this pick open stage stage of the lock?
EXTERN ePickResult PickLockMatchLevel(ObjID pick, ObjID lock, int stage);
// just start or stop the motion, if you are rolling your own
EXTERN BOOL PickLockMotionOnly(BOOL start);

// initialize/close down the system
EXTERN BOOL PickLockInit(void);
EXTERN BOOL PickLockTerm(void);

// since we dont use player arm anymore
EXTERN void PickLockPerFrameCallback(ulong dt);

#endif  // __PICKLOCK_H
