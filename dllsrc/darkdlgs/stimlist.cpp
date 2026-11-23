#include "stdafx.h"
#include <comtools.h>
#include <appagg.h>
#include <objquery.h>
#include <stimuli.h>

#include "darkdlgs.h"
#include "stimlist.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// cStimCombo

cStimCombo::cStimCombo()
{
}

cStimCombo::~cStimCombo()
{
}


BEGIN_MESSAGE_MAP(cStimCombo, cIntSetCombo)
	//{{AFX_MSG_MAP(cStimCombo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// cStimCombo message handlers

void cStimCombo::Fill(IObjectQuery* stims)
{
   ResetContent(); 
   AddItemNamed(0,"None"); 

   AutoAppIPtr(Stimuli); 
   for (; !stims->Done(); stims->Next())
   {
      StimID stim = stims->Object(); 
      const char* name = pStimuli->GetStimulusName(stim); 
      AddItemNamed(stim,name); 
   }
   SetCurSel(0); 
   m_sel = 0; 
}
