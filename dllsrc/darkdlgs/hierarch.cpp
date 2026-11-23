#include "stdafx.h"
#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <traitman.h>
#include <traitbas.h>
#include <iobjsys.h>
#include <objdef.h>
#include <objquery.h>
#include <edittool.h>
#include <iobjed.h>
#include <objedbas.h>
#include <propman.h>
#include <property.h>

#include "darkdlgs.h"
#include "hierarch.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// cHierarchyDialog::cTree 

cHierarchyDialog::cTree::cTree()
   : m_pTraitMan(AppGetObj(ITraitManager))
{
}

cHierarchyDialog::cTree::~cTree()
{
   SafeRelease(m_pTraitMan); 
}

IObjectQuery* cHierarchyDialog::cTree::Successors(ObjID obj)
{
   return m_pTraitMan->Query(obj,kTraitQueryDescendents); 
}

BOOL cHierarchyDialog::cTree::FilterObj(ObjID obj)
{
   return obj < 0 || (m_flags & kObjTreePermitConcretes); 
}

void cHierarchyDialog::cTree::OnDrop(const sNode* node, const sNode* target)
{
   ObjID srcobj = GetNodeObj(node); 
   ObjID targobj = GetNodeObj(target); 
   m_pTraitMan->SetArchetype(srcobj,targobj); 
   RemoveNode(node); 
   BuildSubtree(target); 
}

BOOL cHierarchyDialog::cTree::EditNode(const sNode* node)
{
   if (node->type == kObj)
   {
      static sObjEditorDesc desc;
      AutoAppIPtr_(ObjEditors,pEdit);
      IObjEditor* ed = pEdit->Create(&desc,node->data);
      BOOL retval = ed->Go(kObjEdModal);
      SafeRelease(ed);
      return retval;
   }
   return TRUE; 
}

/////////////////////////////////////////////////////////////////////////////
// cHierarchyDialog dialog


cHierarchyDialog::cHierarchyDialog(const sObjTreeEditorDesc* desc, ObjID focus, CWnd* pParent /*=NULL*/)
	: CDialog(cHierarchyDialog::IDD, pParent),
     m_focus(focus),
     m_desc(*desc),
     m_pTools(AppGetObj(IEditTools)),
     m_pObjSys(AppGetObj(IObjectSystem))
{
	//{{AFX_DATA_INIT(cHierarchyDialog)
	//}}AFX_DATA_INIT

   // copy the tree list, including the terminator
   for (int n = 0; desc->trees[n].treename[0] != '\0'; n++)
      ;

   m_ntrees = n; 
   m_desc.trees = new sObjTreeDesc[n+1]; 
   memcpy(m_desc.trees,desc->trees,sizeof(sObjTreeDesc)*(n+1)); 


   m_rootids = new ObjID[n]; 
}

cHierarchyDialog::~cHierarchyDialog()
{
   delete [] m_desc.trees; 
   delete [] m_rootids; 
   SafeRelease(m_pTools); 
   SafeRelease(m_pObjSys); 
}


void cHierarchyDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(cHierarchyDialog)
	DDX_Control(pDX, IDC_TREELIST, m_treelist);
	DDX_Control(pDX, IDC_TREE, m_tree);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(cHierarchyDialog, CDialog)
	//{{AFX_MSG_MAP(cHierarchyDialog)
	ON_BN_CLICKED(IDC_ADDNODE, OnAddnode)
	ON_BN_CLICKED(IDC_CREATE, OnCreate)
	ON_BN_CLICKED(IDC_DELNODE, OnDelnode)
	ON_CBN_SELCHANGE(IDC_TREELIST, OnSelchangeTreelist)
	ON_BN_CLICKED(IDC_EDIT, OnEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// cHierarchyDialog message handlers

BOOL cHierarchyDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();

   if (m_desc.title[0] != '\0')
      SetWindowText(m_desc.title); 

   for (int i = 0; i < m_ntrees; i++)
   {
      sObjTreeDesc& tree = m_desc.trees[i]; 
      m_rootids[i] = m_pObjSys->GetObjectNamed(tree.rootname.text); 
      m_treelist.AddItemNamed(i,tree.treename); 
   }
   m_treelist.SelectItem(0); 
   m_treelist.UpdateData(FALSE); 
   OnSelchangeTreelist(); 

   if (m_focus != OBJ_NULL)
   {
      const cObjTree::sNode* node = m_tree.GetObjNode(m_focus); 
      if (node)
         m_tree.SelectItem(node->item); 
   }
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void cHierarchyDialog::OnAddnode() 
{
   char buf[2048] = ""; 
   if (m_pTools->EditTypedData("New Object Name","sBuf",buf) == S_OK)
   {
      ObjID parent = m_tree.GetNodeObj(m_tree.Selection()); 
      eObjConcreteness conc = (parent > 0) ? kObjectConcrete : kObjectAbstract; 
      ObjID obj = m_pObjSys->BeginCreate(parent,conc); 

      ulong result = kObjTreeNormal; 
      if (CurTree().func.add)
         result = CurTree().func.add(obj,buf); 

      if (result & kObjTreeCancel)
      {
         m_pObjSys->EndCreate(obj); 
         m_pObjSys->Destroy(obj); 
      }
      else
      {
         if (result & kObjTreeNormal)
         {
            m_pObjSys->NameObject(obj,buf); 

            // inherit donor type from the parent 
            AutoAppIPtr_(PropertyManager,PropMan); 
            cAutoIPtr<IProperty> prop(PropMan->GetPropertyNamed("DonorType")); 
            if (prop->IsSimplyRelevant(parent))
               prop->Copy(obj,parent); 
         }
         m_pObjSys->EndCreate(obj); 

         const cObjTree::sNode* node = m_tree.AddObj(obj,m_tree.Selection()); 
         m_tree.EnsureVisible(node->item); 
      }
      DoCBResult(result); 
   }
}

void cHierarchyDialog::OnCreate() 
{
   ObjID parent = m_tree.GetNodeObj(m_tree.Selection()); 
   ulong result = kObjTreeNormal; 
   if (CurTree().func.create)
      result = CurTree().func.create(parent); 
   DoCBResult(result); 
}

void cHierarchyDialog::OnDelnode() 
{
   const cObjTree::sNode* node = m_tree.Selection(); 
   ObjID obj = m_tree.GetNodeObj(node); 
   const cObjTree::sNode* parent = m_tree.GetNode(m_tree.GetParentItem(node->item)); 

   ulong result = kObjTreeNormal; 
   if (CurTree().func.del)
      result = CurTree().func.del(obj); 

   ObjID parentobj = m_tree.GetNodeObj(parent); 
   if (result & kObjTreeNormal)
   {
      // re-parent the children 
      AutoAppIPtr_(TraitManager,TraitMan); 
      IObjectQuery* query = TraitMan->Query(obj,kTraitQueryDescendents); 
      for (; !query->Done(); query->Next())
         TraitMan->SetArchetype(query->Object(),parentobj); 
      SafeRelease(query); 

      m_pObjSys->Destroy(obj); 
      if (parent)
         m_tree.BuildSubtree(parent); 
   }
   DoCBResult(result); 

   parent = m_tree.GetObjNode(parentobj); 
   if (parent)
      m_tree.EnsureVisible(parent->item); 
}

void cHierarchyDialog::OnSelchangeTreelist() 
{
   const sObjTreeDesc& tree = CurTree(); 
   m_tree.SetRootObj(m_rootids[m_treelist.Selection()]); 
   m_flags = m_desc.flags | tree.flags; 
   m_tree.SetFlags(m_flags); 
   m_tree.BuildTree(); 
   EnableButtons(); 
}

const sObjTreeDesc& cHierarchyDialog::CurTree()
{
   return m_desc.trees[m_treelist.Selection()]; 
}

void cHierarchyDialog::DoCBResult(ulong result)
{
   if (result & kObjTreeQuit)
      OnCancel(); 
   else if (result & kObjTreeChanged)
      m_tree.BuildTree(); 
}

struct sButtFlag
{
   int control; 
   ulong flag; 
}; 

static sButtFlag ButtFlags[] = 
{
   { IDC_ADDNODE, kObjTreeNoAddButton }, 
   { IDC_DELNODE, kObjTreeNoDelButton }, 
   { IDC_CREATE, kObjTreeNoCreateButton },
}; 

#define NUM_BUTTS (sizeof(ButtFlags)/sizeof(ButtFlags[0]))

void cHierarchyDialog::EnableButtons()
{
   for (int i = 0; i < NUM_BUTTS; i++)
   {
      const sButtFlag& butt = ButtFlags[i]; 
      CWnd* ctrl = GetDlgItem(butt.control); 
      int show = (m_flags & butt.flag) ? SW_HIDE : SW_SHOW; 
      ctrl->ShowWindow(show); 
   }
}

void cHierarchyDialog::OnEdit() 
{
   m_tree.EditNode(m_tree.Selection()); 
}

////////////////////////////////////////////////////////////

extern "C" __declspec(dllexport) void LGAPI DoHierarchy(const sObjTreeEditorDesc* desc, ObjID obj)
{
   CWnd* appwnd = CWnd::FromHandle(GetActiveWindow()); 
   cHierarchyDialog dlg(desc,obj,appwnd); 
   dlg.DoModal(); 
}
