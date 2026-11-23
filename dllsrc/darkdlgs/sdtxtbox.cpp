#include "stdafx.h"
#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <sdestool.h>

#include "darkdlgs.h"
#include "sdtxtbox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MIN_BUFSIZE 64

////////////////////////////////////////////////////////////
// cSdescEditBox::cControl

void cSdescEditBox::cControl::MinDims(CWnd* parent, CDC* dc, RECT* rect)
{
   cSdescEditBox* box = (cSdescEditBox*)m_wnd; 
   AutoAppIPtr_(StructDescTools,pTools); 
   ulong bufsize = box->m_field.size; 
   if (bufsize < MIN_BUFSIZE) bufsize = MIN_BUFSIZE; 
   char* buf = new char[bufsize]; 
   pTools->UnparseField(&box->m_field,box->m_value,buf,bufsize); 
   strcat(buf,"X"); 
   SIZE size = dc->GetTextExtent(buf,strlen(buf)); 
   delete buf; 

   rect->left = 0;
   rect->top = 0;
   rect->right = size.cx;
   rect->bottom = size.cy + 4; 
}

void cSdescEditBox::cControl::Create(CWnd* parent, UINT nID, const RECT* rect)
{
   cSdescEditBox* box = (cSdescEditBox*)m_wnd; 
   box->Create(WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,*rect,parent,nID); 
}

/////////////////////////////////////////////////////////////////////////////
// cSdescEditBox

cSdescEditBox::cSdescEditBox(const sFieldDesc* field, void* editme)
   : m_field(*field),
     m_value(editme),
     m_ctl(this)
{
}

cSdescEditBox::~cSdescEditBox()
{
}

cSdescControl* cSdescEditBox::Construct(const sFieldDesc* field, void* editme)
{
   cSdescEditBox* box = new cSdescEditBox(field,editme); 
   return &box->m_ctl; 
}


BEGIN_MESSAGE_MAP(cSdescEditBox, CEdit)
	//{{AFX_MSG_MAP(cSdescEditBox)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// cSdescEditBox message handlers

void cSdescEditBox::DoDataExchange(CDataExchange* pDX) 
{
	CEdit::DoDataExchange(pDX);
   AutoAppIPtr_(StructDescTools,pTools); 

   if (pDX->m_bSaveAndValidate)
   {
      CString text; 
      GetWindowText(text); 
      pTools->ParseField(&m_field,text,m_value); 
   }
   else
   {
      ulong bufsize = MIN_BUFSIZE;
      if (m_field.size > bufsize)
         bufsize = m_field.size; 
      char* buf = new char[bufsize]; 
      pTools->UnparseField(&m_field,m_value,buf,bufsize); 
      SetWindowText(buf); 
      delete buf; 
   }
}
