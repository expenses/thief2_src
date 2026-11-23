#ifndef __SDCHECK_H
#define __SDCHECK_H

#include <sdesbase.h>
#include "sdescctl.h"

/////////////////////////////////////////////////////////////////////////////
// cSdescCheckBox window

class cSdescCheckBox : public CButton
{
// Construction
public:
   static cSdescControl* Construct(const sFieldDesc* desc, void* struc); 
	cSdescCheckBox(const sFieldDesc* f, void* str);

// Attributes
private:
   class cControl : public cBaseSdescControl
   {
   public:
      cControl(cSdescCheckBox* b) : cBaseSdescControl(b) {}; 

      void Create(CWnd* parent, UINT nID, const RECT* rect); 
      void MinDims(CWnd* parent, CDC* dc, RECT* rect); 
   }; 

   cControl m_control; 
   void* m_struct; 
   sFieldDesc m_field; 

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(cSdescCheckBox)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~cSdescCheckBox();

	// Generated message map functions
protected:
	//{{AFX_MSG(cSdescCheckBox)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#endif // __SDCHECK_H
