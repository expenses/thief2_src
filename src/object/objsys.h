// $Header: x:/prj/tech/libsrc/object/RCS/objsys.h 1.6 1999/01/27 16:00:45 mahk Exp $

#ifndef OBJSYS_H
#define OBJSYS_H

#include <appagg.h>
#include <iobjsys.h>
#include <objtype.h>
#include <osystype.h>

////////////////////////////////////////////////////////////
// OBJECT SYSTEM INTERFACE
// 

//
// This is now a wrapper interface around IObjectSystem.
// Newer code should use IObjectSystem
//


#define ObjSysCommitDeletions()  ObjSysEndOfFrame()

EXTERN ObjID BeginObjectCreate(ObjID archetype, eObjConcreteness concrete);
EXTERN void EndObjectCreate(ObjID obj);
EXTERN BOOL ObjectExists(ObjID obj);

#define DestroyObject(obj) \
do { IObjectSystem* sys = AppGetObj(IObjectSystem); IObjectSystem_Destroy(sys,obj); SafeRelease(sys); } while(0)

#endif // OBJSYS_H









