// $Header: r:/prj/cam/src/shock/RCS/shkganpr.h 1.2 1998/07/19 11:29:16 JON Exp $
// Shock gun animation properties

#ifndef __SHKGANPR_H
#define __SHKGANPR_H

#include <propface.h>
#include <shkgunan.h>

// Gun Anim 
#undef INTERFACE
#define INTERFACE IGunAnimProperty
DECLARE_PROPERTY_INTERFACE(IGunAnimProperty)
{
   DECLARE_UNKNOWN_PURE(); 
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(sGunAnim*); 
}; 

#define PROP_GUN_ANIM_POST_NAME "AnimPost"
#define PROP_GUN_ANIM_PRE_NAME "AnimPre"

extern IGunAnimProperty *g_gunAnimPreProperty;
extern IGunAnimProperty *g_gunAnimPostProperty;

void GunAnimPropertyInit(void);
void GunAnimPropertyTerm(void);

#endif