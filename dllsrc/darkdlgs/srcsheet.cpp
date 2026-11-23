#include "stdafx.h"
#include <lg.h>
#include "darkdlgs.h"
#include "srcsheet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// cStimSrcSheet

IMPLEMENT_DYNAMIC(cStimSrcSheet, CPropertySheet)

cStimSrcSheet::cStimSrcSheet(CWnd* pWndParent, sObjStimPair* elems, sStimSourceDesc* desc)
	 : CPropertySheet(IDS_SOURCE_EDITOR, pWndParent)
{
   memset(&m_elems,0,sizeof(m_elems));
   memset(&m_desc,0,sizeof(m_desc)); 
	AddPage(&m_stimpage);
   FromSource(elems,desc); 
}

cStimSrcSheet::~cStimSrcSheet()
{
}

BEGIN_MESSAGE_MAP(cStimSrcSheet, CPropertySheet)
	//{{AFX_MSG_MAP(cStimSrcSheet)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// cStimSrcSheet message handlers

void cStimSrcSheet::FromSource(sObjStimPair* pair, sStimSourceDesc* desc)
{
   if (pair != NULL)
      m_elems = *pair; 
   if (desc != NULL)
      m_desc = *desc; 
   m_stimpage.FromSource(&m_elems,&m_desc); 
}

void cStimSrcSheet::ToSource(sObjStimPair* elems, sStimSourceDesc* desc)
{
   m_stimpage.ToSource(elems,desc); 
}

BOOL cStimSrcSheet::OnInitDialog() 
{
   m_stimpage.FromSource(&m_elems,&m_desc); 
	BOOL result = CPropertySheet::OnInitDialog();
	return result;
}
