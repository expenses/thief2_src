#include "stdafx.h"
#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <stimsrc.h>
#include <ssrcbase.h>
#include <stimbase.h>
#include <propag8n.h>
#include <propag8r.h>
#include <pg8rbase.h>
#include <aredit.h>

#include "darkdlgs.h"
#include "srclist.h"
#include "aredit_.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// cSourceList

cSourceList::cSourceList()
   : m_pSources(AppGetObj(IStimSources))
{
}

cSourceList::~cSourceList()
{
   SafeRelease(m_pSources); 
}

static cLinkList::sColumnDesc columns[] = 
{
   { "ID",           75,  cLinkList::kIDColumn },
   { "Object",       90,  cLinkList::kSourceColumn },
   { "Stimulus",     90,  cLinkList::kDestColumn },
   { "Intensity",    75,  cSourceList::kLevelColumn },
   { "Propagator",   100, cSourceList::kPropagatorColumn },
   { "" }, 
}; 

const cLinkList::sColumnDesc* cSourceList::DescribeColumns()
{
   return columns; 
}

BEGIN_MESSAGE_MAP(cSourceList, cLinkList)
	//{{AFX_MSG_MAP(cSourceList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

int cSourceList::AddLink(LinkID id)
{
   int idx = cLinkList::AddLink(id); 

   sStimSourceDesc source; 
   m_pSources->DescribeSource(id,&source); 

   char buf[40]; 
   sprintf(buf,"%.02f",source.level); 
   SetItem(idx,kLevelColumn,buf); 

   AutoAppIPtr(Propagation); 
   IPropagator* gator = pPropagation->GetPropagator(source.propagator); 
   SetItem(idx,kPropagatorColumn,gator->Describe()->name); 
   SafeRelease(gator); 

   return idx; 
}

void cSourceList::EditLink(LinkID sourceid)
{
   IActReactEditors* editors = ExportActReactEditors(); 
   if (editors == NULL)
      return; 

   sObjStimPair pair;
   pair = m_pSources->GetSourceElems(sourceid); 
   sStimSourceDesc source; 
   m_pSources->DescribeSource(sourceid,&source); 

   if (editors->SourceDialog(kARDlgModal,&pair,&source) == S_OK)
   {
      if (pair.obj == OBJ_NULL || pair.stim == OBJ_NULL)
         return; 




      // replace the source 
      m_pSources->RemoveSource(sourceid); 
      m_pSources->AddSource(pair.obj,pair.stim,&source); 
   }
   SafeRelease(editors); 
}
