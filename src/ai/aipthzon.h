///////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/ai/RCS/aipthzon.h 1.2 1999/03/10 12:58:44 ccarollo Exp $
//
// AI zone-determination header
//

#ifndef __AIPTHZON_H
#define __AIPTHZON_H

#define MAX_AI_PATH_ZONES (65535)

#define AI_ZONE_INVALID   (0)
#define AI_ZONE_SOLO      (65535)
#define AI_ZONE_ALL       (65534)


EXTERN void ResetPathfindZones();
EXTERN void DeterminePathfindZones();
EXTERN void LinkPathfindZones();
EXTERN void FixupPathfindZones();

#endif
