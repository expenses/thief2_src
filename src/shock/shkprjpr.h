// $Header: r:/prj/cam/src/shock/RCS/shkprjpr.h 1.1 1998/07/21 11:07:07 JON Exp $
// Shock projectile related properties

#ifndef __SHKPRJPR_H
#define __SHKPRJPR_H

#include <propface.h>
#include <shkpjbas.h>

F_DECLARE_INTERFACE(IProjectileProperty);

#undef INTERFACE
#define INTERFACE IProjectileProperty

DECLARE_PROPERTY_INTERFACE(IProjectileProperty)
{
   DECLARE_UNKNOWN_PURE();                // IUnknown methods 
   DECLARE_PROPERTY_PURE();               // IProperty methods
   DECLARE_PROPERTY_ACCESSORS(sProjectile*);  // Type-specific accessors, by reference
};

#define PROP_PROJECTILE_DESC "Projectile"

EXTERN IProjectileProperty *g_pProjectileProperty;

EXTERN void ProjectilePropertyInit();
EXTERN BOOL ProjectileGet(ObjID objID, sProjectile **ppProjectile);
#define PROJECTILE_DESC_SET(objID, ppParams) \
   PROPERTY_SET(g_projectileProperty, objID, (sProjectile**)ppParams)
#define PROJECTILE_DESC_DELETE(objID) \
   IProperty_Delete(g_projectileProperty, objID)

#endif