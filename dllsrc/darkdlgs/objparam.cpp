#include "stdafx.h"
#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <stimsens.h>

#include "darkdlgs.h"
#include "objparam.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// radio buttons for each special object param
static int obj_param_controls[kNumSpecialObjParams] = 
{
   IDC_MEBTN,   // kObjParamSensor
   IDC_SRCBTN,  // kObjParamSource
}; 

/////////////////////////////////////////////////////////////////////////////
// cReactObjParam dialog


cReactObjParam::cReactObjParam(CWnd* pParent /*=NULL*/)
	: CDialog(cReactObjParam::IDD, pParent)
{
	//{{AFX_DATA_INIT(cReactObjParam)
	//}}AFX_DATA_INIT

   AutoAppIPtr_(StimSensors,pSensors); 
   for (int i = 0; i < kNumSpecialObjParams; i++)
      m_obj_param[i] = pSensors->ObjParam(i); 
}


void cReactObjParam::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

   BOOL save = pDX->m_bSaveAndValidate; 
   if (!save)
   {
      BOOL found = FALSE; 
      for (int i = 0; i < kNumSpecialObjParams; i++)
         if (m_obj == m_obj_param[i])
         {
            CheckRadioButton(IDC_OBJBTN,IDC_SRCBTN,obj_param_controls[i]); 
            found = TRUE; 
            break; 
         }

      if (!found)
      {
         CheckRadioButton(IDC_OBJBTN,IDC_SRCBTN,IDC_OBJBTN); 
         m_edit.SetObj(m_obj); 
      }

      CWnd* wnd = GetDlgItem(IDC_OBJEDIT); 
      wnd->EnableWindow(IsDlgButtonChecked(IDC_OBJBTN)); 
   }

	//{{AFX_DATA_MAP(cReactObjParam)
	DDX_Control(pDX, IDC_OBJEDIT, m_edit);
	//}}AFX_DATA_MAP

   if (save)
   {
      BOOL found = FALSE; 
      for (int i = 0; i < kNumSpecialObjParams; i++)
         if (IsDlgButtonChecked(obj_param_controls[i]))
         {
            m_obj = m_obj_param[i]; 
            found = TRUE; 
            break; 
         }
      if (!found)
         m_obj = m_edit.GetObj(); 
   }
}


BEGIN_MESSAGE_MAP(cReactObjParam, CDialog)
	//{{AFX_MSG_MAP(cReactObjParam)
	ON_BN_CLICKED(IDC_OBJBTN, OnObjbtn)
	ON_BN_CLICKED(IDC_MEBTN, OnMebtn)
	ON_BN_CLICKED(IDC_SRCBTN, OnSrcbtn)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// cReactObjParam message handlers

void cReactObjParam::OnObjbtn() 
{
   CWnd* wnd = GetDlgItem(IDC_OBJEDIT); 
   wnd->EnableWindow(IsDlgButtonChecked(IDC_OBJBTN)); 
}

void cReactObjParam::OnMebtn() 
{
   CWnd* wnd = GetDlgItem(IDC_OBJEDIT); 
   wnd->EnableWindow(IsDlgButtonChecked(IDC_OBJBTN)); 
}

void cReactObjParam::OnSrcbtn() 
{
   CWnd* wnd = GetDlgItem(IDC_OBJEDIT); 
   wnd->EnableWindow(IsDlgButtonChecked(IDC_OBJBTN)); 
}

void cReactObjParam::SetObj(ObjID obj)
{
   m_obj = obj; 
}

cReactObjParam::~cReactObjParam()
{
}

ObjID cReactObjParam::GetObj()
{
   return m_obj; 
}

void cReactObjParam::SetCaption(const char* cap)
{
   CWnd* wnd = GetDlgItem(IDC_CAPTION); 
   wnd->SetWindowText(cap); 
}

// we're a child dialog; don't let enter/escape close us 
void cReactObjParam::OnCancel() 
{
}

void cReactObjParam::OnOK() 
{
}

// Create ourselves inside a placeholder control in the parent
void cReactObjParam::DoCreate(CWnd* parent, UINT nID)
{
   HWND hwnd = ::GetDlgItem(parent->m_hWnd,nID); 
   CWnd* wnd = CWnd::FromHandle(hwnd); 
   Create(IDD,wnd); 
   ShowWindow(SW_SHOWNORMAL); 
   UpdateWindow(); 
}

void cReactObjParam::SelectSpecial(int which)
{
   m_obj = m_obj_param[which]; 
}
