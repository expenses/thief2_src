///////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/physics/RCS/phystyp2.h 1.19 1998/11/20 14:36:41 MROWLEY Exp $
//
// Physics types
//
// (Must be C friendly, so no classes)
//

#ifndef __PHYSTYP2_H
#define __PHYSTYP2_H

#include <matrixs.h>

typedef void (*PhysReadWrite) (void *buf, size_t elsize, size_t nelem);

typedef int tPhysSubModId;

// a collision
typedef struct sPhysClsn sPhysClsn;

// enum types
typedef uint ePhysIntersectResult;
typedef uint ePhysClsnResult;

typedef enum ePhysForceType
{
   kPFT_Gravity = 0x0100,   // Force to 4 bytes
   kPFT_Wind,
   kPFT_Current,
   kPFT_Collision,
} ePhysForceType;

enum ePhysCtrlType
{
   kPCT_NoControl      = 0x0000,
   kPCT_VelControl     = 0x0001,
   kPCT_RotVelControl  = 0x0002,
   kPCT_LocControl     = 0x0004,
   kPCT_RotControl     = 0x0008,
};

enum ePhysModelCreateFlags
{
   kPMCF_Default        = 0,
   kPMCF_NonMoveable    = 0x0001,
   kPMCF_Uncontrollable = 0x0002,
   kPMCF_NoGravity      = 0x0004,
   kPMCF_Remote         = 0x0008,
   kPMCF_LowDetail      = 0x0010,
   kPMCF_Player         = 0x0020,
   kPMCF_Creature       = 0x0040,
   kPMCF_Rope           = 0x0080,
   kPMCF_Weapon         = 0x0100,
};

#define kPhysSphereDefRadius 1

typedef int ObjID;
typedef BOOL (*LimitCallback) (ObjID objID);

typedef struct
{
   int axis;
   mxs_vector plane_norm;
   LimitCallback callback;
} sAngleLimit;

typedef struct
{
   mxs_vector norm;
   mxs_real   plane_const;
   LimitCallback callback;
} sTransLimit;

typedef struct
{
   ePhysForceType type;
   mxs_vector     vector;     // @TBD: Force or acceleration?
} sPhysForce;

#endif /* !__PHYSTYP2_H */



