// $Header: r:/prj/cam/src/dark/RCS/invtype.h 1.2 1998/06/09 20:03:34 TOML Exp $
#pragma once  
#ifndef __INVTYPE_H
#define __INVTYPE_H

////////////////////////////////////////////////////////////
// PLAYER INVENTORY TYPES
//

typedef enum eWhichInvObj
{
   kCurrentWeapon,
   kCurrentItem,

   kNumItemSels
} eWhichInvObj;


typedef enum eCycleDirection
{
   kCycleForward  = +1,
   kCycleBackward = -1, 
} eCycleDirection;


typedef enum eInventoryType
{
   kInvTypeJunk = 0, // default
   kInvTypeItem,
   kInvTypeWeapon,
} eInventoryType; 

//
// Dark contain types
// That is, the list of possible sub-locations within a container
// 

typedef enum eDarkContainType
{
   // rendered types (negative numbers)
   kContainTypeHand = -2, 
   kContainTypeBelt = -1,
   kContainTypeRendMin = kContainTypeHand,
   kContainTypeRendMax = 0,

   // non-rendered
   kContainTypeNonRendMin = 0,
   kContainTypeGeneric = kContainTypeNonRendMin,  // generic contents
   kContainTypeInventory = kContainTypeGeneric,  // general inventory
   kContainTypeNonRendMax, 

   // total range
   kContainTypeMin = kContainTypeRendMin,
   kContainTypeMax = kContainTypeNonRendMax, 
} eDarkContainType; 

#endif // __INVTYPE_H
