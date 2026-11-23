#ifndef __SDESCDLG_H
#define __SDESCDLG_H

#include <sdesbase.h>
#include <isdesced.h>
#include <isdescst.h>

class cSdescControl; 

/////////////////////////////////////////////////////////////////////////////
// cSdescShell dialog
//
// A dialog that builds one control per field of an sdesc'd struct

class cSdescShell : public CDialog
{
// Construction
public:
	virtual ~cSdescShell();
	cSdescShell(sStructEditorDesc* eddesc, sStructDesc* sdesc, void* struc, CWnd* pParent = NULL);   // standard constructor

   enum { BaseID = 1000 };  // control id of first field

protected:
   void EnableButtons(); 

   CFont* m_font; 
   sStructEditorDesc m_eddesc; 

// Dialog Data
	//{{AFX_DATA(cSdescShell)
	enum { IDD = IDD_SDESC_SHELL };
	CButton	m_ok;
	CButton	m_cancel;
	CButton	m_apply;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(cSdescShell)
	public:
	virtual void OnSetFont(CFont* pFont);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
   void* m_struct; 
   sStructDesc* m_desc; 
   cSdescControl** m_controls; 
   CStatic** m_captions; 

   void LayoutControls(); 
   cSdescControl* CreateControl(sFieldDesc* desc); 

	// Generated message map functions
	//{{AFX_MSG(cSdescShell)
	afx_msg void OnApplyNow();
	virtual BOOL OnInitDialog();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // __SDESCDLG_H
