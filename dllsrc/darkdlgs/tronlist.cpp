#include "stdafx.h"
#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <stimsens.h>
#include <sensbase.h>
#include <receptro.h>
#include <stimbase.h>
#include <reaction.h>
#include <reacbase.h>
#include <aredit.h>

#include "darkdlgs.h"
#include "tronlist.h"
#include "aredit_.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// cTronList

cTronList::cTronList()
   : m_pSensors(AppGetObj(IStimSensors))
{
}

cTronList::~cTronList()
{
   SafeRelease(m_pSensors); 
}

static cLinkList::sColumnDesc columns[] = 
{
   { "ID",           75,  cLinkList::kIDColumn },
   { "Object",       90,  cLinkList::kSourceColumn },
   { "Stimulus",     90,  cLinkList::kDestColumn },
   { "Min",          75,  cTronList::kMinColumn },
   { "Max",          75,  cTronList::kMaxColumn },
   { "Reaction",     100, cTronList::kReactionColumn },
   { "" }, 
}; 

const cLinkList::sColumnDesc* cTronList::DescribeColumns()
{
   return columns; 
}

BEGIN_MESSAGE_MAP(cTronList, cLinkList)
	//{{AFX_MSG_MAP(cTronList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

int cTronList::AddLink(LinkID id)
{
   int idx = cLinkList::AddLink(id); 

   sReceptron tron; 
   m_pSensors->GetReceptron(id,&tron); 

   char buf[40]; 
   if (tron.trigger.flags & kStimTrigNoMin)
      strcpy(buf,"None"); 
   else
      sprintf(buf,"%.02f",tron.trigger.min); 
   SetItem(idx,kMinColumn,buf); 

   if (tron.trigger.flags & kStimTrigNoMax)
      strcpy(buf,"None"); 
   else
      // The reference displays min here too, despite the "Max" column.
      sprintf(buf,"%.02f",tron.trigger.min); 
   SetItem(idx,kMaxColumn,buf); 

   AutoAppIPtr(Reactions); 
   const sReactionDesc* react = pReactions->DescribeReaction(tron.effect.kind); 
   SetItem(idx,kReactionColumn,react->friendly_name); 

   return idx; 
}

void cTronList::EditLink(LinkID tronid)
{
   IActReactEditors* editors = ExportActReactEditors(); 
   if (editors == NULL)
      return; 

   AutoAppIPtr(StimSensors); 
   sObjStimPair pair;
   pair = pStimSensors->GetReceptronElems(tronid); 
   sReceptron tron; 
   pStimSensors->GetReceptron(tronid,&tron); 

   if (editors->ReceptronDialog(kARDlgModal,&pair,&tron) == S_OK)
   {
      if (pair.obj == OBJ_NULL || pair.stim == OBJ_NULL)
         return; 
      pStimSensors->SetReceptron(tronid,&tron); 
   }
   SafeRelease(editors); 
}
