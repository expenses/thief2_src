// sdesface.h
//
// COM face of the struct editor (IStructEditor)
//

#ifndef __SDESFACE_H
#define __SDESFACE_H

#include <comtools.h>
#include <isdesced.h>
#include <isdescst.h>
#include "sdescdlg.h"

//
// Ref-counting IStructEditor base
//

class cBaseStructEditor : public IStructEditor
{
public:
   DECLARE_UNAGGREGATABLE(); 
   virtual ~cBaseStructEditor() {}; 
}; 

//
// The editor itself: a (possibly modeless) cSdescShell 
//

class cStructEditor : public cSdescShell, public cBaseStructEditor
{
public:
	virtual ~cStructEditor();
	cStructEditor(sStructEditorDesc* eddesc, sStructDesc* sdesc, void* editme, CWnd* pParent = NULL);   // standard constructor

	enum { IDD = IDD_SDESC_SHELL };

protected:
   ulong m_flags; 
   CWnd* m_parent; 
   StructEditCBData m_cbdata; 
   StructEditCB m_callback; 

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(cStructEditor)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

   //
   // IStructEditor methods
   //
   STDMETHOD_(BOOL,Go)(eSdescModality modal);
   STDMETHOD(SetCallback)(StructEditCB cb, StructEditCBData data); 
   STDMETHOD_(const sStructEditorDesc*,Describe)() { return &m_eddesc; }; 
   STDMETHOD_(const sStructDesc*,DescribeStruct)() { return m_desc; }; 
   STDMETHOD_(void*,Struct)() { return m_struct; }; 

   void CallbackEvent(ulong kind); 

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(cStructEditor)
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnApplyNow();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // __SDESFACE_H
