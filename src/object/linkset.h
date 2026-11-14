// $Header: x:/prj/tech/libsrc/object/link/RCS/linkset.h 1.1 1999/04/07 14:38:22 TOML Exp $
#pragma once  
#ifndef __LINKSET_H
#define __LINKSET_H

#include <linktype.h>
#include <listset.h>

////////////////////////////////////////////////////////////
// LINK SET CLASS
//


class cLinkSet : public cSimpleListSet<LinkID> 
{
public: 

   // Spit out a link query
   ILinkQuery* Query(tQueryDate bday = QUERY_DATE_NONE); 
   ILinkQuery* PatternQuery(const struct sLinkTemplate* pattern, tQueryDate bday = QUERY_DATE_NONE); 
};



#endif // __LINKSET_H







