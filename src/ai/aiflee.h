///////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/ai/RCS/aiflee.h 1.6 1999/05/10 19:10:47 JON Exp $
//
//
//

#ifndef __AIFLEE_H
#define __AIFLEE_H

#include <aibasabl.h>
#include <aiutils.h>

#pragma once
#pragma pack(4)

struct sAIFleeDest;
struct sAIFleeConditions;

///////////////////////////////////////////////////////////////////////////////
//
// Publish ability and all associated link and property types
//

BOOL AIInitFleeAbility(IAIManager *);
BOOL AITermFleeAbility();

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIFlee
//

class cAIFlee : public cAIAbility
{
public:
   cAIFlee();

   // Standard component methods
   STDMETHOD_(const char *, GetName)();
   STDMETHOD_(void, Init)();

   STDMETHOD_(BOOL, Save)(ITagFile *);
   STDMETHOD_(BOOL, Load)(ITagFile *);

   // Notifications
   STDMETHOD_(void, OnGoalChange)(const cAIGoal * pPrevious, const cAIGoal * pGoal);
   STDMETHOD_(void, OnActionProgress)(IAIAction * pAction);
   STDMETHOD_(void, OnAlertness)(ObjID source,
                                 eAIAwareLevel previous, 
                                 eAIAwareLevel current,
                                 const sAIAlertness * pRaw);
   
   STDMETHOD_(void, OnDamage)(const sDamageMsg * pMsg, ObjID realCulpritID);
   STDMETHOD_(void, OnWeapon)(eWeaponEvent ev, ObjID victim, ObjID culprit);

   // Ability flow
   STDMETHOD (SuggestGoal)(cAIGoal *, cAIGoal **);
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);

protected:
   BOOL CheckHitpointsFlee(const sAIFleeConditions * pConditions);
   BOOL CheckFriendsNotFlee(const sAIFleeConditions * pConditions);
   BOOL IsFleeing();

   ObjID GetFleeSource();
   void SetFleeSource(ObjID);

   ObjID PickFleePoint(ObjID);
   const sAIFleeDest * GetFleeDest();
   void SetFleeDest(ObjID);
   void FlagFleeDestReached();
   BOOL GetFleeDestReached();
   unsigned GetFleeDestExpiration();
   void SetFleeDestExpiration(unsigned);
   void StopFlee();
   
   cAITimer       m_CheckTimer;
   cAIRandomTimer m_BroadcastTimer;

};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIFLEE_H */
