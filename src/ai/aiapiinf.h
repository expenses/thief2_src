///////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/ai/RCS/aiapiinf.h 1.2 1998/09/22 13:48:37 TOML Exp $
//
//
//

#ifndef __AIAPIINF_H
#define __AIAPIINF_H

#include <comtools.h>
#include <aitype.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

F_DECLARE_INTERFACE(IInternalAI);
F_DECLARE_INTERFACE(IAIInform);
typedef unsigned eAISoundConcept;

///////////////////////////////////////////////////////////////////////////////

struct sAIInform
{
   sAIInform() : concept(0) {}
   sAIInform(IInternalAI * pAI, eAISoundConcept concept, ObjID source);
   
   eAISoundConcept concept;
   ObjID           ofWhom;
   eAIAlertLevel   alertValue;
   mxs_vector      loc;
   unsigned        locTime;
   unsigned        maxPass;
   unsigned        transferDelay;
   unsigned        expiration;
};


///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IAIInform
//

IAIInform * AIGetInform(IInternalAI * pAI);

///////////////////////////////////////

DECLARE_INTERFACE_(IAIInform, IUnknown)
{
   //
   // 
   //
   STDMETHOD_(BOOL, WantsInforms)() PURE;
   
   //
   // 
   //
   STDMETHOD (Post)(const sAIInform *) PURE;

   //
   // 
   //
   STDMETHOD (Receive)(const sAIInform *) PURE;

};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIAPIINF_H */
