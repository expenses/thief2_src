#include "stdafx.h"
#include "darkdlgs.h"
#include "simpmenu.h"

#include <lg.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// cSimpleMenu dialog


cSimpleMenu::cSimpleMenu(const char* title, const char* const * list, int n, CWnd* pParent /*=NULL*/)
	: CDialog(cSimpleMenu::IDD, pParent),
     m_title(title),
     m_nstrings(n),
     m_liststrings(list)
{
	//{{AFX_DATA_INIT(cSimpleMenu)
	//}}AFX_DATA_INIT
}


void cSimpleMenu::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(cSimpleMenu)
	DDX_Control(pDX, IDC_LIST, m_listbox);
	//}}AFX_DATA_MAP
   if (pDX->m_bSaveAndValidate)
      m_sel = m_listbox.GetItemData(m_listbox.GetCurSel()); 
}


BEGIN_MESSAGE_MAP(cSimpleMenu, CDialog)
	//{{AFX_MSG_MAP(cSimpleMenu)
	ON_LBN_DBLCLK(IDC_LIST, OnDblclkList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// cSimpleMenu message handlers

BOOL cSimpleMenu::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
   for (int i = 0; i < m_nstrings; i++)
   {
      int idx = m_listbox.AddString(m_liststrings[i]); 
      m_listbox.SetItemData(idx,i); 
   }
   m_listbox.SetCurSel(m_listbox.GetTopIndex()); 

   if (m_title != NULL)
      SetWindowText(m_title); 
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

int cSimpleMenu::Selection()
{
   return m_sel; 
}

void cSimpleMenu::OnDblclkList() 
{
   OnOK(); 
}

////////////////////////////////////////////////////////////

extern "C" __declspec(dllexport) int LGAPI DoSimpleMenu(const char* title, const char* const * strings, int n)
{
   CWnd* appwnd = CWnd::FromHandle(GetActiveWindow()); 
   cSimpleMenu dialog(title,strings,n,appwnd);

   if (dialog.DoModal() == IDOK)
      return dialog.Selection(); 
   else
      return -1; 
}
