// $Header: x:/prj/tech/libsrc/object/trait/RCS/traitprp.h 1.1 1999/04/07 14:41:00 TOML Exp $

#ifndef __TRAITPRP_H
#define __TRAITPRP_H

////////////////////////////////////////////////////////////
//
// PROPERTIES USED IN TRAIT IMPLEMENTATION
//
//

//
// Donor types   @TODO: move this out if necessary
//
enum eTraitDonorTypes
{
   kDonorArchetype,
   kDonorMetaProp,
};

//------------------------------------------------------------
// PROPERTY NAMES
//

// what kind of abstract object am I
#define PROP_DONOR_TYPE_NAME "DonorType" 
// what is my archetype
#define PROP_ARCHETYPE_NAME  "Archetype" 
// Symbolic name used for naming archetypes/metaprops
#define PROP_SYMNAME_NAME  "SymName"

EXTERN void TraitPropertiesInit(void);
EXTERN void TraitPropertiesTerm(void);


#endif // __TRAITPRP_H
