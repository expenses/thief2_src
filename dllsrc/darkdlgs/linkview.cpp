#include "stdafx.h"
#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <linkman.h>
#include <relation.h>
#include <linkbase.h>
#include <linkid.h>
#include <edittool.h>
#include <iobjed.h>
#include <objedbas.h>

#include "darkdlgs.h"
#include "linkview.h"
#include "addlink.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern "C" IObjEditor* LGAPI ConstructObjEditor(const sObjEditorDesc* eddesc, ObjID editme); 

ObjID gCurrentSrc = OBJ_NULL;
ObjID gCurrentDest = OBJ_NULL;
RelationID gCurrentFlavour = RELID_NULL; 

/////////////////////////////////////////////////////////////////////////////
// cLinkView dialog


cLinkView::cLinkView(const sLinkEditorDesc* desc, cQueryFactory* qfact, CWnd* pParent, UINT idd, cLinkList* list)
	: CDialog(idd, pParent),
     m_parent(pParent),
     m_desc(*desc),
     m_factory(qfact),
     m_pLinkMan(AppGetObj(ILinkManager)),
     m_list((list) ? *list : *new cLinkList),
     m_delete_list(list != NULL)
{
	//{{AFX_DATA_INIT(cLinkView)
	//}}AFX_DATA_INIT
}

cLinkView::~cLinkView()
{
   if (m_delete_list)
      delete &m_list; 
   delete m_factory; 
   SafeRelease(m_pLinkMan); 
}

void cLinkView::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(cLinkView)
	DDX_Control(pDX, IDC_LIST, m_list);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(cLinkView, CDialog)
	//{{AFX_MSG_MAP(cLinkView)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST, OnDblclkList)
	ON_BN_CLICKED(IDC_ADD, OnAdd)
	ON_BN_CLICKED(IDC_EDIT, OnEdit)
	ON_BN_CLICKED(IDC_DELETE, OnDelete)
	ON_BN_CLICKED(IDC_SOURCE, OnSource)
	ON_BN_CLICKED(IDC_DEST, OnDest)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// cLinkView message handlers

void cLinkView::OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult) 
{
   OnEdit(); 
	*pResult = 0;
}

void cLinkView::RecomputeList()
{
   m_list.DeleteAllItems(); 
   ILinkQuery* query = m_factory->Query(); 
   for (; !query->Done(); query->Next())
   {
      LinkID id = query->ID(); 
      IRelation* rel = m_pLinkMan->GetRelation(LINKID_RELATION(id)); 
      BOOL add = !(rel->Describe()->flags & kRelationNoEdit) || (m_desc.flags & kLinkEditShowAll); 
      if (add)
         m_list.AddLink(id); 
      SafeRelease(rel); 
   }
   SafeRelease(query); 
}

BOOL cLinkView::OnInitDialog() 
{
	CDialog::OnInitDialog();

   m_list.SetupColumns(); 
   RecomputeList(); 
   if (m_desc.title[0] != '\0')
      SetWindowText(m_desc.title); 
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void cLinkView::OnAdd() 
{
   sLink link = { 0, 0, 0}; 
   LinkID id = LINKID_NULL; 

   // start from the last selected link, if any
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

   if (cAddLinkDialog::DoDialog(&link))
   {
      m_pLinkMan->Add(link.source,link.dest,link.flavor); 
      RecomputeList(); 
   }
}

void cLinkView::OnEdit() 
{
   int n = m_list.GetItemCount(); 
   for (int i = 0; i < n; i++)
      if (m_list.GetItemState(i,LVIS_SELECTED))
         m_list.EditLink(m_list.GetItemData(i)); 
   RecomputeList(); 
}

void cLinkView::OnDelete() 
{
   if (MessageBox("Really delete these links?","Confirm",MB_YESNO) == IDYES)
   {
      int n = m_list.GetItemCount();
      for (int i = 0; i < n; i++)
         if (m_list.GetItemState(i,LVIS_SELECTED))
            DeleteLink(m_list.GetItemData(i));
      RecomputeList();
   } 
}

void cLinkView::DeleteLink(LinkID id)
{
   m_pLinkMan->Remove(id); 
}

BOOL cLinkView::Go()
{
   if (m_desc.flags & kLinkEditModeless)
   {
      Create(IDD,m_parent); 
      ShowWindow(SW_SHOWNORMAL); 
      UpdateWindow(); 
      return FALSE; 
   }
   return DoModal(); 
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

////////////////////////////////////////////////////////////

CString relation_name(RelationID id); 

extern "C" __declspec(dllexport) void LGAPI DoLinkEditor(const sLinkEditorDesc* desc, ObjID src, ObjID dest, RelationID flav)
{
   CWnd* appwnd = CWnd::FromHandle(GetActiveWindow()); 
   sLinkEditorDesc copydesc = *desc; 

   gCurrentSrc = src; 
   gCurrentDest = dest; 
   gCurrentFlavour = flav; 

   if (copydesc.title[0] == '\0')
   {
      AutoAppIPtr_(EditTools,pTools); 
      CString srcname = (src == OBJ_NULL) ? "Any" : pTools->ObjName(src); 
      CString destname = (dest == OBJ_NULL) ? "Any" : pTools->ObjName(dest); 
      CString flavname = relation_name(flav); 
      char buf[256]; 
      sprintf(buf,"Links: %s --[ %s ]--> %s",srcname,flavname,destname); 
      strncpy(copydesc.title,buf,sizeof(copydesc.title)); 
   }



   cLinkView::cQueryFactory* fact = new cStandardFactory(src,dest,flav); 
   cLinkView view(&copydesc,fact,appwnd); 
   view.Go(); 
}

CString relation_name(RelationID id)
{
   if (id == RELID_NULL)
      return "Any"; 

   AutoAppIPtr(LinkManager); 
   cAutoIPtr<IRelation> rel(pLinkManager->GetRelation(id)); 
   return rel->Describe()->name; 
}

void cLinkView::OnSource() 
{
   int n = m_list.GetItemCount(); 
   for (int i = 0; i < n; i++)
      if (m_list.GetItemState(i,LVIS_SELECTED))
      {
         LinkID id = m_list.GetItemData(i); 
         sLink link; 
         m_pLinkMan->Get(id,&link); 
         sObjEditorDesc desc = { "", 0 };
         IObjEditor* ed = ConstructObjEditor(&desc,link.source); 
         if (ed->Go(kObjEdModal))
            RecomputeList(); 
      }
}

void cLinkView::OnDest() 
{
   int n = m_list.GetItemCount(); 
   for (int i = 0; i < n; i++)
      if (m_list.GetItemState(i,LVIS_SELECTED))
      {
         LinkID id = m_list.GetItemData(i); 
         sLink link; 
         m_pLinkMan->Get(id,&link); 
         sObjEditorDesc desc = { "", 0 };
         IObjEditor* ed = ConstructObjEditor(&desc,link.dest); 
         if (ed->Go(kObjEdModal))
            RecomputeList(); 
      }
}
