////////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/physics/RCS/phmoapi.h 1.12 1999/02/11 13:46:14 JON Exp $
//
// Physics motion api
//

#ifndef __PHMOAPI_H
#define __PHMOAPI_H

#include <matrixs.h>

enum ePlayerMotion
{
   kMoInvalid = -1,

   kMoNormal,
   kMoStrideLeft,
   kMoStrideRight,

   kMoCrouch,
   kMoCrawlLeft,
   kMoCrawlRight,

   kMoWithBody,
   kMoWithBodyLeft,
   kMoWithBodyRight,

   kMoJumpLand,
   kMoWeaponSwing,
   kMoWeaponSwingCrouch,

   kMoLeanLeft,
   kMoLeanRight,
   kMoLeanForward,

   kMoCrouchLeanLeft,
   kMoCrouchLeanRight,
   kMoCrouchLeanForward,

   kMoDisable = 1000,
   kMoEnable,
   kBigNum = 0x10000,
};

typedef enum ePlayerMotion ePlayerMotion;

////////////////////////////////////////////////////////////////////////////////

EXTERN void PhysAdjustPlayerHead(mxs_vector *pos, mxs_angvec *ang);

EXTERN void InitPlayerMotion();
EXTERN void TermPlayerMotion();
EXTERN void ResetPlayerMotion();

EXTERN void          PlayerMotionActivate(ePlayerMotion motion);
EXTERN ePlayerMotion PlayerMotionGetActive();

EXTERN void          PlayerMotionActivateList(ePlayerMotion *motions, int size);

EXTERN void          PlayerMotionSetRest(ePlayerMotion restMotion);
EXTERN ePlayerMotion PlayerMotionGetRest();

EXTERN void PlayerMotionSetOffset(short submod, mxs_vector *offset);         
EXTERN void PlayerMotionGetOffset(short submod, mxs_vector *offset);

EXTERN void PlayerMotionUpdate(mxs_real dt);

////////////////////////////////////////////////////////////////////////////////

#endif // __PHMOAPI_H
