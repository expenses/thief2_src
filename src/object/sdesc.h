// $Header: x:/prj/tech/libsrc/object/obsolete/RCS/sdesc.h 1.1 1999/04/07 14:40:03 TOML Exp $

#ifndef SDESC_H
#define SDESC_H

#include <comtools.h>
#include <sdestype.h>
#include <sdestool.h>
#include <appagg.h>

//
// THE SDESC API HAS MOVED TO SDESTOOL.H!  
// 
// This macro exists for back-compatibility, since most people only just register structs. 
//


#define StructDescRegister(str) do { \
   IStructDescTools* tools = AppGetObj(IStructDescTools); \
   COMCall1(tools,Register,str); \
   SafeRelease(tools); \
   } while (0)

#endif   // SDESC_H





