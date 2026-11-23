// A tree control that shows a hierarchy of objects, plus arbitrary
// non-object "container" and "data" nodes. 

#ifndef __OBJTREE_H
#define __OBJTREE_H

#include <objtype.h>

F_DECLARE_INTERFACE(IEditTools);
F_DECLARE_INTERFACE(IObjectQuery);

/////////////////////////////////////////////////////////////////////////////
// cObjTree window

class cObjTree : public CTreeCtrl
{
public:
   enum eNodeType
   {
      kNull,
      kObj,
      kContainer,
      kData,
   }; 

   struct sNode
   {
      eNodeType type; 
      long data; 
      HTREEITEM item; 

      sNode(HTREEITEM i, eNodeType t, long d) : type(t), data(d), item(i) {}; 
   }; 

   enum eDropType
   {
      kDropDisallow,
      kDropOK, 
      kDropCopy, 
   }; 

// Attributes
public:
   const sNode* Selection(); 
   const sNode* GetNode(HTREEITEM item); 
   const sNode* GetObjNode(ObjID obj); 
   ObjID GetNodeObj(const sNode* node); 

   // Edit a node, return whether it changed
   virtual BOOL EditNode(const sNode* node); 

   void SetRootObj(ObjID obj) { m_rootobj = obj; }; 
   ObjID GetRootObj() { return m_rootobj; }; 

// Operations
public:
   void BuildTree(); 
   void BuildSubtree(const sNode* node); 
   const sNode* AddNode(const sNode* parent, const char* name, eNodeType kind, long data); 
   const sNode* AddObj(ObjID obj, const sNode* parent); 
   void RemoveNode(const sNode* node); 

// Construction
public:
	cObjTree(ObjID root = OBJ_NULL);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(cObjTree)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~cObjTree();

protected:
   // Query for the tree-children of an object
   virtual IObjectQuery* Successors(ObjID obj);
   // Should this object be shown?
   virtual BOOL FilterObj(ObjID obj); 
   // Add non-object children to an obj node
   virtual void AddItems(const sNode* objnode); 

   // drag & drop 
   virtual BOOL CanBeDragged(const sNode* node); 
   virtual eDropType CanBeDropped(const sNode* node, const sNode* targ); 
   virtual void OnDrop(const sNode* node, const sNode* target); 

   const sNode* ObjSearch(ObjID obj, const sNode* node); 

   BOOL m_dragging; 
   const sNode* m_dragnode; 
   IEditTools* m_pEditTools; 
   ObjID m_rootobj; 

	// Generated message map functions
protected:
	//{{AFX_MSG(cObjTree)
	afx_msg void OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBegindrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnDblclk(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#endif // __OBJTREE_H
