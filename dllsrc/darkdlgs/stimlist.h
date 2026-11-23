#ifndef __STIMLIST_H
#define __STIMLIST_H

#include "intsetbx.h"

F_DECLARE_INTERFACE(IObjectQuery); 

/////////////////////////////////////////////////////////////////////////////
// cStimCombo window
//
// A combo box of stimuli, by StimID

class cStimCombo : public cIntSetCombo
{
// Construction
public:
	cStimCombo();

// Operations
public:
   // fill from a query of stimuli; consumes nothing
   void Fill(IObjectQuery* stims); 

// Implementation
public:
	virtual ~cStimCombo();

	// Generated message map functions
protected:
	//{{AFX_MSG(cStimCombo)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#endif // __STIMLIST_H
