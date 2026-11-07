/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#ifdef EDITOR
#line 1 "r:\\prj\\thief2\\skup\\thief2\\src\\SCRIPT\\rooscrpt.cpp"
#else
#line 1 "s:\\prj\\thief2\\src\\script\\rooscrpt.cpp"
#endif
#line 1
// $Header: r:/t2repos/thief2/src/script/rooscrpt.cpp,v 1.2 1998/10/05 17:27:39 mahk Exp $
// 

#include <lg.h>

#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>

#include <rooscrpt.h>

// Must be last header 
#include <dbmem.h>


IMPLEMENT_SCRMSG_PERSISTENT(sRoomMsg)
{
   PersistenceHeader(sScrMsg,kRoomMsgVer);
   Persistent(ToObjId);
   Persistent(FromObjId);
   Persistent(MoveObjId);
   PersistentEnum(ObjType);
   PersistentEnum(TransitionType);
   return TRUE;
}


