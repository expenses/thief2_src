// $Header: x:/prj/tech/libsrc/object/objcore/RCS/objtype.h 1.1 1999/04/07 14:38:44 TOML Exp $

#ifndef _OBJTYPE_H
#define _OBJTYPE_H

#include <lg.h>

//
// Object IDs
//
typedef long   ObjID;
typedef ushort HostID;

#define OBJ_NULL 0

// The root of all archetypes
#define ROOT_ARCHETYPE (-1)

// Abstract vs concrete objects 
#define OBJ_IS_ABSTRACT(obj)   ((ObjID)(obj) < (ObjID)0)
#define OBJ_IS_CONCRETE(obj)   ((ObjID)(obj) > (ObjID)0)

// 
// Other types used by IObjectSystem 
//

// bitmask of whether an object is concrete or abstract 
typedef ulong eObjConcreteness; 

// notification messages
typedef ulong eObjNotifyMsg;
typedef void* ObjNotifyData;

// listener descriptor
typedef struct sObjListenerDesc sObjListenerDesc; 
typedef void (* ObjListenerFunc)(ObjID obj, eObjNotifyMsg msg, void* data); 
typedef int tObjListenerHandle; 

typedef struct Label Label;

// enum for selecting a property implemented by the objsys
typedef ulong eObjSysProperty;


// Every object belongs to a "partition."  A save file contains one or more object partitions. 
typedef ulong eObjPartition; 

typedef struct sObjPartitionFilter sObjPartitionFilter; 


#endif // _OBJTYPE_H 


