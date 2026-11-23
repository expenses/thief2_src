#include "stdafx.h"
#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <objtype.h>
#include <edittool.h>

#include "darkdlgs.h"
#include "Cobjided.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// cObjIDEditor

cObjIDEditor::cObjIDEditor()
{
}

cObjIDEditor::~cObjIDEditor()
{
}

BEGIN_MESSAGE_MAP(cObjIDEditor, CEdit)
	//{{AFX_MSG_MAP(cObjIDEditor)
	ON_CONTROL_REFLECT(EN_CHANGE, OnChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// cObjIDEditor message handlers

void cObjIDEditor::SetObj(ObjID obj)
{
   m_obj = obj; 
}

ObjID cObjIDEditor::GetObj()
{
   return m_obj; 
}

void cObjIDEditor::DoDataExchange(CDataExchange* pDX) 
{
	CEdit::DoDataExchange(pDX);
   AutoAppIPtr_(EditTools,pTools); 
   if (pDX->m_bSaveAndValidate)
   {
      CString text; 
      GetWindowText(text); 
      m_obj = pTools->GetObjNamed(text); 
   }
   else
   {
      const char* name = pTools->ObjName(m_obj); 
      SetWindowText(name); 
   }
}

void cObjIDEditor::OnChange() 
{
   AutoAppIPtr_(EditTools,pTools); 
   CString text; 
   GetWindowText(text); 
   m_obj = pTools->GetObjNamed(text); 
}

void AFXAPI DDX_Control(CDataExchange* pDX, int nIDC, cObjIDEditor& control)
{
   DDX_Control(pDX,nIDC,(CWnd&)control);
   control.UpdateData(pDX->m_bSaveAndValidate);
}
