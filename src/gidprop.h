// $Header: r:/prj/cam/src/RCS/gidprop.h 1.3 1998/03/26 13:51:47 mahk Exp $

#ifndef GIDPROP_H
#define GIDPROP_H
#include <property.h>

typedef struct GlobalID GlobalID;

//------------------------------------------------------------
// GID PROPERTY INTERFACE 
//

#undef INTERFACE
#define INTERFACE IGlobalIDProperty
DECLARE_PROPERTY_INTERFACE(IGlobalIDProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(GlobalID*); 
}; 
#undef INTERFACE


//------------------------------------------------------------
// GID PROPERTY NAME
//

#define PROP_GID_NAME "GID"

//------------------------------------------------------------
// GID PROPERTY API
//



EXTERN void GlobalIDPropInit(void);



#endif // GIDPROP_H
