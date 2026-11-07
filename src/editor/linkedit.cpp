/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#ifdef EDITOR
#line 1 "r:\\prj\\thief2\\skup\\thief2\\src\\editor\\linkedit.cpp"
#else
#line 1 "s:\\prj\\thief2\\src\\editor\\linkedit.cpp"
#endif
#line 1
// $Header: r:/t2repos/thief2/src/editor/linkedit.cpp,v 1.2 2000/02/19 13:10:57 toml Exp $
#include <wtypes.h>
#include <dynfunc.h>

#include <linkedit.h>
#include <linkedst.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


static void LGAPI linked_woe(const sLinkEditorDesc* , ObjID , ObjID , RelationID )
{
   CriticalMsg("Could not load dialog!");
}

DeclDynFunc_(void,LGAPI,DoLinkEditor,(const sLinkEditorDesc*,ObjID, ObjID ,RelationID));
ImplDynFunc(DoLinkEditor,"darkdlgs.dll","_DoLinkEditor@16",linked_woe);

#define LinkEditor    (DynFunc(DoLinkEditor).GetProcAddress())

void EditLinks(const sLinkEditorDesc* desc, ObjID src, ObjID dest, RelationID flav)
{
   LinkEditor(desc,src,dest,flav);
}

