#ifndef __SDVECDLG_H
#define __SDVECDLG_H

#include <matrixs.h>
#include <sdesbase.h>
#include "sdescctl.h"

/////////////////////////////////////////////////////////////////////////////
// cVectorDialog dialog
//
// A little child dialog with x/y/z edit boxes 

class cVectorDialog : public CDialog
{
// Construction
public:
   static cSdescControl* Construct(sFieldDesc* desc, void* struc); 
	cVectorDialog(sFieldDesc* desc, void* struc);   

// Dialog Data
	//{{AFX_DATA(cVectorDialog)
	enum { IDD = IDD_VECTOR };
	float	m_x;
	float	m_y;
	float	m_z;
	//}}AFX_DATA

   class cControl : public cSdescControl
   {
      cVectorDialog* dlg; 
   public:
      cControl(cVectorDialog* d) : dlg(d) {}; 

      void Create(CWnd* parent, UINT nID, const RECT* rect); 
      void MinDims(CWnd* parent, CDC* dc, RECT* rect); 
      void Destroy() { delete dlg; }; 
      void Update(BOOL save) { dlg->UpdateData(save); }; 
      CWnd* Window() { return dlg; }; 
   }; 

protected:
   cControl m_control; 
   sFieldDesc m_field; 
   mxs_vector* m_pvec; 

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(cVectorDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
 

	// Generated message map functions
protected:
	//{{AFX_MSG(cVectorDialog)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

#endif // __SDVECDLG_H
