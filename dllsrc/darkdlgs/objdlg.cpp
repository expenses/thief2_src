#include "stdafx.h"
#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <iobjed.h>
#include <edtrait.h>
#include <edtraist.h>
#include <traitman.h>
#include <traitbas.h>
#include <objquery.h>
#include <iobjsys.h>
#include <edittool.h>

#include "darkdlgs.h"
#include "objdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// menu ids for traits start here; metaproperties use their (negative) obj ids 
#define TRAIT_BASE_ID 0x100

/////////////////////////////////////////////////////////////////////////////
// cObjEditor::cTraitMenu

cObjEditor::cTraitMenu::cTraitMenu()
   : m_pEdit(AppGetObj(IObjEditors))
{
   m_container.LoadBitmap(IDB_CONTAINER); 
   m_obj.LoadBitmap(IDB_OBJ); 
   m_trait.LoadBitmap(IDB_DATA); 

   CreatePopupMenu(); 
   AddAllTraits(); 
   AddMetaProps(); 
}

cObjEditor::cTraitMenu::~cTraitMenu()
{
   SafeRelease(m_pEdit); 
   DestroySubMenus(); 
   DestroyMenu(); 
}

//
// Find or create the submenu for a category.  "Foo: Bar" is "Bar" inside "Foo"
//

CMenu* cObjEditor::cTraitMenu::GetContainer(const char* name, CMenu* root)
{
   if (root == NULL)
      root = this; 

   CString s = name; 
   char* delim = strchr(s,':'); 
   if (delim != NULL)
   {
      *delim = '\0'; 
      CMenu* newroot = GetContainer(s,root); 
      *delim = ':'; 
      for (delim++; isspace(*delim); delim++)
         ; 
      return GetContainer(delim,newroot); 
   }






   int i; 
   if (FindSortedMenuIndex(root,name,&i))
   {
      CMenu* submenu = root->GetSubMenu(i); 
      if (submenu != NULL)
         return submenu; 
   }

   CMenu* result = new CMenu; 
   result->CreatePopupMenu(); 
   root->InsertMenu(i,MF_BYPOSITION|MF_POPUP,(UINT)result->m_hMenu,name); 
   if (strlen(name) > 1)
      root->SetMenuItemBitmaps(i,MF_BYPOSITION,&m_container,&m_container); 
   return result; 
}

void cObjEditor::cTraitMenu::AddTrait(IEditTrait* trait, int idx)
{
   const sEditTraitDesc* desc = trait->Describe(); 
   if (desc->flags & kTraitInvisible)
      return; 
   const sEditTraitCaps* caps = trait->Capabilities(); 
   if (!(caps->flags & kTraitCanAdd))
      return; 

   CMenu* parent = this; 
   if (desc->strings.category[0] != '\0')
      parent = GetContainer(desc->strings.category); 
   else
   {
      // file it under its first letter 
      char buf[2]; 
      buf[0] = toupper(desc->strings.name[0]); 
      buf[1] = '\0'; 
      parent = GetContainer(buf); 
   }

   int i; 
   FindSortedMenuIndex(parent,desc->strings.friendly_name,&i); 
   parent->InsertMenu(i,MF_BYPOSITION,idx,desc->strings.friendly_name); 
   parent->SetMenuItemBitmaps(i,MF_BYPOSITION,&m_trait,&m_trait); 
}

void cObjEditor::cTraitMenu::AddAllTraits()
{
   sEditTraitIter iter; 
   for (IEditTrait* trait = m_pEdit->FirstTrait(&iter); trait != NULL; trait = m_pEdit->NextTrait(&iter))
   {
      AddTrait(trait,iter.idx + TRAIT_BASE_ID); 
      SafeRelease(trait); 
   }
}

void cObjEditor::cTraitMenu::DestroySubMenus()
{
   UINT n = GetMenuItemCount(); 
   for (int i = 0; i < n; i++)
   {
      CMenu* destroy = GetSubMenu(i); 
      if (destroy)
      {
         destroy->DestroyMenu(); 
         delete destroy; 
      }
   }
}

void cObjEditor::cTraitMenu::Go(CWnd* wnd, CPoint pos)
{
   TrackPopupMenu(TPM_CENTERALIGN,pos.x,pos.y,wnd); 
}

IEditTrait* cObjEditor::cTraitMenu::GetTrait(int idx)
{
   sEditTraitIter iter; 
   for (IEditTrait* trait = m_pEdit->FirstTrait(&iter); trait != NULL; trait = m_pEdit->NextTrait(&iter))
   {
      if (idx == iter.idx + TRAIT_BASE_ID)
         return trait; 
      SafeRelease(trait); 
   }
   return NULL; 
}

void cObjEditor::cTraitMenu::AddMetaProps()
{
   AutoAppIPtr_(TraitManager,pTraitMan); 
   AutoAppIPtr_(EditTools,pTools); 
   AutoAppIPtr_(ObjectSystem,pObjSys); 

   ObjID root = pTraitMan->RootMetaProperty(); 
   IObjectQuery* query = pTraitMan->Query(root,kTraitQueryAllDescendents); 
   for (; !query->Done(); query->Next())
   {
      ObjID obj = query->Object(); 
      if (pTraitMan->IsMetaProperty(obj))
      {

         // Build a container path from the archetype chain
         CString container = ""; 
         BOOL first = TRUE; 
         IObjectQuery* archq = pTraitMan->Query(obj,kTraitQueryAllArchetypes); 
         for (; !archq->Done(); archq->Next())
         {
            CString name = pObjSys->GetName(archq->Object()); 
            if (first)
               container = name; 
            else
               container = name + ":" + container; 
            first = FALSE; 
         }
         SafeRelease(archq); 

         CMenu* menu = GetContainer(container); 
         UINT idx; 
         const char* name = pTools->ObjName(obj); 
         FindSortedMenuIndex(menu,name,(int*)&idx); 
         int i = obj & 0xFFFF; 
         menu->InsertMenu(idx,MF_BYPOSITION,i,name); 
         menu->SetMenuItemBitmaps(idx,MF_BYPOSITION,&m_obj,&m_obj); 
      }
   }
   SafeRelease(query); 
}

BOOL cObjEditor::cTraitMenu::FindSortedMenuIndex(CMenu* menu, const char* name, int* idx)
{
   UINT n = menu->GetMenuItemCount(); 
   for (int i = 0; i < n; i++)
   {
      CString label; 
      menu->GetMenuString(i,label,MF_BYPOSITION); 
      int compare = label.CompareNoCase(name); 
      if (compare >= 0)
      {
         *idx = i; 
         return compare == 0; 
      }
   }
   *idx = i; 
   return FALSE; 
}

/////////////////////////////////////////////////////////////////////////////
// cObjEditor dialog


cObjEditor::cObjEditor(const sObjEditorDesc* desc, ObjID obj, CWnd* pParent /*=NULL*/)
	: CDialog(cObjEditor::IDD, pParent),
     m_obj(obj),
     m_desc(*desc),
     m_tree(obj,&gHiddenList)
{
	//{{AFX_DATA_INIT(cObjEditor)
	//}}AFX_DATA_INIT
}

void cObjEditor::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(cObjEditor)
	DDX_Control(pDX, IDC_HIDDEN, m_hidden);
	DDX_Control(pDX, IDC_TREE, m_tree);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(cObjEditor, CDialog)
	//{{AFX_MSG_MAP(cObjEditor)
	ON_BN_CLICKED(IDC_ADD, OnAdd)
	ON_BN_CLICKED(IDC_DELETE, OnDelete)
	ON_BN_CLICKED(IDC_EDIT, OnEdit)
	ON_BN_CLICKED(IDC_EXPAND, OnExpand)
	ON_WM_CREATE()
	ON_BN_CLICKED(IDC_REFRESH, OnRefresh)
	ON_BN_CLICKED(IDC_HIDE, OnHide)
	ON_LBN_DBLCLK(IDC_HIDDEN, OnDblclkHidden)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE, OnSelchangedTree)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// cObjEditor message handlers

void cObjEditor::OnAdd() 
{
   RECT rect;
   CWnd* butt = GetDlgItem(IDC_ADD); 
   butt->GetWindowRect(&rect); 
   CPoint pos((rect.right+rect.left)/2,(rect.bottom+rect.top)/2); 
   m_menu.Go(this,pos); 
}

void cObjEditor::OnDelete() 
{
   const sNode* node = m_tree.Selection(); 
   if (node == NULL)
      return; 

   switch (node->type)
   {
      case cObjTree::kData:
      {
         IEditTrait* trait = (IEditTrait*)node->data; 
         if (trait->Capabilities()->flags & kTraitCanRemove)
         {
            trait->Remove(m_tree.GetNodeObj(node)); 
            m_tree.RemoveNode(node); 
         }
      }
      break; 

      case cObjTree::kObj:
      {
         // remove a metaproperty 
         ObjID obj = node->data; 
         if (obj == m_obj)
            return; 
         const sNode* parent = m_tree.GetNode(m_tree.GetParentItem(node->item)); 
         ObjID pobj = m_tree.GetNodeObj(parent); 
         AutoAppIPtr_(TraitManager,pTraitMan); 
         // can't remove the archetype 
         if (pTraitMan->GetArchetype(pobj) == obj)
            return; 
         pTraitMan->RemoveObjMetaProperty(pobj,obj); 
         m_tree.RemoveNode(node); 
      }
      break; 
   }
}

void cObjEditor::OnEdit() 
{
   m_tree.EditNode(m_tree.Selection()); 
}

BOOL cObjEditor::OnInitDialog() 
{
	CDialog::OnInitDialog();

   if (m_desc.title[0] != '\0')
      SetWindowText(m_desc.title); 
   else
   {
      AutoAppIPtr_(EditTools,pTools); 
      SetWindowText(pTools->ObjName(m_obj)); 
   }

   FillHiddenList(); 
   m_tree.LoadImages(); 
   m_tree.BuildTree(); 
   m_tree.ExpandObj(NULL,FALSE); 
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

////////////////////////////////////////////////////////////

extern "C" __declspec(dllexport) void LGAPI DoObjEdit(const sObjEditorDesc* desc, ObjID obj)
{
   CWnd* appwnd = CWnd::FromHandle(GetActiveWindow()); 
   cObjEditor dlg(desc,obj,appwnd); 
   dlg.DoModal(); 
}

////////////////////////////////////////////////////////////

BOOL cObjEditor::OnCommand(WPARAM wParam, LPARAM lParam) 
{
   // menu command 
   if (lParam == 0 && HIWORD(wParam) == 0 && LOWORD(wParam) >= TRAIT_BASE_ID)
      DoMenuCommand(LOWORD(wParam)); 
	
	return CDialog::OnCommand(wParam, lParam);
}

void cObjEditor::OnExpand() 
{
   m_tree.ExpandObj(NULL,TRUE); 
}

void cObjEditor::DoMenuCommand(UINT uitem)
{
   int item = (short)uitem; 
   ObjID obj = m_tree.GetNodeObj(m_tree.Selection()); 

   if (item >= 0)
   {
      IEditTrait* trait = m_menu.GetTrait(item);
      if (trait != NULL)
      {
         BOOL added = TRUE;
         const sEditTraitCaps* caps = trait->Capabilities();
         trait->Add(obj);
         if (caps->flags & kTraitCanEdit)
         {
            long result = trait->Edit(obj);
            if (result != S_OK && (caps->flags & kTraitCanRemove))
            {
               trait->Remove(obj);
               added = FALSE;
            }
         }
         if (added)
         {
            const sNode* objnode = m_tree.GetObjNode(obj);
            const sNode* tnode = m_tree.AddTrait(trait,objnode);
            if (tnode)
               m_tree.SelectItem(tnode->item);
         }
         SafeRelease(trait);
      }
   }
   else
   {
      AutoAppIPtr_(TraitManager,pTraitMan);
      pTraitMan->AddObjMetaProperty(obj,item);
      m_tree.BuildTree();
      const sNode* node = m_tree.GetObjNode(item);
      if (node)
         m_tree.EnsureVisible(node->item);
   } 
}

void cObjEditor::OnOK() 
{
	CDialog::OnOK();
}

void cObjEditor::OnCancel() 
{
	CDialog::OnCancel();
}

int cObjEditor::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CDialog::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	return 0;
}

void cObjEditor::OnRefresh() 
{
   ObjID obj = m_tree.GetNodeObj(m_tree.Selection()); 
   m_tree.BuildTree(); 
   const sNode* objnode = m_tree.GetObjNode(obj); 
   m_tree.EnsureVisible(objnode->item); 
   m_tree.ExpandObj(objnode,FALSE); 
}

cHiddenList cObjEditor::gHiddenList; 

void cObjEditor::FillHiddenList()
{
   m_hidden.ResetContent(); 
   for (int i = 0; i < gHiddenList.Size(); i++)
      m_hidden.AddString(gHiddenList[i]); 
}

void cObjEditor::OnHide() 
{
   const sNode* node = m_tree.Selection(); 
   if (node->type == cObjTree::kContainer)
   {
      CString str = m_tree.GetItemText(node->item); 
      gHiddenList.Hide(str); 
      m_hidden.AddString(str); 
      m_tree.ExpandContainer(str,TVE_COLLAPSE); 
   }
}

void cObjEditor::OnDblclkHidden() 
{
   int i = m_hidden.GetCurSel(); 
   CString str; 
   m_hidden.GetText(i,str); 
   gHiddenList.Unhide(str); 
   m_tree.ExpandContainer(str,TVE_EXPAND); 
   m_hidden.DeleteString(i); 
}

void cObjEditor::OnSelchangedTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
   const sNode* node = m_tree.Selection(); 
   GetDlgItem(IDC_HIDE)->EnableWindow(node->type == cObjTree::kContainer); 
}
