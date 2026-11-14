#line 57
///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/cpptools/RCS/dynarray.cpp $
// $Author: TOML $
// $Date: 1998/05/23 07:08:04 $
// $Revision: 1.11 $
//

#include <lg.h>
#include <dynarray.h>

#include <stdlib.h>

#ifndef NO_DB_MEM
// Must be last header
#include <memall.h>
#include <dbmem.h>
#endif

///////////////////////////////////////
//
// Resize the array buffer to the desired number of slots.
//
// NOTE: The logical size does not change.
//

BOOL cDABaseSrvFns::DoResize(void **ppItems, unsigned nItemSize, unsigned nNewSlots)
{
   if (nNewSlots)
   {
      void* newP;

      if (*ppItems)
         newP = realloc(*ppItems, nNewSlots * nItemSize);
      else
         newP = malloc(nNewSlots * nItemSize);

      AssertMsg(newP, "Dynamic array resize failed");

      if (!newP)
         return FALSE;

      *ppItems = newP;
   }

   else if (*ppItems)
   {
      free(*ppItems);
      *ppItems = 0;
   }

   return TRUE;
}

#ifndef SHIP
const char cDABaseSrvFns::gm_pszOutOfRange[] = "Index %d out of range";
#endif
