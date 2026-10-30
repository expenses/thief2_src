// $Header: x:/prj/tech/libsrc/object/property/RCS/prpstats.h 1.1 1999/04/07 14:40:23 TOML Exp $
#pragma once  
#ifndef __PRPSTATS_H
#define __PRPSTATS_H

enum ePropTimeStats
{
   kPropCopyTime,
   kPropGetDonorTime,
   kPropGetExemplarTime, 
   kPropIterStartTime,
   kPropIterNextTime,
   kPropIterStopTime,
   kPropCreateTime,
   kPropDeleteTime,
   kPropGetTime,
   kPropSetTime,
   kPropRelevantTime, 
   kPropListenerTime,
   kPropTouchTime, 
   kPropRequiresTime,
   kPropImpliesTime,
   kPropRebuildConcretesTime,

   kPropNumTimeStats
}; 

struct sPropTimeStats
{
   ulong time[kPropNumTimeStats]; 
};


#endif // __PRPSTATS_H
