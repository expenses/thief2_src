// COMobjed.cpp
//
// COM face (IObjEditor) on the object editor dialog
//

#include "stdafx.h"
#include <lg.h>
#include <comtools.h>
#include <iobjed.h>
#include <objedbas.h>

#include "darkdlgs.h"
#include "objdlg.h"

class cObjEditorFace : public cObjEditor, 
                       public cCTUnaggregated<IObjEditor,&IID_IObjEditor,kCTU_Default>
{
public:
   cObjEditorFace(const sObjEditorDesc* desc, ObjID obj, CWnd* parent)
      : cObjEditor(desc,obj,parent), m_parent(parent), CB(NULL) {}; 
   virtual ~cObjEditorFace() {}; 

   STDMETHOD_(BOOL,Go)(eObjEdModality modal)
   {
      if (modal == kObjEdModal)
         return DoModal() == IDOK; 

      Create(IDD,m_parent); 
      ShowWindow(SW_SHOWNORMAL); 
      UpdateWindow(); 
      return FALSE; 
   }

   STDMETHOD(SetCallback)(ObjEditCB cb, ObjEditCBData data) { CB = cb; CBData = data; return S_OK; }
   STDMETHOD_(const sObjEditorDesc*,Describe)() { return &m_desc; }
   STDMETHOD_(ObjID,Object)() { return m_obj; }

protected:
   void CallbackEvent(int kind); 

   CWnd* m_parent; 
   ObjEditCB CB; 
   ObjEditCBData CBData; 

	//{{AFX_MSG(cObjEditorFace)
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
}; 

int cObjEditorFace::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (cObjEditor::OnCreate(lpCreateStruct) == -1)
		return -1;
   // the window holds a reference to us 
   AddRef(); 
	return 0;
}

void cObjEditorFace::OnDestroy() 
{
	cObjEditor::OnDestroy();
   Release(); 
}

void cObjEditorFace::OnCancel() 
{
   cObjEditor::OnCancel(); 
   CallbackEvent(kObjEdCancel); 
}

void cObjEditorFace::OnOK() 
{
   cObjEditor::OnOK(); 
   CallbackEvent(kObjEdDone); 
}

void cObjEditorFace::CallbackEvent(int kind)
{
   if (CB)
   {
      sObjEditorEvent event = { kind, this }; 
      CB(&event,CBData); 
   }
}

BEGIN_MESSAGE_MAP(cObjEditorFace, cObjEditor)
	//{{AFX_MSG_MAP(cObjEditorFace)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

////////////////////////////////////////////////////////////

extern "C" __declspec(dllexport) IObjEditor* LGAPI ConstructObjEditor(const sObjEditorDesc* eddesc, ObjID editme)
{
   CWnd* appwnd = CWnd::FromHandle(GetActiveWindow()); 
   return new cObjEditorFace(eddesc,editme,appwnd); 
}
