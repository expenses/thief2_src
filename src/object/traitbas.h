// $Header: x:/prj/tech/libsrc/object/trait/RCS/traitbas.h 1.1 1999/04/07 14:40:58 TOML Exp $

#ifndef __TRAITBAS_H
#define __TRAITBAS_H
#include <traittyp.h>

//
// MetaProp Priorities
//

enum eMetaPropertyPriority
{
   // bit which means "special constant" 
   kMPropSpecialPriority = 0x80000000,
   kMPropHighestPriority = kMPropSpecialPriority|1,
   // This really means "lowest, but still higher than archetype" 
   kMPropLowestPriority  = kMPropSpecialPriority|2,

   // Only applies to "special" priorities, indicates that
   // "lower than archetype" is intended
   kMPropPriorityBelowArchetype = 0x40000000,
};

//
// Trait Manager Queries
//

enum _eTraitQueryType
{

   kTraitQueryArchetype,
   kTraitQueryDonors,
   kTraitQueryDescendents,                

   kTraitQueryTransitive = 0x80000000,
   kTraitQueryAllArchetypes = kTraitQueryArchetype|kTraitQueryTransitive,
   kTraitQueryAllDonors = kTraitQueryDonors|kTraitQueryTransitive,
   kTraitQueryAllDescendents = kTraitQueryDescendents|kTraitQueryTransitive,
};

//
// Function that defines a trait; given an object (and some data)
// return whether or not the object possesses (intrinsically) 
// the trait described by that data
//
typedef void* TraitPredicateData;
typedef BOOL (* TraitPredicate)(ObjID obj, TraitPredicateData data);

//
// Trait Descriptor
//


struct sTraitDesc
{
   char name[32];
   ulong flags;
};

//
// Trait descriptor flags
//

enum eTraitFlags
{
   kTraitUninherited = 1 << 0,
   kTraitUncached    = 1 << 1,
};



struct sTraitPredicate 
{
   TraitPredicate func;
   TraitPredicateData data;
};

//
// Hierarchy listener flags
//

enum eHierarchyMsgKind
{
   kDonorAdded,
   kDonorRemoved, 
}; 

struct sHierarchyMsg 
{
   int kind; 
   ObjID obj;
   ObjID donor; 
}; 



#endif // __TRAITBAS_H



