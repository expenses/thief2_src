#ifndef __SDTXTBOX_H
#define __SDTXTBOX_H

#include <sdesbase.h>
#include "sdescctl.h"

/////////////////////////////////////////////////////////////////////////////
// cSdescEditBox window
//
// An edit box that parses/unparses a field

class cSdescEditBox : public CEdit
{
// Construction
public:
	cSdescEditBox(const sFieldDesc* field, void* editme);
   static cSdescControl* Construct(const sFieldDesc* field, void* editme); 

// Attributes
public:
   sFieldDesc m_field; 
   void* m_value; 

   class cControl : public cBaseSdescControl
   {
   public:
      cControl(cSdescEditBox* b) : cBaseSdescControl(b) {}; 

      void Create(CWnd* parent, UINT nID, const RECT* rect); 
      void MinDims(CWnd* parent, CDC* dc, RECT* rect); 
   }; 

   cControl m_ctl; 

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(cSdescEditBox)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~cSdescEditBox();

	// Generated message map functions
protected:
	//{{AFX_MSG(cSdescEditBox)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#endif // __SDTXTBOX_H
