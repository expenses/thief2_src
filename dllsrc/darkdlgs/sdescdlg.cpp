#include "stdafx.h"
#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <wappapi.h>

#include "darkdlgs.h"
#include "sdescdlg.h"
#include "sdescctl.h"
#include "sdcheck.h"
#include "sdbits.h"
#include "sdenum.h"
#include "sdtxtbox.h"
#include "sdvecdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// cSdescShell dialog


cSdescShell::cSdescShell(sStructEditorDesc* eddesc, sStructDesc* sdesc, void* struc, CWnd* pParent /*=NULL*/)
	: CDialog(cSdescShell::IDD, pParent),
     m_font(NULL), m_eddesc(*eddesc), m_struct(struc), m_desc(sdesc),
     m_controls(new cSdescControl*[sdesc->nfields]),
     m_captions(new CStatic*[sdesc->nfields])
{ 
	//{{AFX_DATA_INIT(cSdescShell)
	//}}AFX_DATA_INIT
}

cSdescShell::~cSdescShell()
{
   for (int i = 0; i < m_desc->nfields; i++)
      delete m_captions[i]; 
   delete [] m_controls; 
   delete [] m_captions; 
}


void cSdescShell::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(cSdescShell)
	DDX_Control(pDX, IDOK, m_ok);
	DDX_Control(pDX, IDCANCEL, m_cancel);
	DDX_Control(pDX, ID_APPLY_NOW, m_apply);
	//}}AFX_DATA_MAP
   for (int i = 0; i < m_desc->nfields; i++)
      m_controls[i]->Update(pDX->m_bSaveAndValidate); 
}


BEGIN_MESSAGE_MAP(cSdescShell, CDialog)
	//{{AFX_MSG_MAP(cSdescShell)
	ON_BN_CLICKED(ID_APPLY_NOW, OnApplyNow)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// cSdescShell message handlers

void cSdescShell::OnApplyNow() 
{
   UpdateData(TRUE); 
}

BOOL cSdescShell::OnInitDialog() 
{
   LayoutControls(); 
	CDialog::OnInitDialog();
   EnableButtons(); 
   m_controls[0]->Window()->SetFocus(); 
	return FALSE;  // we set the focus 
}

int cSdescShell::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CDialog::OnCreate(lpCreateStruct) == -1)
		return -1;

   int n = m_desc->nfields; 
   for (int i = 0; i < n; i++)
      m_controls[i] = CreateControl(&m_desc->fields[i]); 

   if (m_eddesc.title[0] != '\0')
      SetWindowText(m_eddesc.title); 
   else
      SetWindowText(m_desc->name); 
	
	return 0;
}

////////////////////////////////////////

void cSdescShell::LayoutControls()
{
   CDC* dc = GetDC(); 

   if (m_font != NULL)
      dc->SelectObject(m_font); 
   CSize xsize = dc->GetTextExtent("X",1); 

   int n = m_desc->nfields; 

   RECT dlgrect; 

   GetClientRect(&dlgrect); 

   // compute the size of the controls & captions 
   long h = 0; 
   long w = 0; 
   int capwidth = 0; 
   int i;
   for (i = 0; i < n; i++)
   {
      RECT cdims; 
      m_controls[i]->MinDims(this,dc,&cdims); 
      h += cdims.bottom + 1; 
      if (cdims.right > w)
         w = cdims.right; 

      if (!(m_eddesc.flags & kStructEditNoFieldNames) 
          && !(m_controls[i]->Flags() & cSdescControl::kNoCaption))
      {
         CString text = m_desc->fields[i].name; 
         CSize size = dc->GetTextExtent(text); 
         if (size.cx > capwidth)
            capwidth = size.cx; 
      }
   }


   capwidth += xsize.cx; 
   w += capwidth; 


   h += xsize.cy; 
   w += xsize.cx; 


   if (w < dlgrect.right - dlgrect.left)
      w = dlgrect.right - dlgrect.left; 


   RECT bounds = { xsize.cx/2, xsize.cy/2, w - xsize.cx/2, h - xsize.cy/2 }; 

   // move the buttons down below the controls 

   UINT buttids[] = { IDOK, IDCANCEL, ID_APPLY_NOW }; 
   CWnd* buttwnds[] = { &m_ok, &m_cancel, &m_apply }; 

   for (i = 0; i < 3; i++)
   {
      CWnd* buttwnd = buttwnds[i]; 


      VERIFY(buttwnd->SubclassDlgItem(buttids[i],this)); 
      RECT buttrect; 

      buttwnd->GetWindowRect(&buttrect); 
      ScreenToClient(&buttrect); 
      buttrect.bottom = h + (buttrect.bottom - buttrect.top); 
      buttrect.top = h; 
      buttwnd->MoveWindow(&buttrect,FALSE); 
   }

   RECT okrect; 


   m_ok.GetWindowRect(&okrect); 

   h += okrect.bottom - okrect.top; 

   // resize the dialog, keeping it centered
   RECT wndrect; 



   GetWindowRect(&wndrect); 


   int deltaw = w - (dlgrect.right - dlgrect.left); 
   wndrect.left -= (deltaw+1)/2; 
   wndrect.right += deltaw/2; 

   int deltah = h - (dlgrect.bottom - dlgrect.top); 
   wndrect.bottom += deltah/2; 
   wndrect.top -= (deltah+1)/2; 


   MoveWindow(&wndrect,TRUE); 

   // now create the controls 
   int top = bounds.top; 


   for (i = 0; i < n; i++)
   {

      ulong flags = m_controls[i]->Flags(); 
      RECT mindims; 


      m_controls[i]->MinDims(this,dc,&mindims); 

      RECT dims = bounds; 
      dims.top = top; 
      dims.bottom = top + mindims.bottom; 



      if ((m_eddesc.flags & kStructEditNoFieldNames) 
          || (flags & cSdescControl::kNoCaption))
         m_captions[i] = NULL; 

      else
      {
         CStatic* caption = new CStatic; 
         m_captions[i] = caption; 

         RECT capdims = dims; 

         capdims.right = capdims.left + capwidth; 
         caption->Create(m_desc->fields[i].name,WS_CHILD|WS_VISIBLE,capdims,this,0xFFFF); 

         if (m_font != NULL)
            caption->SetFont(m_font); 

         dims.left = capdims.right; 
      }


      m_controls[i]->Create(this,BaseID+i,&dims); 

      // bit 1 of the editor flags means "read only" here 
      if ((m_eddesc.flags & (1 << 1)) || (m_desc->fields[i].flags & kFieldFlagNotEdit))
         m_controls[i]->Window()->EnableWindow(FALSE); 


      if (m_font != NULL)
         m_controls[i]->Window()->SetFont(m_font); 

      top = dims.bottom + 1; 

   }

   ReleaseDC(dc); 

}

void cSdescShell::OnDestroy() 
{
   for (int i = 0; i < m_desc->nfields; i++)
      m_controls[i]->Destroy(); 
	CDialog::OnDestroy();
}

cSdescControl* cSdescShell::CreateControl(sFieldDesc* desc)
{
   switch(desc->type)
   {
      case kFieldTypeBool:
         return cSdescCheckBox::Construct(desc,m_struct);

      case kFieldTypeVector:
         return cVectorDialog::Construct(desc,m_struct);

      case kFieldTypeBits:
         if (desc->datasize > 0)
            return cSdescBits::Construct(desc,m_struct); 
         break; 

      case kFieldTypeEnum:
         if (desc->datasize > 0)
            return cSdescEnum::Construct(desc,m_struct); 
         break; 

   }
   return cSdescEditBox::Construct(desc,m_struct); 
}

extern "C" __declspec(dllexport) BOOL LGAPI ModalSdescTest(sStructDesc* desc, void* editme)
{
   AutoAppIPtr(WinApp); 
   CWnd* appwnd = CWnd::FromHandle(pWinApp->GetMainWnd()); 
   sStructEditorDesc eddesc = { "Title", 0 }; 
   cSdescShell shell(&eddesc,desc,editme,appwnd); 
   if (shell.DoModal() == IDOK)
      return TRUE;
   return FALSE; 
}

void cSdescShell::OnSetFont(CFont* pFont) 
{
   m_font = pFont; 
	CDialog::OnSetFont(pFont);
}

void cSdescShell::EnableButtons()
{
   CWnd* butts[] = { &m_ok, &m_apply, &m_cancel }; 
   ulong flags[] = { kStructEditNoOKButton, kStructEditNoApplyButton, kStructEditNoCancelButton }; 

   for (int i = 0; i < (int)(sizeof(butts)/sizeof(butts[0])); i++)
   {
      if (m_eddesc.flags & flags[i])
      {
         butts[i]->EnableWindow(FALSE); 
         butts[i]->ShowWindow(SW_HIDE); 
      }
      else
      {
         butts[i]->EnableWindow(TRUE); 
         butts[i]->ShowWindow(SW_SHOW); 
      }
   }
}
