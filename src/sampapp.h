// $Header: r:/prj/cam/src/RCS/sampapp.h 1.2 1996/11/10 02:37:23 mahk Exp $

#ifndef __SAMPAPP_H
#define __SAMPAPP_H

//
// @INSTRUCTIONS: 1) Rename this function to something appropriate to your system.
//                2) Change MY_CREATEFUNC in sampapp.c to use the same name
//
EXTERN void LGAPI SampSysCreate(void);

//
// @INSTRUCTIONS: 1) Check out camguids.c and grab a GUID (instructions are in that file)
//                2) Name the GUID and put it's definition below.
//                3) Change my_guid in sampapp.c to point to your new GUID.
//
DEFINE_LG_GUID(UUID_SampApp, SOME_NUMERIC_CONSTANT);

//
// KEEP THIS FILE RELATIVELY CLEAR OF JUNK!  DO NOT PUT YOUR SYSTEM'S API HERE! 
// DO NOT INCLUDE OTHER HEADER FILES FROM THIS FILE!
//

#endif // __SAMPAPP_H

