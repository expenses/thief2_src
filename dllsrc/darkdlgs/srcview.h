#ifndef __SRCVIEW_H
#define __SRCVIEW_H

#include "linkview.h"

/////////////////////////////////////////////////////////////////////////////
// cSourceView dialog

class cSourceView : public cLinkView
{
// Construction
public:
	cSourceView(const sLinkEditorDesc* desc, cQueryFactory* qfact, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(cSourceView)
	enum { IDD = IDD_SOURCEVIEW };
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(cSourceView)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(cSourceView)
	afx_msg void OnAdd();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // __SRCVIEW_H
