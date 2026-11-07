/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#ifdef EDITOR
#line 1 "r:\\prj\\thief2\\skup\\thief2\\src\\ai\\aiqdeath.cpp"
#else
#line 1 "s:\\prj\\thief2\\src\\ai\\aiqdeath.cpp"
#endif
#line 1
//
//
//

#include <aiqdeath.h>

#include <lg.h>
#include <appagg.h>

#include <aiapisnd.h>
#include <aidebug.h>
#include <aisndtyp.h>

#include <creatext.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIQuickDeath
//

STDMETHODIMP_(const char *) cAIQuickDeath::GetName()
{
   return "QuickDeath";
}

//////////////////////////////////////

STDMETHODIMP_(void) cAIQuickDeath::Init()
{
   // Intentially not calling base -- want no goals or notifications by default
   SetNotifications(kAICN_Death);
}

//////////////////////////////////////

STDMETHODIMP_(void) cAIQuickDeath::OnDeath(const sDamageMsg * pMsg)
{
   // Halt all current sounds
   if (m_pAI->AccessSoundEnactor())
      m_pAI->AccessSoundEnactor()->HaltCurrent();

   CreatureBeDead(m_pAI->GetObjID());
   m_pAI->Kill();

   AIWatch(Death, m_pAI->GetObjID(), "quick slain");
}



