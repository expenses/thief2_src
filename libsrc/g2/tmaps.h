// $Header: x:/prj/tech/libsrc/g2/RCS/tmaps.h 1.3 1997/05/16 09:38:49 KEVIN Exp $

#ifndef __TMAPS_H
#define __TMAPS_H

#include <dev2d.h>
#include <plyparam.h>

typedef void (*g2_il_func)(int x, int xf, fix u, fix v);

typedef struct {
   fix du_frac,dv_frac;
   long dsrc[2];
} g2s_tmap_info;

#endif

