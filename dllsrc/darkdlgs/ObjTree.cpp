#include "stdafx.h"
#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <edittool.h>
#include <objquery.h>

#include "darkdlgs.h"
#include "objtree.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// cObjTree

cObjTree::cObjTree(ObjID root)
   : m_dragnode(NULL),
     m_pEditTools(AppGetObj(IEditTools)),
     m_rootobj(root)
{
}

cObjTree::~cObjTree()
{
   SafeRelease(m_pEditTools); 
}

BEGIN_MESSAGE_MAP(cObjTree, CTreeCtrl)
	//{{AFX_MSG_MAP(cObjTree)
	ON_NOTIFY_REFLECT(TVN_DELETEITEM, OnDeleteitem)
	ON_NOTIFY_REFLECT(TVN_BEGINDRAG, OnBegindrag)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnDblclk)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// cObjTree message handlers

IObjectQuery* cObjTree::Successors(ObjID obj)
{
   return NULL; 
}

const cObjTree::sNode* cObjTree::AddNode(const sNode* parent, const char* name, eNodeType kind, long data)
{
   HTREEITEM parentitem = (parent) ? parent->item : GetRootItem(); 
   HTREEITEM after = TVI_LAST; 

   // Non-object nodes go first, alphabetically 
   if (kind != kObj)
   {
      after = TVI_FIRST; 
      HTREEITEM child = GetChildItem(parentitem); 
      HTREEITEM next = NULL; 
      for (; child != NULL; child = next)
      {
         next = GetNextSiblingItem(child); 
         const sNode* node = GetNode(child); 
         if (node->type == kObj || strcmp(name,GetItemText(child)) < 0)
         {
            after = GetPrevSiblingItem(child); 
            if (after == NULL)
               after = TVI_FIRST; 
            break; 
         }
      }
   }



   HTREEITEM item = InsertItem(name,parentitem,after); 
   sNode* node = new sNode(item,kind,data); 
   SetItemData(item,(DWORD)node); 
   SetItemImage(item,kind,kind); 
   return node; 
}

void cObjTree::RemoveNode(const sNode* node)
{
   if (node)
      DeleteItem(node->item); 
}

const cObjTree::sNode* cObjTree::GetObjNode(ObjID obj)
{
   return ObjSearch(obj,NULL); 
}

const cObjTree::sNode* cObjTree::GetNode(HTREEITEM item)
{
   return (item == NULL) ? NULL : (const sNode*)GetItemData(item); 
}

const cObjTree::sNode* cObjTree::ObjSearch(ObjID obj, const sNode* node)
{
   if (node == NULL)
   {
      node = GetNode(GetRootItem()); 
      if (node->type == kObj && node->data == obj)
         return node; 
   }

   for (HTREEITEM item = GetChildItem(node->item); item != NULL; item = GetNextSiblingItem(item))
   {
      const sNode* child = GetNode(item); 
      if (child->type != kObj)
         continue; 
      if (child->data == obj)
         return child; 
      child = ObjSearch(obj,child); 
      if (child)
         return child; 
   }
   return NULL; 
}

void cObjTree::AddItems(const sNode* objnode)
{
}

void cObjTree::BuildTree()
{
   DeleteAllItems(); 
   const sNode* rootnode = AddObj(m_rootobj,NULL); 
   BuildSubtree(rootnode); 
   Expand(rootnode->item,TVE_EXPAND); 
   Select(rootnode->item,TVGN_CARET); 
}

const cObjTree::sNode* cObjTree::AddObj(ObjID obj, const sNode* parent)
{
   return AddNode(parent,m_pEditTools->ObjName(obj),kObj,obj); 
}

void cObjTree::BuildSubtree(const sNode* node)
{
   if (node->type == kObj)
   {
   // nuke the old children 
   if (ItemHasChildren(node->item))
   {
      HTREEITEM child = GetChildItem(node->item);
      while (child != NULL)
      {
         HTREEITEM deleteme = child; 
         child = GetNextSiblingItem(child); 
         DeleteItem(deleteme); 
      }
   }

   AddItems(node); 

   ObjID obj = node->data; 
   IObjectQuery* query = Successors(obj); 
   if (query == NULL)
      return;
   for (; !query->Done(); query->Next())
   {
      ObjID childobj = query->Object(); 
      if (!FilterObj(childobj))
         continue;
      const sNode* child = AddObj(childobj,node);
      BuildSubtree(child);
   }
   SafeRelease(query);
   }
}

void cObjTree::OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
   sNode* node = (sNode*)GetItemData(pNMTreeView->itemOld.hItem); 
   delete node; 
	
	*pResult = 0;
}

BOOL cObjTree::FilterObj(ObjID obj)
{
   return TRUE; 
}

const cObjTree::sNode* cObjTree::Selection()
{
   return GetNode(GetSelectedItem()); 
}

ObjID cObjTree::GetNodeObj(const sNode* node)
{
   if (node == NULL)
      return OBJ_NULL; 

   while (node->type != kObj)
   {
      HTREEITEM item = GetParentItem(node->item); 
      if (item == NULL)
         return OBJ_NULL; 
      node = GetNode(item); 
   }
   return node->data; 
}

BOOL cObjTree::CanBeDragged(const sNode* node)
{
   // can't drag the root
   if (node->type == kObj && node->data == m_rootobj)
      return FALSE;
   return TRUE; 
}

cObjTree::eDropType cObjTree::CanBeDropped(const sNode* node, const sNode* targ)
{
   if (GetNodeObj(node) == GetNodeObj(targ))
      return kDropDisallow; 

   // can't drop an object onto its own parent 
   if (node->type == kObj)
   {
      const sNode* parent = GetNode(GetParentItem(node->item)); 
      if (GetNodeObj(parent) == GetNodeObj(targ))
         return kDropDisallow; 
   }
   return kDropOK; 
}

void cObjTree::OnDrop(const sNode* node, const sNode* target)
{
}

void cObjTree::OnBegindrag(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
   sNode* node = (sNode*)GetItemData(pNMTreeView->itemNew.hItem); 
   if (CanBeDragged(node))
   {
      m_dragnode = node; 
      SetCursor(LoadCursor(NULL,IDC_CROSS)); 
   }
   else
   {
      SetCursor(LoadCursor(NULL,IDC_NO)); 
      m_dragnode = NULL; 
   }
   SetCapture(); 
   m_dragging = TRUE; 
	
	*pResult = 0;
}

static LPCTSTR drop_cursors[] = 
{
   IDC_NO,     // kDropDisallow
   IDC_CROSS,  // kDropOK
   IDC_CROSS,  // kDropCopy
}; 

void cObjTree::OnMouseMove(UINT nFlags, CPoint point) 
{
   if (m_dragnode)
   {
      UINT flags; 
      HTREEITEM item = HitTest(point,&flags); 
      if (item != NULL)
      {
         const sNode* node = GetNode(item); 
         eDropType drop = CanBeDropped(m_dragnode,node); 
         SetCursor(LoadCursor(NULL,drop_cursors[drop])); 
         SelectDropTarget((drop != kDropDisallow) ? item : NULL); 
      }
   }
	
	CTreeCtrl::OnMouseMove(nFlags, point);
}

void cObjTree::OnLButtonUp(UINT nFlags, CPoint point) 
{
   if (m_dragnode)
   {
      HTREEITEM targitem = GetDropHilightItem(); 
      if (targitem != NULL)
      {
         const sNode* targ = GetNode(targitem); 
         OnDrop(m_dragnode,targ); 
         SelectDropTarget(NULL); 
      }
   }

   if (m_dragging)
   {
      SetCursor(LoadCursor(NULL,IDC_ARROW)); 
      ReleaseCapture(); 
      m_dragging = FALSE; 
      m_dragnode = NULL; 
   }
	
	CTreeCtrl::OnLButtonUp(nFlags, point);
}

BOOL cObjTree::EditNode(const sNode* node)
{
   return TRUE; 
}

void cObjTree::OnDblclk(NMHDR* pNMHDR, LRESULT* pResult) 
{
   EditNode(Selection()); 
	*pResult = 0;
}
