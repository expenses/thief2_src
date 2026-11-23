#ifndef __SDBITS_H
#define __SDBITS_H

#include <sdesbase.h>
#include "sdescctl.h"

/////////////////////////////////////////////////////////////////////////////
// cSdescBits window
//
// A button that pops up a menu of checkable flags

class cSdescBits : public CButton
{
// Construction
public:
   static cSdescControl* Construct(const sFieldDesc* desc, void* struc); 
	cSdescBits(const sFieldDesc* desc, void* str);

   class cMenu : public CMenu
   {
   protected:
      const sFieldDesc* m_field; 

   public:
      cMenu(const sFieldDesc* desc); 
      ~cMenu(); 

      void FromBits(long bits); 
      void ToBits(long* bits); 
      void Go(CWnd* wnd, CPoint pos); 

   protected:
      void FillItems(); 
   }; 

   class cControl : public cBaseSdescControl
   {
   public:
      cControl(cSdescBits* b) : cBaseSdescControl(b) {}; 

      void Create(CWnd* parent, UINT nID, const RECT* rect); 
      void MinDims(CWnd* parent, CDC* dc, RECT* rect); 
   }; 

// Implementation
public:
	virtual ~cSdescBits();

protected:
   cControl m_control; 
   void* m_struct; 
   sFieldDesc m_field; 
   cMenu m_menu; 

   void Relabel(); 

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(cSdescBits)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

	// Generated message map functions
protected:
	//{{AFX_MSG(cSdescBits)
	afx_msg void OnClicked();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#endif // __SDBITS_H
