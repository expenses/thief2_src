// $Header: r:/prj/cam/src/dark/RCS/drkinv.h 1.4 1998/05/22 00:35:53 mahk Exp $
// dark specific inventory stuff

#ifndef __DRKINV_H
#define __DRKINV_H

#include <drkienum.h>

// initialize the drk inv/ui controller
EXTERN void drkInvInit(void);

// check head focus to see if we have focus on an obj in the world
EXTERN void drkCheckHeadFocus(void);

EXTERN BOOL head_focus_nonfrob;
EXTERN float head_focus_dist2_tol;

// control capabilities and speed associated with inv actions/objects
EXTERN void drkInvCapControl(eDrkInvCap cap, eDrkInvControl control);
EXTERN void drkInvSpeedControlHack(float speed_fac, float rot_fac);

#endif  // __DRKINV_H


