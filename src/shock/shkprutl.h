// $Header: r:/prj/cam/src/shock/RCS/shkprutl.h 1.1 1998/08/19 15:38:43 JON Exp $

#ifndef __SHKPRUTL_H
#define __SHKPRUTL_H

///////////////////////////////////////////////////////////////////////////////
//
// Handy way to specifify nozeroing without having a seperate class for 
// every property
//

#ifdef __DATAOPS__H

template <class TYPE> 
class cNoZeroDataOps : public cClassDataOps<TYPE>
{
public:
   cNoZeroDataOps()
    : cClassDataOps<TYPE>(cClassDataOps<TYPE>::kNoFlags)
   {
   }
}; 

#else

class cNoZeroDataOps;

#endif // __DATAOPS_H

#endif // __SHKPRUTL_H