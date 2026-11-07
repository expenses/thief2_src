/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#ifdef EDITOR
#line 1 "r:\\prj\\thief2\\skup\\thief2\\src\\SCRIPT\\mtscrpt.cpp"
#else
#line 1 "s:\\prj\\thief2\\src\\script\\mtscrpt.cpp"
#endif
#line 1
// $Header: r:/t2repos/thief2/src/script/mtscrpt.cpp,v 1.1 1999/12/23 16:53:54 BFarquha Exp $

#include <comtools.h>
#include <appagg.h>

#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>

#include <mtscrpt.h>

// must be last header
#include <dbmem.h>


////////////////////////////////////////
//
// BODY SCRIPTING MESSAGE IMPLEMENTATION
//

IMPLEMENT_SCRMSG_PERSISTENT(sMediumTransMsg)
{
   PersistenceHeader(sScrMsg, kMediumTransMsgVer);
   PersistentEnum(nFromType);
   PersistentEnum(nToType);
   return TRUE;
}






