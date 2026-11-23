// A combo box whose items each carry an integer; the selection is exchanged
// as that integer.

#ifndef __INTSETBX_H
#define __INTSETBX_H

/////////////////////////////////////////////////////////////////////////////
// cIntSetCombo window

class cIntSetCombo : public CComboBox
{
// Construction
public:
	cIntSetCombo();

// Attributes
public:
   ulong Selection(); 
   void SelectItem(ulong item); 

// Operations
public:
   void AddItemNamed(ulong item, const char* name); 

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(cIntSetCombo)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~cIntSetCombo();

protected:
   ulong m_sel; 

	// Generated message map functions
protected:
	//{{AFX_MSG(cIntSetCombo)
	afx_msg BOOL OnSelchange();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

void AFXAPI DDX_Control(CDataExchange* pDX, int nIDC, cIntSetCombo& control);

#endif // __INTSETBX_H
