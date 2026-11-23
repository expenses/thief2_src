#ifndef __OBJDLG_H
#define __OBJDLG_H

#include <objedbas.h>
#include "proptree.h"
#include "hidden.h"

F_DECLARE_INTERFACE(IObjEditors);
F_DECLARE_INTERFACE(IEditTrait);

/////////////////////////////////////////////////////////////////////////////
// cObjEditor dialog
//
// Edit the traits (properties, relations, metaproperties) of an object 

class cObjEditor : public CDialog
{
   typedef cObjTree::sNode sNode; 

// Construction
public:
	cObjEditor(const sObjEditorDesc* desc, ObjID obj, CWnd* pParent = NULL);   // standard constructor

protected:
   //
   // The "add" menu
   //
   class cTraitMenu : public CMenu
   {
   public:
      cTraitMenu(); 
      ~cTraitMenu(); 

      void Go(CWnd* wnd, CPoint pos); 
      IEditTrait* GetTrait(int idx); 

   protected:
      void AddMetaProps(); 
      void DestroySubMenus(); 
      void AddAllTraits(); 
      void AddTrait(IEditTrait* trait, int idx); 
      CMenu* GetContainer(const char* name, CMenu* root = NULL); 
      static BOOL FindSortedMenuIndex(CMenu* menu, const char* name, int* idx); 

      IObjEditors* m_pEdit; 
      CBitmap m_container; 
      CBitmap m_obj; 
      CBitmap m_trait; 
   }; 

   void DoMenuCommand(UINT uitem); 
   void FillHiddenList(); 

   ObjID m_obj; 
   sObjEditorDesc m_desc; 
   cTraitMenu m_menu; 

   static cHiddenList gHiddenList; 

// Dialog Data
	//{{AFX_DATA(cObjEditor)
	enum { IDD = IDD_OBJEDITOR };
	CListBox	m_hidden;
	cTraitTree	m_tree;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(cObjEditor)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(cObjEditor)
	afx_msg void OnAdd();
	afx_msg void OnDelete();
	afx_msg void OnEdit();
	virtual BOOL OnInitDialog();
	afx_msg void OnExpand();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnRefresh();
	afx_msg void OnHide();
	afx_msg void OnDblclkHidden();
	afx_msg void OnSelchangedTree(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // __OBJDLG_H
