// $Header: r:/prj/cam/src/dark/RCS/drkwswd.h 1.8 1998/11/02 03:18:45 dc Exp $
// Player-specific weapon stuff

#ifndef __DRKWSWD_H
#define __DRKWSWD_H

EXTERN BOOL EquipWeapon(ObjID owner, ObjID weapon, int type);
EXTERN BOOL UnEquipWeapon(ObjID owner, ObjID weapon);
EXTERN BOOL IsWeaponEquipped(ObjID owner, ObjID weapon);
// is this weapon for obj a sword? - only works for player at the moment
EXTERN BOOL WeaponIsSword(ObjID owner, ObjID weapon);
EXTERN BOOL WeaponIsBlackjack(ObjID owner, ObjID weapon); // i suck

EXTERN void StartWeaponBlock(ObjID owner, ObjID weapon);
EXTERN void StartWeaponAttack(ObjID owner, ObjID weapon);
EXTERN void FinishWeaponAction(ObjID owner, ObjID weapon);

EXTERN void UpdateWeaponAttack();

EXTERN void SetBlockMotionType(int type);

// player sword action types
enum {
   kPlayerSwordActionBlock,
   kPlayerSwordActionBlockDirected,
   kPlayerSwordActionSwingShort,
   kPlayerSwordActionSwingMediumLeft,
   kPlayerSwordActionSwingMediumRight,
   kPlayerSwordActionSwingLongLeft,
   kPlayerSwordActionSwingLongRight,
   kPlayerSwordNumActions,
};

#endif

