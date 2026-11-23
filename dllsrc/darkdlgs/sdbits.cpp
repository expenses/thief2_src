#include "stdafx.h"
#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <sdestool.h>

#include "darkdlgs.h"
#include "sdbits.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

////////////////////////////////////////////////////////////
// cSdescBits::cControl

void cSdescBits::cControl::MinDims(CWnd* parent, CDC* dc, RECT* rect)
{
   SIZE size = dc->GetTextExtent("Xflags...X",5); 
   rect->left = 0;
   rect->top = 0;
   rect->right = size.cx + 4*GetSystemMetrics(SM_CXEDGE); 
   rect->bottom = size.cy + 4*GetSystemMetrics(SM_CYEDGE); 
}

void cSdescBits::cControl::Create(CWnd* parent, UINT nID, const RECT* rect)
{
   cSdescBits* box = (cSdescBits*)m_wnd; 
   box->Create("flags...",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,*rect,parent,nID); 
}

/////////////////////////////////////////////////////////////////////////////
// cSdescBits

cSdescBits::cSdescBits(const sFieldDesc* desc, void* str)
   : m_control(this),
     m_struct(str),
     m_field(*desc),
     m_menu(desc)
{
}

cSdescBits::~cSdescBits()
{
}


BEGIN_MESSAGE_MAP(cSdescBits, CButton)
	//{{AFX_MSG_MAP(cSdescBits)
	ON_CONTROL_REFLECT(BN_CLICKED, OnClicked)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// cSdescBits message handlers

void cSdescBits::OnClicked() 
{
   RECT rect; 
   GetWindowRect(&rect); 
   CPoint pos(rect.right,rect.top); 
   m_menu.Go(this,pos); 
}

void cSdescBits::DoDataExchange(CDataExchange* pDX) 
{
	CButton::DoDataExchange(pDX);
   long value; 
   AutoAppIPtr_(StructDescTools,pTools); 
   if (pDX->m_bSaveAndValidate)
   {
      m_menu.ToBits(&value); 
      pTools->SetIntegral(&m_field,value,m_struct); 
   }
   else
   {
      pTools->GetIntegral(&m_field,m_struct,&value); 
      m_menu.FromBits(value); 
   }
   Relabel(); 
}

BOOL cSdescBits::OnCommand(WPARAM wParam, LPARAM lParam) 
{
   // menu command 
   if (lParam == 0 && HIWORD(wParam) == 0)
   {
      int i = LOWORD(wParam); 
      UINT check = m_menu.CheckMenuItem(i,MF_BYCOMMAND); 
      check ^= MF_CHECKED;
      m_menu.CheckMenuItem(i,MF_BYCOMMAND|check); 
      UpdateData(TRUE); 
   }
	return CButton::OnCommand(wParam, lParam);
}

cSdescControl* cSdescBits::Construct(const sFieldDesc* desc, void* struc)
{
   cSdescBits* box = new cSdescBits(desc,struc); 
   return &box->m_control; 
}

////////////////////////////////////////////////////////////
// cSdescBits::cMenu

cSdescBits::cMenu::cMenu(const sFieldDesc* desc)
   : m_field(desc)
{
   CreatePopupMenu(); 
   FillItems(); 
}

cSdescBits::cMenu::~cMenu()
{
   DestroyMenu(); 
}

void cSdescBits::cMenu::FillItems()
{
   char** strings = (char**)m_field->data;
   strings -= m_field->min; 
   for (int i = m_field->min; i < m_field->max; i++)
      AppendMenu(MF_STRING,i,strings[i]); 
}

void cSdescBits::cMenu::FromBits(long bits)
{
   for (int i = m_field->min; i < m_field->max; i++)
   {
      int check = (bits & (1 << i)) ? MF_CHECKED : MF_UNCHECKED; 
      CheckMenuItem(i,MF_BYCOMMAND|check); 
   }
}

void cSdescBits::cMenu::ToBits(long* bits)
{
   *bits = 0; 
   for (int i = m_field->min; i < m_field->max; i++)
      if (GetMenuState(i,MF_BYCOMMAND) & MF_CHECKED)
         *bits |= 1 << i; 
}

void cSdescBits::cMenu::Go(CWnd* wnd, CPoint pos)
{
   TrackPopupMenu(TPM_LEFTALIGN,pos.x,pos.y,wnd); 
}

////////////////////////////////////////////////////////////

void cSdescBits::Relabel()
{
   AutoAppIPtr_(StructDescTools,pTools); 
   char buf[128]; 
   pTools->UnparseField(&m_field,m_struct,buf,sizeof(buf)); 
   SetWindowText(buf); 
}
