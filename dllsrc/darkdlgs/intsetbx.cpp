#include "stdafx.h"
#include <lg.h>
#include "darkdlgs.h"
#include "intsetbx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// cIntSetCombo

cIntSetCombo::cIntSetCombo()
   : m_sel(0)
{
}

cIntSetCombo::~cIntSetCombo()
{
}

BEGIN_MESSAGE_MAP(cIntSetCombo, CComboBox)
	//{{AFX_MSG_MAP(cIntSetCombo)
	ON_CONTROL_REFLECT_EX(CBN_SELCHANGE, OnSelchange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// cIntSetCombo message handlers

BOOL cIntSetCombo::OnSelchange() 
{
   m_sel = GetItemData(GetCurSel()); 
   return FALSE; // let the parent see it too 
}

void cIntSetCombo::AddItemNamed(ulong item, const char* name)
{
   int idx = AddString(name); 
   SetItemData(idx,item); 
}

void cIntSetCombo::SelectItem(ulong item)
{
   m_sel = item; 
}

ulong cIntSetCombo::Selection()
{
   return m_sel; 
}

void cIntSetCombo::DoDataExchange(CDataExchange* pDX) 
{
	CComboBox::DoDataExchange(pDX);

   if (pDX->m_bSaveAndValidate)
   {
      m_sel = GetItemData(GetCurSel()); 
   }
   else
   {
      BOOL found = FALSE; 
      for (int i = 0; i < GetCount(); i++)
      {
         ulong entry = GetItemData(i); 
         if (entry == m_sel)
         {
            SetCurSel(i); 
            found = TRUE; 
            break; 
         }
      }
      if (!found)
      {
         SetCurSel(0); 
         m_sel = GetItemData(0); 
      }
   }
}

void AFXAPI DDX_Control(CDataExchange* pDX, int nIDC, cIntSetCombo& control)
{
   DDX_Control(pDX,nIDC,(CWnd&)control);
   control.UpdateData(pDX->m_bSaveAndValidate);
}
