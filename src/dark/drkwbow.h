////////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/dark/RCS/drkwbow.h 1.4 1998/10/22 00:11:28 dc Exp $
//
// Weapon (hand-to-hand-combat) related functions
//

#ifndef __DRKWBOW_H
#define __DRKWBOW_H

#include <objtype.h>

EXTERN BOOL EquipBow();
EXTERN BOOL UnEquipBow();
EXTERN BOOL IsBowEquipped();

EXTERN void StartBowAttack();
EXTERN BOOL FinishBowAttack();
EXTERN BOOL AbortBowAttack();
EXTERN void ForceFinishBowAttack(BOOL abort);
EXTERN void UpdateBowAttack(long dt);

EXTERN BOOL SetCurrentArrow(ObjID arrow);

EXTERN ObjID PlayerArrow();

EXTERN void SetBowZoomActive (BOOL active);

EXTERN void BowInit();
EXTERN void BowTerm(); 


#endif
