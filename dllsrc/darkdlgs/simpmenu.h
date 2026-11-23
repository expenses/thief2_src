#ifndef __SIMPMENU_H
#define __SIMPMENU_H

/////////////////////////////////////////////////////////////////////////////
// cSimpleMenu dialog

class cSimpleMenu : public CDialog
{
// Construction
public:
   cSimpleMenu(const char* title, const char* const * list, int n, CWnd* pParent = NULL);   // standard constructor

   int Selection(); 

// Dialog Data
	//{{AFX_DATA(cSimpleMenu)
	enum { IDD = IDD_SIMPLEMENU };
	CListBox	m_listbox;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(cSimpleMenu)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
   const char* m_title;
   int m_sel; 
   int m_nstrings;
   const char* const * m_liststrings; 

	// Generated message map functions
	//{{AFX_MSG(cSimpleMenu)
	virtual BOOL OnInitDialog();
	afx_msg void OnDblclkList();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // __SIMPMENU_H
