#include "stdafx.h"
#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <sdestool.h>

#include "darkdlgs.h"
#include "sdcheck.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

////////////////////////////////////////////////////////////
// cSdescCheckBox::cControl

void cSdescCheckBox::cControl::MinDims(CWnd* parent, CDC* dc, RECT* rect)
{
   SIZE size = dc->GetTextExtent("X",1); 
   rect->left = 0;
   rect->top = 0;
   rect->right = size.cx + 2*GetSystemMetrics(SM_CXBORDER); 
   rect->bottom = size.cy + 2*GetSystemMetrics(SM_CYBORDER); 
}

void cSdescCheckBox::cControl::Create(CWnd* parent, UINT nID, const RECT* rect)
{
   cSdescCheckBox* box = (cSdescCheckBox*)m_wnd; 
   box->Create("",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX,*rect,parent,nID); 
}

/////////////////////////////////////////////////////////////////////////////
// cSdescCheckBox

cSdescCheckBox::cSdescCheckBox(const sFieldDesc* f, void* str)
   : m_control(this),
     m_struct(str),
     m_field(*f)
{
}

cSdescCheckBox::~cSdescCheckBox()
{
}


BEGIN_MESSAGE_MAP(cSdescCheckBox, CButton)
	//{{AFX_MSG_MAP(cSdescCheckBox)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// cSdescCheckBox message handlers

void cSdescCheckBox::DoDataExchange(CDataExchange* pDX) 
{
	CButton::DoDataExchange(pDX);
   AutoAppIPtr_(StructDescTools,pTools); 
   if (pDX->m_bSaveAndValidate)
   {
      int state = GetCheck(); 
      pTools->SetIntegral(&m_field,state,m_struct); 
   }
   else
   {
      long val; 
      pTools->GetIntegral(&m_field,m_struct,&val); 
      SetCheck(val != 0); 
   }
}

cSdescControl* cSdescCheckBox::Construct(const sFieldDesc* desc, void* struc)
{
   cSdescCheckBox* box = new cSdescCheckBox(desc,struc); 
   return &box->m_control; 
}
