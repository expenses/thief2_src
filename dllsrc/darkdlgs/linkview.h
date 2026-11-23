#ifndef __LINKVIEW_H
#define __LINKVIEW_H

#include <comtools.h>
#include <appagg.h>
#include <linktype.h>
#include <linkman.h>
#include <linkedst.h>
#include "linklist.h"

/////////////////////////////////////////////////////////////////////////////
// cLinkView dialog
//
// A list of links, with buttons to add/edit/delete them

class cLinkView : public CDialog
{
public:
   //
   // Supplies the query of links to show
   //
   class cQueryFactory
   {
   public:
      virtual ~cQueryFactory() {}; 
      virtual ILinkQuery* Query() = 0; 
   }; 

// Construction
public:
	cLinkView(const sLinkEditorDesc* desc, cQueryFactory* qfact, CWnd* pParent = NULL, UINT idd = IDD, cLinkList* list = NULL);   // standard constructor
   virtual ~cLinkView(); 

   BOOL Go(); 

// Dialog Data
	//{{AFX_DATA(cLinkView)
	enum { IDD = IDD_LINKVIEW };
	//}}AFX_DATA

protected:
   cLinkList& m_list; 
   CWnd* m_parent; 
   sLinkEditorDesc m_desc; 
   cQueryFactory* m_factory; 
   ILinkManager* m_pLinkMan; 
   BOOL m_delete_list; 

   virtual void DeleteLink(LinkID id); 
   void RecomputeList(); 

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(cLinkView)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(cLinkView)
	afx_msg void OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult);
	virtual BOOL OnInitDialog();
	afx_msg void OnAdd();
	afx_msg void OnEdit();
	afx_msg void OnDelete();
	afx_msg void OnSource();
	afx_msg void OnDest();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

// The last link editor's "current" link, used as the default for "Add"
extern ObjID gCurrentSrc; 
extern ObjID gCurrentDest; 
extern RelationID gCurrentFlavour; 

#endif // __LINKVIEW_H
