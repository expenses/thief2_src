// $Header: x:/prj/tech/libsrc/object/property/RCS/propknow.h 1.1 1999/04/07 14:40:21 TOML Exp $
#pragma once  
#ifndef __PROPKNOW_H
#define __PROPKNOW_H

#include <propman.h>
#include <appagg.h>
#include <lazyagg.h>

//
// A dumb "thing that knows about the property manager" class
//


class cPropertyManagerKnower
{
protected:
   static LazyAggMember(IPropertyManager) pPropMan; 
public:

   static IPropertyManager *GetManager() { return pPropMan; };
};


#endif // __PROPKNOW_H

