#ifndef __FLREGION_H
#define __FLREGION_H
/*
 * $Source: r:/prj/cam/src/RCS/reg.h $
 * $Revision: 1.1 $
 * $Author: mahk $
 * $Date: 1996/07/15 22:53:32 $
 *
 */

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
#include <region.h>

// THIS FILE CONTAINS A FLIGHT LAYER ABOVE THE REGION API
// THAT WILL BE MORE COMPATIBLE WITH THE FUTURE(TM)


#define make_region(parent,region,rect,z,data) \
   region_create(parent,region,rect,z,0,REG_USER_CONTROLLED|AUTOMANAGE_FLAG,NULL,NULL,NULL,data);
   
#define set_region_expose(reg,exp) \
   do { \
      (reg)->status_flags |= EXPOSE_CB; \
      (reg)->expose = (exp);  \
   } while(0)


#ifdef __cplusplus
}
#endif // __cplusplus
#endif // __FLREGION_H

