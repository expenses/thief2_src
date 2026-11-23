#include "stdafx.h"
#include <comtools.h>
#include <appagg.h>
#include <propag8n.h>
#include <propag8r.h>
#include <pg8rbase.h>

#include "darkdlgs.h"
#include "pg8rlist.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// cGatorCombo

cGatorCombo::cGatorCombo()
{
}

cGatorCombo::~cGatorCombo()
{
}

void cGatorCombo::Fill(StimID stim)
{
   ResetContent(); 
   AddItemNamed(0,"None"); 

   AutoAppIPtr_(Propagation,pGation); 
   IPropagatorQuery* gators = pGation->GetPropagators(stim); 
   for (gators->Start(); !gators->Done(); gators->Next())
   {
      cAutoIPtr<IPropagator> gator(gators->Propagator()); 
      AddItemNamed(gators->ID(),gator->Describe()->name); 
   }
   SafeRelease(gators); 
   SetCurSel(0); 
   m_sel = 0; 
}
