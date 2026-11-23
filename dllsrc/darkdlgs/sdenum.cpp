#include "stdafx.h"
#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <sdestool.h>

#include "darkdlgs.h"
#include "sdenum.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

////////////////////////////////////////////////////////////
// cSdescEnum::cControl

void cSdescEnum::cControl::MinDims(CWnd* parent, CDC* dc, RECT* rect)
{
   cSdescEnum* box = (cSdescEnum*)m_wnd; 
   char** strings = (char**)box->m_field.data; 
   memset(rect,0,sizeof(*rect)); 

   for (int i = 0; i < box->m_field.datasize; i++)
   {
      CString str = strings[i]; 
      SIZE size = dc->GetTextExtent(str); 
      size.cx += 2*GetSystemMetrics(SM_CXEDGE); 
      size.cy += 2*GetSystemMetrics(SM_CYEDGE); 
      if (size.cx > rect->right)
         rect->right = size.cx; 
      if (size.cy > rect->bottom)
         rect->bottom = size.cy; 
   }
   // room for the button 
   rect->right += 2*GetSystemMetrics(SM_CXEDGE); 
   rect->bottom += 2*GetSystemMetrics(SM_CYEDGE); 
}

void cSdescEnum::cControl::Create(CWnd* parent, UINT nID, const RECT* rect)
{
   cSdescEnum* box = (cSdescEnum*)m_wnd; 
   // make room for the drop-down 
   RECT myrect = *rect; 
   myrect.bottom += (myrect.bottom - myrect.top) * box->m_field.datasize; 
   box->Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST,myrect,parent,nID); 
}

/////////////////////////////////////////////////////////////////////////////
// cSdescEnum

cSdescEnum::cSdescEnum(const sFieldDesc* desc, void* struc)
   : m_control(this),
     m_struct(struc),
     m_field(*desc)
{
}

cSdescEnum::~cSdescEnum()
{
}


BEGIN_MESSAGE_MAP(cSdescEnum, cIntSetCombo)
	//{{AFX_MSG_MAP(cSdescEnum)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// cSdescEnum message handlers

int cSdescEnum::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (cIntSetCombo::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   char** strings = (char**)m_field.data; 
   for (int i = 0; i < m_field.datasize; i++)
      AddItemNamed(i + m_field.min, strings[i]); 
	
	return 0;
}

void cSdescEnum::DoDataExchange(CDataExchange* pDX) 
{
   BOOL init = !pDX->m_bSaveAndValidate; 
   AutoAppIPtr_(StructDescTools,pTools); 
   if (init)
   {
      long val; 
      pTools->GetIntegral(&m_field,m_struct,&val); 
      SelectItem(val); 
   }
	cIntSetCombo::DoDataExchange(pDX);
   if (!init)
      pTools->SetIntegral(&m_field,Selection(),m_struct); 
}

cSdescControl* cSdescEnum::Construct(const sFieldDesc* field, void* struc)
{
   cSdescEnum* box = new cSdescEnum(field,struc); 
   return &box->m_control; 
}
