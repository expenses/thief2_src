///////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/shock/RCS/shkflash.h 1.2 1999/08/05 17:37:59 Justin Exp $
//
// gun flashes
//

#ifndef __SHKFLASH_H
#define __SHKFLASH_H

#include <matrixs.h>
#include <objtype.h>

// Create flash objects as specified by archetype flash links at the various VHots of the gun
// Each link identifies the flash object (destination) and the vhot number as data
EXTERN void CreateGunFlashes(ObjID archetypeID, ObjID gunID, 
                             mxs_ang facingOffset = 0,
                             BOOL isGhost = FALSE);

EXTERN void GunFlashInit(void);
EXTERN void GunFlashTerm(void);

#endif // __SHKFLASH_H
