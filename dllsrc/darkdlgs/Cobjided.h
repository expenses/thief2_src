#ifndef __COBJIDED_H
#define __COBJIDED_H

/////////////////////////////////////////////////////////////////////////////
// cObjIDEditor window
//
// An edit box that holds an object, by name 

class cObjIDEditor : public CEdit
{
// Construction
public:
	cObjIDEditor();

// Attributes
public:
   ObjID GetObj(); 
   void SetObj(ObjID obj); 

private:
   ObjID m_obj; 

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(cObjIDEditor)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~cObjIDEditor();

	// Generated message map functions
protected:
	//{{AFX_MSG(cObjIDEditor)
	afx_msg void OnChange();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

// Unlike MFC's overload, also exchange the child control's data.
void AFXAPI DDX_Control(CDataExchange* pDX, int nIDC, cObjIDEditor& control);

#endif // __COBJIDED_H
