///////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/ai/RCS/aitrginf.h 1.3 1999/05/10 10:45:58 TOML Exp $
//
//
//

#ifndef __AITRGINF_H
#define __AITRGINF_H

///////////////////////////////////////////////////////////////////////////////
//
// AI Combat shared target information
//

///////////////////////////////////////

enum eAITargetFlags
{
   kAITF_CanRaycast   = 0x01,
   kAITF_Behind       = 0x02,
   kAITF_XXBehind       = 0x04,
   
};

///////////////////////////////////////

enum eAICombatRange 
{ 
   kAICR_None, 
   kAICR_Near, 
   kAICR_Norm, 
   kAICR_JustFar, 
   kAICR_Far, 
   kAICR_Huge, 
   kAICR_HugeZ, 
   kAICR_Unseen,
   
   kAICR_IntMax = 0xffffffff
};

///////////////////////////////////////

struct sAITargetInfo
{
   // Object
   ObjID                id;
   
   // Presumed location
   mxs_vector           loc;
   float                zLocNearFloor;
   tAIPathCellID        cell;
   
   // Raw distance, angle
   float                distSq;
   floatang             targetHeading;       // the heading of the target from us
   floatang             targetHeadingDelta;  // delta between our heading & above
   
   // Tactical range
   eAICombatRange       range;
   
   // Flags
   unsigned             flags;

   // Raw awareness
   const sAIAwareness * pAwareness;
};

///////////////////////////////////////////////////////////////////////////////

#endif /* !__AITRGINF_H */
