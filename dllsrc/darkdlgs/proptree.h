// An object tree that shows each object's editable traits (properties,
// relations...) grouped by category.

#ifndef __PROPTREE_H
#define __PROPTREE_H

#include "objtree.h"

F_DECLARE_INTERFACE(ITraitManager);
F_DECLARE_INTERFACE(IObjEditors);
F_DECLARE_INTERFACE(IEditTrait);

class cHiddenList; 

/////////////////////////////////////////////////////////////////////////////
// cTraitTree window

class cTraitTree : public cObjTree
{
// Operations
public:
   void ExpandContainer(CString container, int code = TVE_EXPAND, const sNode* root = NULL); 
   void SortNodeChildren(const sNode* node); 
   void RebuildTraits(const sNode* objnode);
   void LoadImages(); 
   void ExpandObj(const sNode* node = NULL, BOOL recurse = TRUE); 
   const sNode* AddTrait(IEditTrait* trait, const sNode* parent); 

   BOOL EditNode(const sNode* node); 

// Construction
public:
	cTraitTree(ObjID obj = OBJ_NULL, cHiddenList* hidden = NULL);

// Implementation
public:
	virtual ~cTraitTree();

protected:
   void AddItems(const sNode* objnode); 
   IObjectQuery* Successors(ObjID obj); 
   const sNode* GetContainer(const sNode* parent, const char* name); 
   static int CALLBACK CompareFunc(LPARAM lparam1, LPARAM lparam2, LPARAM treeparm);
   BOOL Hidden(CString cat); 

   ITraitManager* m_pTraitMan; 
   IObjEditors* m_pEditors; 
   CImageList m_images; 
   cHiddenList* m_hidden; 

	// Generated message map functions
protected:
	//{{AFX_MSG(cTraitTree)
	afx_msg void OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#endif // __PROPTREE_H
