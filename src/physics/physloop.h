// $Header: r:/prj/cam/src/physics/RCS/physloop.h 1.4 1998/08/20 13:56:55 mahk Exp $
#ifndef __PHYSLOOP_H
#define __PHYSLOOP_H


// Loop client GUID
DEFINE_LG_GUID(LOOPID_Physics, 0x3b);
DEFINE_LG_GUID(LOOPID_PhysicsBase, 0x1ce);

//
// Loop Client Factory Descriptor
EXTERN struct sLoopClientDesc PhysicsLoopClientDesc;
EXTERN struct sLoopClientDesc PhysicsBaseClientDesc;

//
// KEEP THIS FILE RELATIVELY CLEAR OF JUNK!  DO NOT PUT YOUR SYSTEM'S API HERE! 
// DO NOT INCLUDE OTHER HEADER FILES FROM THIS FILE!
//

 
#endif // __PHYSLOOP_H
