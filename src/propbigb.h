// $Header: r:/prj/cam/src/RCS/propbigb.h 1.1 1997/01/04 00:03:28 mahk Exp $

#ifndef PROPBIGB_H
#define PROPBIGB_H

////////////////////////////////////////////////////////////
// BASE TYPES FOR BIG ARRAY PROPERTY IMPLEMENTATION 
////////////////////////////////////////////////////////////


//
// Descriptor structure
//

typedef void* tPropArrayElem;


struct sPropArrayDesc
{
   tPropArrayElem* vector; // the actual vector
   ObjID min_idx;  // min_idx <= idx < max_idx
   ObjID max_idx; 
   int elemsize; // size of an element 
}; 


#endif // PROPBIGB_H
