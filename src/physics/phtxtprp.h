// $Header: r:/prj/cam/src/physics/RCS/phtxtprp.h 1.1 1998/06/16 17:49:35 mahk Exp $
#pragma once  
#ifndef __PHTXTPRP_H
#define __PHTXTPRP_H
#include <objtype.h>

////////////////////////////////////////////////////////////
// PHYSICS PROPERTIES FOR TEXTURES
//
// Could be useful for other things too.  
// 

EXTERN void PhysTexturePropsInit(void); 
EXTERN void PhysTexturePropsTerm(void); 


//
// Friction
//

#define PROP_FRICTION_NAME "Friction" 
EXTERN float GetObjFriction(ObjID obj);
EXTERN void  SetObjFriction(ObjID obj, float val); 

//
// Elasticity
//

#define PROP_ELASTICITY_NAME "Elasticity" 
EXTERN float GetObjElasticity(ObjID obj);
EXTERN void  SetObjElasticity(ObjID obj, float val); 

//
// Climbability
//

#define PROP_CLIMBABILITY_NAME "Climbability" 
EXTERN float GetObjClimbability(ObjID obj);
EXTERN void  SetObjClimbability(ObjID obj, float val); 

//
// Can attach (should this really be a link?)
//
#define PROP_CAN_ATTACH_NAME "CanAttach" 
EXTERN BOOL  ObjCanAttach(ObjID obj);
EXTERN void  SetObjCanAttach(ObjID obj, BOOL val); 


#endif // __PHTXTPRP_H

