// $Header: x:/prj/tech/libsrc/object/property/RCS/propinv.h 1.1 1999/04/07 14:40:21 TOML Exp $
#ifndef __PROPINV_H
#define __PROPINV_H
#include <property.h>
#include <propdef.h>

////////////////////////////////////////////////////////////
// INVERTIBLE PROPERTY INTERFACES
// 
// Invertible properties are properties that can also be mapped backwards 
// from values to objects.
//

#define DECLARE_INVERSE_PROPERTY_ACCESSORS(TYPE) \
   STDMETHOD_ (BOOL, GetObj)(THIS_ TYPE val, ObjID* objp) CONSTFUNC PURE; 



#endif // __PROPINV_H



