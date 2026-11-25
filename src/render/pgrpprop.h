/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/render/pgrpprop.h,v 1.7 2000/01/31 09:53:06 adurant Exp $
//
// Particle group property
//
#pragma once

#ifndef __PGRPPROP_H
#define __PGRPPROP_H

#include <property.h>
#include <parttype.h>

////////////////////////////////////////////////////////////////////////////////

enum eParticleAttachType
{
   kPAT_object,
   kPAT_vhot,
   kPAT_joint,
   kPAT_submodel,
};

typedef struct 
{
   int type;

   int vhot;
   int joint;
   int submodel;
} sParticleAttachLinkData;

////////////////////////////////////////////////////////////////////////////////

enum eDetailAttachType
{
   kDAT_object,
   kDAT_vhot,
   kDAT_joint,
   kDAT_submodel,
   kDAT_subobject,
   kDAT_transparent_decal,
};

enum eDetailAttachFlags
{
   kDAF_NoAutoDelete = 0x01,
   kDAF_NoJointRot   = 0x02,
};

typedef struct
{
   uchar type;
   uchar joint;
   short vhot_or_sub;
   mxs_vector rel_pos;
   mxs_angvec rel_rot;
   uchar flags;
   uchar pad;
   ulong decal_plane_norm;
   float decal_plane_dist;
} sDetailAttachLinkData;

////////////////////////////////////////////////////////////////////////////////

#define PROP_PARTICLE_GROUP_NAME       "ParticleGroup"
#define PROP_PARTICLE_LAUNCH_INFO_NAME "PGLaunchInfo"

#define LINK_PARTICLE_ATTACHMENT_NAME  "ParticleAttachement"

////////////////////////////////////////////////////////////////////////////////

#undef  INTERFACE
#define INTERFACE IParticleGroupProperty
DECLARE_PROPERTY_INTERFACE(IParticleGroupProperty)
{
   DECLARE_UNKNOWN_PURE(); 
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(ParticleGroup *); 
}; 

////////////////////////////////////////////////////////////////////////////////

#undef  INTERFACE
#define INTERFACE IPGroupLaunchInfoProperty
DECLARE_PROPERTY_INTERFACE(IPGroupLaunchInfoProperty)
{
   DECLARE_UNKNOWN_PURE(); 
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(ParticleLaunchInfo *); 
};

////////////////////////////////////////////////////////////////////////////////

EXTERN void ParticleGroupPropInit(void);
EXTERN void ParticleGroupPropTerm(void);

#ifdef EDITOR
EXTERN void ParticleGroupRefreshLinks();
#endif

EXTERN ParticleGroup *ObjGetParticleGroup(ObjID obj);
EXTERN void ObjSetParticleGroup(ObjID obj, ParticleGroup *pg);
EXTERN void ObjGetParticleAttachment(ObjID obj, ObjID *attach_obj, sParticleAttachLinkData **data);
EXTERN ParticleLaunchInfo *ObjGetParticleGroupLaunchInfo(ObjID obj);

EXTERN BOOL DetailAttachCalcRelTransform(ObjID child, ObjID parent, sDetailAttachLinkData *data);

EXTERN BOOL ObjIsParticleGroup(ObjID obj);

EXTERN BOOL ObjParticleIsActive(ObjID obj);
EXTERN void ObjParticleSetActive(ObjID obj, BOOL active);

EXTERN void ParticleGroupEnterMode(void); 
EXTERN void ParticleGroupExitMode(void); 

// Create and particles for an object (owner) from an abstract object
EXTERN void ParticleGroupCreateFromArchetype(ObjID archetypeID, ObjID ownerID);
// Destroy all particle groups attached to this object
EXTERN void ParticleGroupDestroyAttached(ObjID ownerID);

////////////////////////////////////////////////////////////////////////////////

#endif



