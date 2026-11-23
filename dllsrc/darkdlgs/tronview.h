#ifndef __TRONVIEW_H
#define __TRONVIEW_H

#include "linkview.h"

/////////////////////////////////////////////////////////////////////////////
// cTronView dialog

class cTronView : public cLinkView
{
// Construction
public:
	cTronView(const sLinkEditorDesc* desc, cQueryFactory* qfact, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(cTronView)
	enum { IDD = IDD_TRONVIEW };
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(cTronView)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(cTronView)
	afx_msg void OnAdd();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // __TRONVIEW_H
