// $Header: r:/prj/cam/src/shock/RCS/shkprutl.h 1.2 1999/11/19 14:53:47 adurant Exp $

This file has been moved to nzprutil.h AMSD

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
