// $Header: r:/prj/cam/src/physics/RCS/phystest.h 1.8 1998/08/19 09:57:59 CCAROLLO Exp $

#ifndef _PHYSTEST_H
#define _PHYSTEST_H

#ifndef SHIP

EXTERN void PhysicsLaunchSphere(mxs_real speed);
EXTERN void PhysicsLaunchOBB(mxs_real speed);
EXTERN void PhysicsSetControl(mxs_real speed);
EXTERN void PhysicsStopControl();

EXTERN void PhysicsJoyride();
EXTERN void PhysicsTestRaycast(ObjID objID);


#ifdef DBG_ON
EXTERN void PhysicsDebugObj(ObjID objID);
EXTERN void PhysicsDebug(int what);
#endif

#endif // SHIP

#endif
