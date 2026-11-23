#include "stdafx.h"
#include <lg.h>
#include <comtools.h>

#include "darkdlgs.h"
#include "sdesface.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cBaseStructEditor,IStructEditor); 

/////////////////////////////////////////////////////////////////////////////
// cStructEditor dialog


cStructEditor::cStructEditor(sStructEditorDesc* eddesc, sStructDesc* sdesc, void* editme, CWnd* pParent /*=NULL*/)
	: cSdescShell(eddesc,sdesc,editme,pParent),
     m_flags(eddesc->flags),
     m_parent(pParent),
     m_callback(NULL)
{
	//{{AFX_DATA_INIT(cStructEditor)
	//}}AFX_DATA_INIT
}

void cStructEditor::DoDataExchange(CDataExchange* pDX)
{
	cSdescShell::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(cStructEditor)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(cStructEditor, cSdescShell)
	//{{AFX_MSG_MAP(cStructEditor)
	ON_BN_CLICKED(ID_APPLY_NOW, OnApplyNow)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// cStructEditor message handlers

void cStructEditor::OnOK() 
{
	cSdescShell::OnOK();
   CallbackEvent(kStructEdOK); 
}

void cStructEditor::OnCancel() 
{
	cSdescShell::OnCancel();
   CallbackEvent(kStructEdCancel); 
}

void cStructEditor::OnApplyNow() 
{
   cSdescShell::OnApplyNow(); 
   CallbackEvent(kStructEdApply); 
}

void cStructEditor::CallbackEvent(ulong kind)
{
   if (m_callback)
   {
      sStructEditEvent event = { kind, this }; 
      m_callback(&event,m_cbdata); 
   }
}

////////////////////////////////////////

STDMETHODIMP_(BOOL) cStructEditor::Go(eSdescModality modal)
{
   // no point in applying if no one is listening
   if (m_callback == NULL)
      m_eddesc.flags |= kStructEditNoApplyButton; 

   if (modal == kStructEdModal)
      return DoModal() == IDOK; 

   Create(IDD,m_parent); 
   ShowWindow(SW_SHOWNORMAL); 
   UpdateWindow(); 
   return FALSE; 
}

STDMETHODIMP cStructEditor::SetCallback(StructEditCB cb, StructEditCBData data)
{
   if (cb != NULL && m_eddesc.flags != m_flags)
   {
      m_eddesc.flags = m_flags; 
      EnableButtons(); 
   }
   m_callback = cb; 
   m_cbdata = data; 
   return S_OK; 
}

////////////////////////////////////////

int cStructEditor::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (cSdescShell::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   // the window holds a reference to us
   AddRef(); 
	
	return 0;
}

void cStructEditor::OnDestroy() 
{
	cSdescShell::OnDestroy();
   Release(); 
}

////////////////////////////////////////////////////////////

extern "C" __declspec(dllexport) IStructEditor* LGAPI ConstructStructEditor(sStructEditorDesc* eddesc, sStructDesc* desc, void* editme)
{
   CWnd* appwnd = CWnd::FromHandle(GetActiveWindow()); 



   return new cStructEditor(eddesc,desc,editme,appwnd); 
}

cStructEditor::~cStructEditor()
{
}
