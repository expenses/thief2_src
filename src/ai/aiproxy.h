///////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/ai/RCS/aiproxy.h 1.1 1999/05/21 13:29:12 TOML Exp $
//
//
//

#ifndef __AIPROXY_H
#define __AIPROXY_H

#include <ai.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIProxy.  For multi-player games, where the "brains" of this AI are elsewhere.
//

class cAIProxy: public cAI
{
public:
   STDMETHOD_(BOOL, IsNetworkProxy)();  // True for all instances of this class.
   STDMETHOD_(void, StartProxyAction)(IAIAction *pAction, ulong deltaTime);
   virtual HRESULT OnNormalFrame();
   virtual void Enact();
};

///////////////////////////////////////////////////////////////////////////////

#endif /* !__AIPROXY_H */
