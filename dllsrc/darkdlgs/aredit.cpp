// aredit.cpp
//
// The act/react editors: stimulus source & receptron dialogs and lists
//

#include "stdafx.h"
#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <wappapi.h>
#include <linkman.h>
#include <relation.h>
#include <edittool.h>
#include <linkedst.h>

#include "darkdlgs.h"
#include "aredit_.h"
#include "srcsheet.h"
#include "trondlg.h"
#include "srcview.h"
#include "tronview.h"

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cAREditors,IActReactEditors); 

cAREditors::cAREditors()
{
}

cAREditors::~cAREditors()
{
}

////////////////////////////////////////////////////////////

STDMETHODIMP cAREditors::SourceDialog(eARDlgFlags flags, sObjStimPair* elems, sStimSourceDesc* desc)
{
   ASSERT(flags & kARDlgModal); // modeless NYI

   AutoAppIPtr(WinApp); 
   CWnd* appwnd = CWnd::FromHandle(pWinApp->GetMainWnd()); 
   cStimSrcSheet sheet(appwnd,elems,desc); 
   if (sheet.DoModal() == IDOK)
   {
      sheet.ToSource(elems,desc);
      return S_OK;
   }
   return S_FALSE; 
}

STDMETHODIMP cAREditors::ReceptronDialog(eARDlgFlags flags, sObjStimPair* elems, sReceptron* desc)
{
   ASSERT(flags & kARDlgModal); // modeless NYI

   AutoAppIPtr(WinApp); 
   CWnd* appwnd = CWnd::FromHandle(pWinApp->GetMainWnd()); 
   cReceptronDialog sheet(elems,desc,appwnd); 
   if (sheet.DoModal() == IDOK)
   {
      sheet.ToTron(elems,desc);
      return S_OK;
   }
   return S_FALSE; 
}

//
// Query factory for a source/dest/relation triple
//

class cStandardFactory : public cLinkView::cQueryFactory
{
   ObjID src; 
   ObjID dest; 
   RelationID rel; 
   ILinkManager* LinkMan; 

public:
   cStandardFactory(ObjID s, ObjID d, RelationID r)
      : src(s), dest(d), rel(r), LinkMan(AppGetObj(ILinkManager))
   {
   }

   ~cStandardFactory()
   {
      SafeRelease(LinkMan); 
   }

   ILinkQuery* Query()
   {
      return LinkMan->Query(src,dest,rel); 
   }
}; 

STDMETHODIMP cAREditors::ReceptronList(eARDlgFlags flags, const sObjStimPair* pair)
{
   ASSERT(flags & kARDlgModal); // modeless NYI

   CWnd* appwnd = CWnd::FromHandle(GetActiveWindow()); 
   sLinkEditorDesc copydesc = { "", kLinkEditShowAll }; 
   ObjID src = pair->obj; 
   ObjID dest = pair->stim; 

   if (copydesc.title[0] == '\0')
   {
      AutoAppIPtr_(EditTools,pTools); 
      CString srcname = (src == OBJ_NULL) ? "Any" : pTools->ObjName(src); 
      CString destname = (dest == OBJ_NULL) ? "Any" : pTools->ObjName(dest); 
      char buf[256]; 
      sprintf(buf,"Receptrons: %s ----> %s",srcname,destname); 
      strncpy(copydesc.title,buf,sizeof(copydesc.title)); 
   }

   AutoAppIPtr_(LinkManager,pLinkMan); 
   IRelation* rel = pLinkMan->GetRelationNamed("Receptron");
   RelationID flav = rel->GetID();
   gCurrentSrc = src;
   gCurrentDest = dest;
   gCurrentFlavour = flav; 

   cLinkView::cQueryFactory* fact = new cStandardFactory(src,dest,flav); 
   cTronView view(&copydesc,fact,appwnd); 
   view.Go(); 
   return S_OK; 
}

STDMETHODIMP cAREditors::SourceList(eARDlgFlags flags, const sObjStimPair* pair)
{
   ASSERT(flags & kARDlgModal); // modeless NYI

   CWnd* appwnd = CWnd::FromHandle(GetActiveWindow()); 
   sLinkEditorDesc copydesc = { "", kLinkEditShowAll }; 
   ObjID src = pair->obj; 
   ObjID dest = pair->stim; 

   if (copydesc.title[0] == '\0')
   {
      AutoAppIPtr_(EditTools,pTools); 
      CString srcname = (src == OBJ_NULL) ? "Any" : pTools->ObjName(src); 
      CString destname = (dest == OBJ_NULL) ? "Any" : pTools->ObjName(dest); 
      char buf[256]; 
      sprintf(buf,"Sources: %s ----> %s",srcname,destname); 
      strncpy(copydesc.title,buf,sizeof(copydesc.title)); 
   }

   AutoAppIPtr_(LinkManager,pLinkMan); 
   IRelation* rel = pLinkMan->GetRelationNamed("arSrcDesc");
   RelationID flav = rel->GetID();
   gCurrentSrc = src;
   gCurrentDest = dest;
   gCurrentFlavour = flav; 

   cLinkView::cQueryFactory* fact = new cStandardFactory(src,dest,flav); 
   cSourceView view(&copydesc,fact,appwnd); 
   view.Go(); 
   return S_OK; 
}

////////////////////////////////////////////////////////////

extern "C" __declspec(dllexport) IActReactEditors* LGAPI ExportActReactEditors()
{
   return new cAREditors; 
}
