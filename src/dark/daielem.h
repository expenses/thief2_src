///////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/dark/RCS/daielem.h 1.2 1998/08/31 11:16:18 TOML Exp $
//
//
//

#ifndef __DAIELEM_H
#define __DAIELEM_H

#include <ai.h>
#include <aibascmp.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIElemental
//

class cAIElemental : public cAI
{
public:
   STDMETHOD_(float, GetGroundOffset)();
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIElementalLightAbility
//

class cAIElementalLightAbility : public cAIComponentBase<IAIComponent, &IID_IAIComponent>
{
public:
   cAIElementalLightAbility();

   STDMETHOD_(const char *, GetName)();
   STDMETHOD_(void, Init)();

   // Notifications
   STDMETHOD_(void, OnModeChange)(eAIMode previous, eAIMode mode);

private:
   int m_lastLightProp;
};

///////////////////////////////////////////////////////////////////////////////

#endif /* !__DAIELEM_H */
