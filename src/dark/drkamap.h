// $Header: r:/prj/cam/src/dark/RCS/drkamap.h 1.3 1998/09/29 18:00:07 mahk Exp $
#pragma once  
#ifndef __DRKAMAP_H
#define __DRKAMAP_H

////////////////////////////////////////////////////////////
//
// Automap API  
//

EXTERN void DarkAutomapInit(void);
EXTERN void DarkAutomapTerm(void); 

EXTERN void SwitchToDarkAutomapMode(BOOL push); 

//
// Quest variables
//
#define MIN_PAGE_QVAR "MAP_MIN_PAGE"
#define MAX_PAGE_QVAR "MAP_MAX_PAGE"

#define OOB_PAGE_QVAR "MAP_OOB_PAGE"
#define OOB_LOC_QVAR  "MAP_OOB_LOC"





#endif // __DRKAMAP_H
