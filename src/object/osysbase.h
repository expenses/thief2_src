// $Header: x:/prj/tech/libsrc/object/rcs/osysbase.h 1.10 1999/03/17 13:00:43 ccarollo Exp $

#ifndef OSYSBASE__H
#define OSYSBASE__H

#include <objtype.h>
#include <objdef.h>
#include <iobjsys_.h>

#include <mprintf.h>

////////////////////////////////////////
//
// All backward-compatibility now!
//

inline BOOL ObjExists(ObjID obj)
{
   mprintf("ObjExists() macro is obsolete!\n");

   return gObjSys->Exists(obj);
}

#endif // OSYSBASE__H




