/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#ifdef EDITOR
#line 1 "r:\\prj\\thief2\\skup\\thief2\\src\\SCRIPT\\bodscrpt.cpp"
#else
#line 1 "s:\\prj\\thief2\\src\\script\\bodscrpt.cpp"
#endif
#line 1
// $Header: r:/t2repos/thief2/src/script/bodscrpt.cpp,v 1.3 1998/10/05 17:21:48 mahk Exp $

#include <comtools.h>
#include <appagg.h>

#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>

#include <bodscrpt.h>

// must be last header
#include <dbmem.h>


////////////////////////////////////////
//
// BODY SCRIPTING MESSAGE IMPLEMENTATION
//

IMPLEMENT_SCRMSG_PERSISTENT(sBodyMsg)
{
   PersistenceHeader(sScrMsg, kBodyMsgVer);
   PersistentEnum(ActionType);
   return TRUE;
}






