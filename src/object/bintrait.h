// $Header: x:/prj/tech/libsrc/object/link/RCS/bintrait.h 1.1 1999/04/07 14:38:20 TOML Exp $
#pragma once  
#ifndef __BINTRAIT_H
#define __BINTRAIT_H
#include <comtools.h>

////////////////////////////////////////////////////////////
// Tools for using relations as "binary traits" 
//

F_DECLARE_INTERFACE(ITrait); 
F_DECLARE_INTERFACE(IRelation); 
F_DECLARE_INTERFACE(ILinkQuery); 

//------------------------------------------------------------
// SLOW, UNCACHED VERSIONS
//
//
// Query all links inherited by one or two objs
// 


// All links out from me or my donors to just you, in inheritance order 
// dest may be a wildcard 
EXTERN ILinkQuery* QueryInheritedLinksSingleUncached(IRelation* rel, ObjID src, ObjID dest);  

// All links from me and my donors to you and your donors, in inheritance order
// No wildcards allowed 
EXTERN ILinkQuery* QueryInheritedLinksUncached(IRelation* rel, ObjID src, ObjID dest);  

//------------------------------------------------------------
// CACHED VERSIONS
//

// Make a "trait" for use by these functions.  You should do this once.  
ITrait* MakeTraitFromRelation(IRelation* pRel); 

//
// Query all links inherited by one or two objs
// 

// All links out from me or my donors to just you, in inheritance order 
// dest may be a wildcard 
EXTERN ILinkQuery* QueryInheritedLinksSingle(ITrait* trait, IRelation* rel, ObjID src, ObjID dest);  

// All links from me and my donors to you and your donors, in inheritance order
// No wildcards allowed 
EXTERN ILinkQuery* QueryInheritedLinks(ITrait* trait, IRelation* rel, ObjID src, ObjID dest);  




#endif // __BINTRAIT_H



