// $Header: x:/prj/tech/libsrc/object/RCS/dataopst.h 1.2 1999/01/27 16:00:10 mahk Exp $
#pragma once  
#ifndef __DATAOPST_H
#define __DATAOPST_H

#include <comtools.h>

F_DECLARE_INTERFACE(IDataOps);
F_DECLARE_INTERFACE(IDataOpsFile); 

#ifndef CONSTFUNC
#ifdef __cplusplus
#define CONSTFUNC const
#else
#define CONSTFUNC
#endif 
#endif 

//
// A generic 32-bit value, that might be a pointer, or might not.
//

struct sDatum
{
   void* value; 

   operator void*() { return value; }; 
   sDatum(void* v = NULL) : value(v) {}; 
}; 


#endif // __DATAOPST_H
