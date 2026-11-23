#include "stdafx.h"
#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <stimsens.h>
#include <sensbase.h>
#include <receptro.h>
#include <stimbase.h>
#include <linkbase.h>
#include <aredit.h>

#include "darkdlgs.h"
#include "tronview.h"
#include "tronlist.h"
#include "aredit_.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// cTronView dialog


cTronView::cTronView(const sLinkEditorDesc* desc, cQueryFactory* qfact, CWnd* pParent /*=NULL*/)
	: cLinkView(desc,qfact,pParent,cTronView::IDD,new cTronList)
{
   m_delete_list = TRUE; 
	//{{AFX_DATA_INIT(cTronView)
	//}}AFX_DATA_INIT
}


void cTronView::DoDataExchange(CDataExchange* pDX)
{
	cLinkView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(cTronView)
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(cTronView, cLinkView)
	//{{AFX_MSG_MAP(cTronView)
	ON_BN_CLICKED(IDC_ADD, OnAdd)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// cTronView message handlers

void cTronView::OnAdd() 
{
   sLink link = { 0, 0, 0}; 
   LinkID id = LINKID_NULL; 

   // start from the last selected receptron, if any
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
   sReceptron tron; 
   memset(&tron,0,sizeof(tron)); 

   AutoAppIPtr_(StimSensors,pSensors); 
   if (id != LINKID_NULL)
      pSensors->GetReceptron(id,&tron); 

   IActReactEditors* pEdit = ExportActReactEditors(); 
   if (pEdit->ReceptronDialog(kARDlgModal,&pair,&tron) == S_OK)
   {
      if (pair.obj == OBJ_NULL || pair.stim == OBJ_NULL)
         return; 
      if (SUCCEEDED(pSensors->AddReceptron(pair.obj,pair.stim,&tron)))
         RecomputeList(); 
   }
   SafeRelease(pEdit); 
}
