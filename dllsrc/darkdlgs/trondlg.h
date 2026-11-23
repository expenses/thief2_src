#ifndef __TRONDLG_H
#define __TRONDLG_H

#include <stimbase.h>
#include <sensbase.h>
#include <receptro.h>
#include <reacbase.h>
#include "intsetbx.h"
#include "stimlist.h"
#include "Cobjided.h"
#include "objparam.h"

/////////////////////////////////////////////////////////////////////////////
// cReceptronDialog dialog

class cReceptronDialog : public CDialog
{
// Construction
public:
	cReceptronDialog(sObjStimPair* pair, sReceptron* tron, CWnd* pParent = NULL);   // standard constructor

   void ToTron(sObjStimPair* elems, sReceptron* tron); 
   void FromTron(sObjStimPair* elems, sReceptron* tron); 

// Dialog Data
	//{{AFX_DATA(cReceptronDialog)
	enum { IDD = IDD_RECEPTRON };
private:
	CStatic	m_effect_text;
	cIntSetCombo	m_effect;
	cStimCombo	m_stim;
	cObjIDEditor	m_obj;
	float	m_max;
	float	m_min;
	BOOL	m_nomax;
	BOOL	m_nomin;
	//}}AFX_DATA

protected:
   cReactObjParam m_agent; 
   cReactObjParam m_target; 
   sReactionParam m_param; 
   sReceptron m_tron; 
   sObjStimPair m_elems; 

   void FillEffectText(const char* type); 

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(cReceptronDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
   void UpdateReaction(); 

	// Generated message map functions
	//{{AFX_MSG(cReceptronDialog)
	afx_msg void OnNomax();
	afx_msg void OnNomin();
	afx_msg void OnTempbutt();
	afx_msg void OnSelchangeReaction();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // __TRONDLG_H
