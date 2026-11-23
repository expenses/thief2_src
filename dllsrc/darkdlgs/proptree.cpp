#include "stdafx.h"
#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <traitman.h>
#include <traitbas.h>
#include <iobjed.h>
#include <edtrait.h>
#include <edtraist.h>
#include <objedbas.h>
#include <edittool.h>
#include <iobjsys.h>

#include "darkdlgs.h"
#include "proptree.h"
#include "hidden.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// cTraitTree

cTraitTree::cTraitTree(ObjID obj, cHiddenList* hidden)
   : cObjTree(obj),
     m_pTraitMan(AppGetObj(ITraitManager)),
     m_pEditors(AppGetObj(IObjEditors)),
     m_hidden(hidden)
{
}

cTraitTree::~cTraitTree()
{
   m_images.DeleteImageList(); 
   SafeRelease(m_pTraitMan); 
   SafeRelease(m_pEditors); 
}

BEGIN_MESSAGE_MAP(cTraitTree, cObjTree)
	//{{AFX_MSG_MAP(cTraitTree)
	ON_NOTIFY_REFLECT(TVN_DELETEITEM, OnDeleteitem)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// cTraitTree message handlers

IObjectQuery* cTraitTree::Successors(ObjID obj)
{
   return m_pTraitMan->Query(obj,kTraitQueryDonors); 
}

static CString trait_item_text(ObjID obj, IEditTrait* trait)
{
   const sEditTraitDesc* desc = trait->Describe(); 
   CString result = desc->strings.friendly_name; 
   const sEditTraitCaps* caps = trait->Capabilities(); 
   if (caps->flags & kTraitCanUnparse)
   {
      result += ": "; 
      char buf[80]; 
      trait->Unparse(obj,buf,sizeof(buf)); 
      result += buf; 
   }
   return result; 
}

void cTraitTree::AddItems(const sNode* objnode)
{
   RebuildTraits(objnode); 
}

//
// Find or create the container node for a category name.
// "Foo: Bar" means container "Bar" inside container "Foo"
//

const cObjTree::sNode* cTraitTree::GetContainer(const sNode* parent, const char* name)
{
   CString s = name; 
   char* delim = strchr(s,':'); 
   if (delim != NULL)
   {
      *delim = '\0'; 
      const sNode* newroot = GetContainer(parent,s); 
      *delim = ':'; 
      for (delim++; isspace(*delim); delim++)
         ; 
      return GetContainer(newroot,delim); 
   }

   for (HTREEITEM child = GetChildItem(parent->item); child != NULL; child = GetNextSiblingItem(child))
   {
      const sNode* node = GetNode(child); 
      if (node->type == kContainer)
      {
         CString itemtext = GetItemText(child); 
         if (itemtext.CompareNoCase(name) == 0)
            return node; 
      }
   }

   const sNode* node = AddNode(parent,name,kContainer,0); 
   Expand(node->item,TVE_EXPAND); 
   return node; 
}

void cTraitTree::OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
   sNode* node = (sNode*)GetItemData(pNMTreeView->itemOld.hItem); 
   if (node->type == kData)
   {
      IEditTrait* trait = (IEditTrait*)node->data; 
      SafeRelease(trait); 
   }
   cObjTree::OnDeleteitem(pNMHDR,pResult); 
}

static CString trait_item_text(ObjID obj, IEditTrait* trait); 

BOOL cTraitTree::EditNode(const sNode* node)
{
   if (node->type == kData)
   {
      IEditTrait* trait = (IEditTrait*)node->data; 
      if (trait->Capabilities()->flags & kTraitCanEdit)
      {
         ObjID obj = GetNodeObj(node); 
         if (trait->Edit(obj) == S_OK)
         {
            CString text = trait_item_text(obj,trait); 
            SetItemText(node->item,text); 
            return TRUE; 
         }
         return FALSE; 
      }
   }
   else if (node->type == kObj)
   {
      // rename the object 
      ObjID obj = node->data; 
      const char* def_name = ""; 
      AutoAppIPtr_(ObjectSystem,pObjSys); 
      const char* name = pObjSys->GetName(obj); 
      if (name != NULL)
         def_name = name; 

      char buf[2048]; 
      strncpy(buf,def_name,sizeof(buf)); 
      buf[sizeof(buf)-1] = '\0'; 

      AutoAppIPtr_(EditTools,pTools); 
      if (pTools->EditTypedData("Object Name","sBuf",buf) == S_OK)
      {
         pObjSys->NameObject(obj,buf); 
         SetItem(node->item,TVIF_TEXT,pTools->ObjName(obj),0,0,0,0,0); 
         return TRUE; 
      }
      return FALSE; 
   }
   return TRUE; 
}

const cObjTree::sNode* cTraitTree::AddTrait(IEditTrait* trait, const sNode* parent)
{
   // find the object node 
   while (parent != NULL && parent->type != kObj)
   {
      HTREEITEM item = GetParentItem(parent->item); 
      parent = GetNode(item); 
   }
   if (parent == NULL)
      return NULL; 

   ObjID obj = parent->data; 
   const sEditTraitDesc* desc = trait->Describe(); 
   if (trait->IntrinsicTo(obj) && !(desc->flags & kTraitInvisible))
   {
      if (desc->strings.category[0] != '\0')
         parent = GetContainer(parent,desc->strings.category);
      CString name = trait_item_text(obj,trait);
      const sNode* result = AddNode(parent,name,kData,(long)trait);
      trait->AddRef(); // node holds a reference
      return result;
   }
   return NULL; 
}

void cTraitTree::ExpandObj(const sNode* node, BOOL recurse)
{
   if (node == NULL)
      node = GetNode(GetRootItem()); 

   Expand(node->item,TVE_EXPAND); 
   for (HTREEITEM child = GetChildItem(node->item); child != NULL; child = GetNextSiblingItem(child))
   {
      const sNode* cnode = GetNode(child); 
      switch (cnode->type)
      {
         case kContainer:
            if (!Hidden(GetItemText(child)))
               Expand(child,TVE_EXPAND);
            break;
         case kObj:
            if (recurse)
            {
               Expand(child,TVE_EXPAND);
               ExpandObj(cnode,TRUE);
            }
            break;
      }
   }
}

BOOL cTraitTree::Hidden(CString cat)
{
   return m_hidden != NULL && m_hidden->Hidden(cat); 
}

// images, indexed by node type 
static UINT bitmaps[] = 
{
   IDB_CONTAINER,  // kNull
   IDB_OBJ,        // kObj
   IDB_CONTAINER,  // kContainer
   IDB_DATA,       // kData
}; 

#define NUM_IMAGES (sizeof(bitmaps)/sizeof(bitmaps[0]))

void cTraitTree::LoadImages()
{
   m_images.Create(16,16,ILC_MASK,NUM_IMAGES,NUM_IMAGES); 
   for (int i = 0; i < NUM_IMAGES; i++)
   {
      CBitmap image; 
      image.LoadBitmap(bitmaps[i]); 
      m_images.Add(&image,RGB(255,255,255)); 
   }
   SetImageList(&m_images,TVSIL_NORMAL); 
}

void cTraitTree::RebuildTraits(const sNode* objnode)
{
   while (objnode->type != kObj)
      objnode = GetNode(GetParentItem(objnode->item)); 

   // delete all the non-object children
   HTREEITEM child = GetChildItem(objnode->item); 
   HTREEITEM next = NULL; 
   for (; child != NULL; child = next)
   {
      next = GetNextSiblingItem(child); 
      const sNode* node = GetNode(child); 
      if (node->type != kObj)
         DeleteItem(node->item); 
   }

   ObjID obj = objnode->data; 
   sEditTraitIter iter; 
   for (IEditTrait* trait = m_pEditors->FirstTrait(&iter); trait != NULL; trait = m_pEditors->NextTrait(&iter))
   {
      AddTrait(trait,objnode); 
      SafeRelease(trait); 
   }
}

int CALLBACK cTraitTree::CompareFunc(LPARAM lparam1, LPARAM lparam2, LPARAM treeparm)
{
   const sNode* node1 = (const sNode*)lparam1; 
   const sNode* node2 = (const sNode*)lparam2; 
   cTraitTree* tree = (cTraitTree*)treeparm; 

   // objects go last 
   if (node1->type != node2->type)
   {
      if (node1->type == kObj)
         return 1; 
      if (node2->type == kObj)
         return -1; 
   }
   if (node1->type == kObj)
      return 0; 

   return strcmp(tree->GetItemText(node1->item),tree->GetItemText(node2->item)); 
}

void cTraitTree::SortNodeChildren(const sNode* node)
{
   TVSORTCB sort = { (node) ? node->item : NULL, CompareFunc, (LPARAM)this }; 
   SortChildrenCB(&sort); 
}

void cTraitTree::ExpandContainer(CString container, int code, const sNode* root)
{
   HTREEITEM rooti = (root) ? root->item : GetRootItem(); 
   for (HTREEITEM child = GetChildItem(rooti); child != NULL; child = GetNextSiblingItem(child))
   {
      const sNode* node = GetNode(child); 
      if (node->type == kContainer)
      {
         if (container.CompareNoCase(GetItemText(child)) == 0)
            Expand(child,code); 
      }
      if (node->type == kObj)
         ExpandContainer(container,code,node); 
   }
}
