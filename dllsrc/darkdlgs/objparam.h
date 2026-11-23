#ifndef __OBJPARAM_H
#define __OBJPARAM_H

#include <sensbase.h>
#include "Cobjided.h"

/////////////////////////////////////////////////////////////////////////////
// cReactObjParam dialog
//
// A child dialog for picking a reaction object parameter: either a
// specific object, or one of the special ones ("me", "source")

class cReactObjParam : public CDialog
{
// Construction
public:
   void SelectSpecial(int which); 
   void DoCreate(CWnd* parent, UINT nID); 
   void SetCaption(const char* cap); 
   ObjID GetObj(); 
	virtual ~cReactObjParam();
   void SetObj(ObjID obj); 
	cReactObjParam(CWnd* pParent = NULL);   // standard constructor

   ObjID m_obj_param[kNumSpecialObjParams]; 

// Dialog Data
	//{{AFX_DATA(cReactObjParam)
	enum { IDD = IDD_REACT_OBJPARAM };
	cObjIDEditor	m_edit;
	//}}AFX_DATA

protected:
   ObjID m_obj; 

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(cReactObjParam)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(cReactObjParam)
	afx_msg void OnObjbtn();
	afx_msg void OnMebtn();
	afx_msg void OnSrcbtn();
	virtual void OnCancel();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // __OBJPARAM_H
