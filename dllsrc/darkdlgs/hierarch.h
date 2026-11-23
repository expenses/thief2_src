#ifndef __HIERARCH_H
#define __HIERARCH_H

#include <otreebas.h>
#include "objtree.h"
#include "intsetbx.h"

F_DECLARE_INTERFACE(ITraitManager);
F_DECLARE_INTERFACE(IEditTools);
F_DECLARE_INTERFACE(IObjectSystem);

/////////////////////////////////////////////////////////////////////////////
// cHierarchyDialog dialog
//
// Browse/edit an archetype hierarchy 

class cHierarchyDialog : public CDialog
{
// Construction
public:
	virtual ~cHierarchyDialog();
	cHierarchyDialog(const sObjTreeEditorDesc* desc, ObjID focus = OBJ_NULL, CWnd* pParent = NULL);   // standard constructor

   class cTree : public cObjTree
   {
      ITraitManager* m_pTraitMan; 
      ulong m_flags; 
   protected:
      void OnDrop(const sNode* node, const sNode* target); 
      IObjectQuery* Successors(ObjID obj); 
      BOOL FilterObj(ObjID obj); 

   public:
      cTree(); 
      ~cTree(); 
      BOOL EditNode(const sNode* node); 

      void SetFlags(ulong f) { m_flags = f; }; 
      ulong GetFlags() { return m_flags; }; 
   }; 

protected:
   void EnableButtons(); 
   void DoCBResult(ulong result); 
   const sObjTreeDesc& CurTree(); 

   ObjID m_focus; 
   int m_ntrees; 
   ulong m_flags; 
   ObjID* m_rootids; 
   sObjTreeEditorDesc m_desc; 
   IEditTools* m_pTools; 
   IObjectSystem* m_pObjSys; 

// Dialog Data
	//{{AFX_DATA(cHierarchyDialog)
	enum { IDD = IDD_HIERARCHY };
	cIntSetCombo	m_treelist;
	cTree	m_tree;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(cHierarchyDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(cHierarchyDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnAddnode();
	afx_msg void OnCreate();
	afx_msg void OnDelnode();
	afx_msg void OnSelchangeTreelist();
	afx_msg void OnEdit();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // __HIERARCH_H
