// $Header: r:/prj/cam/src/dark/RCS/drkpwups.h 1.4 1998/08/08 23:33:37 dc Exp $
// Dark Specific Powerup Code and Script Services

#pragma once
#ifndef __DRKPWUPS_H
#define __DRKPWUPS_H

#include <objtype.h>

/////////////////
// power up specific calls/utilities

// check if this object is in fungus carpet at the moment
EXTERN BOOL ObjInFungus(ObjID obj);

// trigger a "flash" effect in world at location of obj
EXTERN void DrkTriggerWorldFlash(ObjID obj);

// to try and deploy a new object
EXTERN BOOL DrkObjTryDeploy(ObjID src_obj, ObjID deploy_arch);

// hack for now to try to clean up nearby blood
EXTERN void TryCleanBlood(ObjID water_src, float rad);

EXTERN void FungusizeSound(struct sESndEvent* ev); 

//////////////
// initializers

EXTERN void DrkPowerupInit(void);
EXTERN void DrkPowerupTerm(void);

#define LINK_RENDER_FLASH_NAME  "RenderFlash"

#endif  // __DRKPWUPS_H




