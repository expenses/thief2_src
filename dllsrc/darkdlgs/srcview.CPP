#include "stdafx.h"
#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <stimsrc.h>
#include <ssrcbase.h>
#include <stimbase.h>
#include <linkbase.h>
#include <aredit.h>

#include "darkdlgs.h"
#include "srcview.h"
#include "srclist.h"
#include "aredit_.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// cSourceView dialog


cSourceView::cSourceView(const sLinkEditorDesc* desc, cQueryFactory* qfact, CWnd* pParent /*=NULL*/)
	: cLinkView(desc,qfact,pParent,cSourceView::IDD,new cSourceList)
{
   m_delete_list = TRUE; 
	//{{AFX_DATA_INIT(cSourceView)
	//}}AFX_DATA_INIT
}


void cSourceView::DoDataExchange(CDataExchange* pDX)
{
	cLinkView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(cSourceView)
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(cSourceView, cLinkView)
	//{{AFX_MSG_MAP(cSourceView)
	ON_BN_CLICKED(IDC_ADD, OnAdd)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// cSourceView message handlers

void cSourceView::OnAdd() 
{
   sLink link = { 0, 0, 0}; 
   LinkID id = LINKID_NULL; 

   // start from the last selected source, if any
   int n = m_list.GetItemCount();
   for (int i = n - 1; i >= 0; i--)
      if (i == 0 || m_list.GetItemState(i,LVIS_SELECTED))
      {
         id = m_list.GetItemData(i); 
         m_pLinkMan->Get(id,&link); 
         break; 
      }

   if (id == LINKID_NULL)
   {
      link.source = gCurrentSrc; 
      link.dest = gCurrentDest; 
      link.flavor = gCurrentFlavour; 
   }

   sObjStimPair pair = { link.source, link.dest }; 
   sStimSourceDesc source; 
   memset(&source,0,sizeof(source)); 

   AutoAppIPtr_(StimSources,pSources); 
   if (id != LINKID_NULL)
      pSources->DescribeSource(id,&source); 

   IActReactEditors* pEdit = ExportActReactEditors(); 
   if (pEdit->SourceDialog(kARDlgModal,&pair,&source) == S_OK)
   {
      if (pair.obj == OBJ_NULL || pair.stim == OBJ_NULL)
         return; 
      if (SUCCEEDED(pSources->AddSource(pair.obj,pair.stim,&source)))
         RecomputeList(); 
   }
   SafeRelease(pEdit); 
}
