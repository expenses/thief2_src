#include "stdafx.h"
#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <linkman.h>
#include <relation.h>
#include <linkbase.h>

#include "darkdlgs.h"
#include "addlink.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// cAddLinkDialog dialog


cAddLinkDialog::cAddLinkDialog(const sLink* link, CWnd* pParent /*=NULL*/)
	: CDialog(cAddLinkDialog::IDD, pParent),
     m_link(*link)
{
	//{{AFX_DATA_INIT(cAddLinkDialog)
	//}}AFX_DATA_INIT
}


void cAddLinkDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
   if (!pDX->m_bSaveAndValidate)
   {
      m_to.SetObj(m_link.dest); 
      m_from.SetObj(m_link.source); 
      m_flavor.SelectItem(m_link.flavor); 
   }
	//{{AFX_DATA_MAP(cAddLinkDialog)
	DDX_Control(pDX, IDC_TO, m_to);
	DDX_Control(pDX, IDC_FROM, m_from);
	DDX_Control(pDX, IDC_FLAVOR, m_flavor);
	//}}AFX_DATA_MAP
   if (pDX->m_bSaveAndValidate)
   {
      m_link.source = m_from.GetObj(); 
      m_link.dest = m_to.GetObj(); 
      m_link.flavor = (RelationID)m_flavor.Selection(); 
   }
}


BEGIN_MESSAGE_MAP(cAddLinkDialog, CDialog)
	//{{AFX_MSG_MAP(cAddLinkDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void cAddLinkDialog::FromLink(const sLink* link)
{
   m_link = *link; 
}

void cAddLinkDialog::ToLink(sLink* link)
{
   *link = m_link; 
}

/////////////////////////////////////////////////////////////////////////////
// cAddLinkDialog message handlers

BOOL cAddLinkDialog::OnInitDialog() 
{

   VERIFY(m_from.SubclassDlgItem(IDC_FROM,this)); 
   VERIFY(m_to.SubclassDlgItem(IDC_TO,this)); 
   VERIFY(m_flavor.SubclassDlgItem(IDC_FLAVOR,this)); 

   // fill the flavor list with every relation and its inverse 
   AutoAppIPtr_(LinkManager,pLinkMan); 
   sRelationIter iter; 
   RelationID relid; 
   pLinkMan->IterStart(&iter); 
   while (pLinkMan->IterNext(&iter,&relid))
   {
      IRelation* rel = pLinkMan->GetRelation(relid); 
      m_flavor.AddItemNamed(relid,rel->Describe()->name); 
      SafeRelease(rel); 

      relid = RELID_INVERT(relid); 
      rel = pLinkMan->GetRelation(relid); 
      m_flavor.AddItemNamed(relid,rel->Describe()->name); 
      SafeRelease(rel); 
   }
   pLinkMan->IterStop(&iter); 

	CDialog::OnInitDialog();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL cAddLinkDialog::DoDialog(sLink* link)
{
   CWnd* appwnd = CWnd::GetActiveWindow(); 
   cAddLinkDialog dlg(link,appwnd); 
   BOOL result = dlg.DoModal() == IDOK; 
   if (result)
      dlg.ToLink(link); 
   return result; 
}

////////////////////////////////////////////////////////////

extern "C" __declspec(dllexport) BOOL LGAPI DisplayLinkDialog(sLink& link)
{
   return cAddLinkDialog::DoDialog(&link); 
}
