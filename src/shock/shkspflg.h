// $Header: r:/prj/cam/src/shock/RCS/shkspflg.h 1.1 1998/09/28 21:58:29 XEMU Exp $

#ifndef __SHKSPFLG_H
#define __SHKSPFLG_H

typedef enum
{
   kSpawnFlagNone = 0x0,
   kSpawnFlagPopLimit = 0x1,
   kSpawnFlagPlayerDist = 0x2,

   kSpawnFlagDefault = kSpawnFlagPopLimit|kSpawnFlagPlayerDist,
   kSpawnFlagAll  = 0xFFFF,
};

#endif