#ifndef __SDENUM_H
#define __SDENUM_H

#include <sdesbase.h>
#include "sdescctl.h"
#include "intsetbx.h"

/////////////////////////////////////////////////////////////////////////////
// cSdescEnum window

class cSdescEnum : public cIntSetCombo
{
// Construction
public:
   static cSdescControl* Construct(const sFieldDesc* field, void* struc); 
	cSdescEnum(const sFieldDesc* desc, void* struc);

   class cControl : public cBaseSdescControl
   {
   public:
      cControl(cSdescEnum* b) : cBaseSdescControl(b) {}; 

      void Create(CWnd* parent, UINT nID, const RECT* rect); 
      void MinDims(CWnd* parent, CDC* dc, RECT* rect); 
   }; 
   friend class cControl; 

// Implementation
public:
	virtual ~cSdescEnum();

protected:
   cControl m_control; 
   void* m_struct; 
   sFieldDesc m_field; 

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(cSdescEnum)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	//}}AFX_VIRTUAL

	// Generated message map functions
protected:
	//{{AFX_MSG(cSdescEnum)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#endif // __SDENUM_H
