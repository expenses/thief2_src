// $Header: x:/prj/tech/libsrc/object/objcore/RCS/objdef.h 1.1 1999/04/07 14:38:43 TOML Exp $
#pragma once  
#ifndef __OBJDEF_H
#define __OBJDEF_H

#include <objtype.h> 

//
// Built-in properties
//
enum eObjSysProperty_
{
   kSymNameProperty,     // symbolic name property
   kTransienceProperty,  // whether we save to disk or not 
};

//
// Object Concreteness
//

enum eObjConcreteness_
{
   kObjectConcrete = 1,
   kObjectAbstract = 2,
   kObjectAll = (kObjectConcrete|kObjectAbstract)
};

//
// Listener descriptor 
//

struct sObjListenerDesc 
{
   ObjListenerFunc func; 
   void* data; 
};

//
// Partition filter 
//

// return true IFF the obj belongs in our partition 
typedef BOOL (LGAPI * tObjPartitionFilterFunc)(ObjID obj, const struct sObjPartitionFilter* filter); 

struct sObjPartitionFilter
{
   tObjPartitionFilterFunc func; 
   void* data; 
}; 


#endif // __OBJDEF_H






