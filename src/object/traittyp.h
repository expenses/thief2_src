// $Header: x:/prj/tech/libsrc/object/trait/RCS/traittyp.h 1.1 1999/04/07 14:41:00 TOML Exp $

#ifndef __TRAITTYP_H
#define __TRAITTYP_H
#include <objtype.h>
#include <osystype.h>
#include <comtools.h>

////////////////////////////////////////////////////////////
// BASE TYPES FOR TRAITS
//

// 
// COM interfaces
//
F_DECLARE_INTERFACE(ITrait);
F_DECLARE_INTERFACE(ITraitManager);
F_DECLARE_INTERFACE(IObjectQuery);
F_DECLARE_INTERFACE(IDonorQuery);

//
// Hierarchy types
//

typedef long tMetaPropertyPriority;

//
// Trait/query types
//
typedef struct sTraitDesc sTraitDesc;
typedef ulong eTraitQueryType;
typedef struct sTraitPredicate sTraitPredicate;

//
// Hierarchy listener types
//

typedef struct sHierarchyMsg sHierarchyMsg; 
typedef void* HierarchyListenerData;
typedef void (LGAPI* HierarchyListenFunc)(const sHierarchyMsg* msg, HierarchyListenerData data); 


#endif // __TRAITTYP_H


