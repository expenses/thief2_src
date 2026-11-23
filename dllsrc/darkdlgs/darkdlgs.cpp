#include "stdafx.h"
#include "darkdlgs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CDarkdlgsApp

BEGIN_MESSAGE_MAP(CDarkdlgsApp, CWinApp)
	//{{AFX_MSG_MAP(CDarkdlgsApp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDarkdlgsApp construction

CDarkdlgsApp::CDarkdlgsApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CDarkdlgsApp object

CDarkdlgsApp theApp;

BOOL CDarkdlgsApp::InitInstance()
{
   return CWinApp::InitInstance(); 
}
