// $Header: x:/prj/tech/libsrc/object/link/RCS/listset.h 1.1 1999/04/07 14:38:22 TOML Exp $
#pragma once  
#ifndef __LISTSET_H
#define __LISTSET_H

#include <dlistsim.h>

////////////////////////////////////////////////////////////
// "Set" augmentation for cSimpleDList
//


template <class ELEM>
class cSimpleListSet : public cSimpleDList<ELEM>
{
   ulong nElems;

public:
   cSimpleListSet() : nElems(0) {};

   BOOL AddElem(const ELEM& elem);
   BOOL RemoveElem(const ELEM& elem);
   BOOL HasElem(const ELEM& elem);
   void RemoveAll();
   ulong Size() { return nElems; }; 
};

#endif // __LISTSET_H



