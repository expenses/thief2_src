// $Header: r:/prj/cam/src/shock/RCS/shkpjbas.h 1.1 1998/07/21 11:06:42 JON Exp $

#ifndef __SHKPJBAS_H
#define __SHKPJBAS_H

#include <mxang.h>

// info structure attached to a projectile archetype object
typedef struct sProjectile
{
   int m_spray;             // number of projectiles/trigger pull (instantaneous)
   mxs_ang m_spread;        // random angular error per shot
} sProjectile;

#endif