// $Header: r:/prj/cam/src/shock/RCS/shkdmg.h 1.2 1998/07/09 14:41:11 JON Exp $
// Shock damage models

#ifndef __SHKDMG_H
#define __SHKDMG_H

#include <dmgmodel.h>

EXTERN void ShockDamageInit(void);
EXTERN void ShockDamageShutDown(void);

EXTERN eDamageResult LGAPI ShockDamageListener(const sDamageMsg* pMsg, tDamageCallbackData data);

#endif