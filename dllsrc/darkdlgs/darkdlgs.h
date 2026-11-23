#if !defined(AFX_DARKDLGS_H__C5DFFEC6_4A2E_11D0_9EB4_00A024AA6AB7__INCLUDED_)
#define AFX_DARKDLGS_H__C5DFFEC6_4A2E_11D0_9EB4_00A024AA6AB7__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CDarkdlgsApp
// See darkdlgs.cpp for the implementation of this class
//

class CDarkdlgsApp : public CWinApp
{
public:
	CDarkdlgsApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDarkdlgsApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CDarkdlgsApp)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}

#endif // !defined(AFX_DARKDLGS_H__C5DFFEC6_4A2E_11D0_9EB4_00A024AA6AB7__INCLUDED_)
