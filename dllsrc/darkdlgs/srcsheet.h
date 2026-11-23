#ifndef __SRCSHEET_H
#define __SRCSHEET_H

#include "Srcstmpg.h"

/////////////////////////////////////////////////////////////////////////////
// cStimSrcSheet
//
// Property sheet for editing a stimulus source

class cStimSrcSheet : public CPropertySheet
{
	DECLARE_DYNAMIC(cStimSrcSheet)

// Construction
public:
	cStimSrcSheet(CWnd* pWndParent, sObjStimPair* elems, sStimSourceDesc* desc);

// Attributes
public:
	cSourceStimPage m_stimpage;

// Operations
public:
   void ToSource(sObjStimPair* elems, sStimSourceDesc* desc); 
   void FromSource(sObjStimPair* pair, sStimSourceDesc* desc); 

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(cStimSrcSheet)
	public:
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~cStimSrcSheet();

private:
   sStimSourceDesc m_desc; 
   sObjStimPair m_elems; 

// Generated message map functions
protected:
	//{{AFX_MSG(cStimSrcSheet)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#endif // __SRCSHEET_H
