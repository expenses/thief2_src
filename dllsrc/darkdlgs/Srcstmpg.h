#ifndef __SRCSTMPG_H
#define __SRCSTMPG_H

#include <ssrcbase.h>
#include <stimbase.h>
#include "Cobjided.h"
#include "stimlist.h"
#include "pg8rlist.h"

/////////////////////////////////////////////////////////////////////////////
// cSourceStimPage dialog
//
// Object, stimulus, propagator & intensity of a stimulus source

class cSourceStimPage : public CPropertyPage
{
	DECLARE_DYNCREATE(cSourceStimPage)

// Construction
public:
	cSourceStimPage();
	~cSourceStimPage();

   BOOL ToSource(sObjStimPair* elems, sStimSourceDesc* desc); 
   BOOL FromSource(sObjStimPair* elems, sStimSourceDesc* desc); 

// Dialog Data
	//{{AFX_DATA(cSourceStimPage)
	enum { IDD = IDD_SOURCE_STIM_PAGE };
	CButton	m_shape;
	CButton	m_life;
	cObjIDEditor	m_obj;
	cStimCombo	m_stim;
	cGatorCombo	m_gator;
	float	m_level;
	//}}AFX_DATA

protected:
   sStimSourceDesc m_desc; 
   sObjStimPair m_elems; 

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(cSourceStimPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(cSourceStimPage)
	afx_msg void OnSelchangePgator();
	afx_msg void OnSelchangeStimulus();
	virtual BOOL OnInitDialog();
	afx_msg void OnShapebutt();
	afx_msg void OnLifebutt();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

#endif // __SRCSTMPG_H
