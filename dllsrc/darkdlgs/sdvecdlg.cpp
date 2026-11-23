#include "stdafx.h"
#include <lg.h>
#include "darkdlgs.h"
#include "sdvecdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

////////////////////////////////////////////////////////////
// cVectorDialog::cControl

void cVectorDialog::cControl::MinDims(CWnd* parent, CDC* dc, RECT* rect)
{
   // Find our dialog template, and convert its size from dialog units 
   LPCTSTR resname = MAKEINTRESOURCE(IDD); 
   HINSTANCE hInst = AfxGetResourceHandle();
   HRSRC hResource = ::FindResource(hInst, resname, RT_DIALOG);
   HGLOBAL hTemplate = LoadResource(hInst, hResource);
   const DLGTEMPLATE* lpDialogTemplate = (LPCDLGTEMPLATE)LockResource(hTemplate);

   rect->left = rect->top = 0; 
   SIZE units = dc->GetTextExtent("8",1); 
   rect->right = lpDialogTemplate->cx*units.cx/4; 
   rect->bottom = lpDialogTemplate->cy*units.cy/8; 

   UnlockResource(hTemplate); 
}

void cVectorDialog::cControl::Create(CWnd* parent, UINT nID, const RECT* rect)
{
   dlg->CDialog::Create(IDD,parent); 
   dlg->MoveWindow(rect,FALSE); 
}

/////////////////////////////////////////////////////////////////////////////
// cVectorDialog dialog


cVectorDialog::cVectorDialog(sFieldDesc* desc, void* struc)
	: CDialog(cVectorDialog::IDD, NULL),
     m_control(this),
     m_field(*desc),
     m_pvec((mxs_vector*)((char*)struc + desc->offset))
{
	//{{AFX_DATA_INIT(cVectorDialog)
	m_x = 0.0f;
	m_y = 0.0f;
	m_z = 0.0f;
	//}}AFX_DATA_INIT
}


void cVectorDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
   if (!pDX->m_bSaveAndValidate)
   {
      m_x = m_pvec->x; 
      m_y = m_pvec->y; 
      m_z = m_pvec->z; 
   }
	//{{AFX_DATA_MAP(cVectorDialog)
	DDX_Text(pDX, IDC_X, m_x);
	DDX_Text(pDX, IDC_Y, m_y);
	DDX_Text(pDX, IDC_Z, m_z);
	//}}AFX_DATA_MAP
   if (pDX->m_bSaveAndValidate)
   {
      m_pvec->x = m_x; 
      m_pvec->y = m_y; 
      m_pvec->z = m_z; 
   }
}

BEGIN_MESSAGE_MAP(cVectorDialog, CDialog)
	//{{AFX_MSG_MAP(cVectorDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

cSdescControl* cVectorDialog::Construct(sFieldDesc* desc, void* struc)
{
   cVectorDialog* dlg = new cVectorDialog(desc,struc); 
   return &dlg->m_control; 
}
