///////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/ai/RCS/aiprdev.h 1.6 1999/02/14 16:02:21 JON Exp $
//
// Device properties
//

#ifndef __AIDEVPR_H
#define __AIDEVPR_H

#include <aiprops.h>
#include <aiutils.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

F_DECLARE_INTERFACE(IAIDeviceProperty);
F_DECLARE_INTERFACE(IAITurretProperty);
F_DECLARE_INTERFACE(IAICameraProperty);

///////////////////////////////////////////////////////////////////////////////

void AIInitDeviceProp(void);
void AITermDeviceProp(void);
void AIInitTurretProp(void);
void AITermTurretProp(void);
void AIInitCameraProp(void);
void AITermCameraProp(void);

///////////////////////////////////////

#define PROP_AI_Device "AI_Device"

EXTERN IAIDeviceProperty * g_pAIDeviceProperty;

struct sAIDeviceParams
{
   // raise/lower params
   int m_jointRaise;
   float m_inactivePos;
   float m_activePos;
   float m_raiseSpeed;
   // rotate params
   int m_jointRotate;
   float m_facingEpsilon;
};

EXTERN const sAIDeviceParams g_AIDefaultDeviceParams;

#define AIGetDeviceParams(obj) \
   AIGetProperty(g_pAIDeviceProperty, (obj), (sAIDeviceParams *)&g_AIDefaultDeviceParams)

#undef  INTERFACE
#define INTERFACE IAIDeviceProperty
DECLARE_PROPERTY_INTERFACE(IAIDeviceProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAIDeviceParams *);
};

///////////////////////////////////////

#define PROP_AI_Turret "AI_Turret"

EXTERN IAITurretProperty * g_pAITurretProperty;

struct sAITurretParams
{
   float m_fireEpsilon;
   tSimTime m_firePause;
   float m_pitchEpsilon;
   float m_maxRange;
};

EXTERN const sAITurretParams g_AIDefaultTurretParams;

#define AIGetTurretParams(obj) \
   AIGetProperty(g_pAITurretProperty, (obj), (sAITurretParams *)&g_AIDefaultTurretParams)

#undef  INTERFACE
#define INTERFACE IAITurretProperty
DECLARE_PROPERTY_INTERFACE(IAITurretProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAITurretParams *);
};

///////////////////////////////////////

#define PROP_AI_Camera "AI_Camera"

EXTERN IAICameraProperty * g_pAICameraProperty;

struct sAICameraParams
{
   float m_scanAngle1;
   float m_scanAngle2;
   float m_scanSpeed;
};

EXTERN const sAICameraParams g_AIDefaultCameraParams;

#define AIGetCameraParams(obj) \
   AIGetProperty(g_pAICameraProperty, (obj), (sAICameraParams *)&g_AIDefaultCameraParams)

#undef  INTERFACE
#define INTERFACE IAICameraProperty
DECLARE_PROPERTY_INTERFACE(IAICameraProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAICameraParams *);
};

#pragma pack()

#endif /* !__AIDEVPR_H */



