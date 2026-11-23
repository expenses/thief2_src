#ifndef __ADDLINK_H
#define __ADDLINK_H

#include <linktype.h>
#include <linkbase.h>
#include "Cobjided.h"
#include "intsetbx.h"

/////////////////////////////////////////////////////////////////////////////
// cAddLinkDialog dialog
//
// Pick a source, destination and flavor for a new link

class cAddLinkDialog : public CDialog
{
// Construction
public:
	cAddLinkDialog(const sLink* link, CWnd* pParent = NULL);   // standard constructor

   // run the dialog; returns TRUE and fills in link if OK was pressed 
   static BOOL DoDialog(sLink* link); 

   void ToLink(sLink* link); 
   void FromLink(const sLink* link); 

protected:
   sLink m_link; 

// Dialog Data
	//{{AFX_DATA(cAddLinkDialog)
	enum { IDD = IDD_ADDLINK };
	cObjIDEditor	m_to;
	cObjIDEditor	m_from;
	cIntSetCombo	m_flavor;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(cAddLinkDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(cAddLinkDialog)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // __ADDLINK_H
