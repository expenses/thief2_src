// $Header: r:/t2repos/thief2/dllsrc/mottable/mottable.h,v 1.1 1998/01/08 15:50:11 kate Exp $

#ifndef __MOTTABLE_H
#define __MOTTABLE_H

#include <imottab.h>

#define DLLEXPORT __declspec (dllexport)

EXTERN DLLEXPORT IMotionTable * LGAPI GetMotionTable();


#endif // __MOTTABLE_H
