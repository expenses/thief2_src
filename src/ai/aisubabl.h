///////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/ai/RCS/aisubabl.h 1.2 1999/03/04 13:31:21 TOML Exp $
//
//
//

#ifndef __AISUBABL_H
#define __AISUBABL_H

#include <aibasatr.h>


///////////////////////////////////////////////////////////////////////////////
//
// Common subability routines 
// Using non-member function to not force clients to do template 
// instantiation, and to not create extra header dependency
//

BOOL AISubabilityIsOwnerLosingControl(const IAIAbility *, const cAIGoal * pPrevious, const cAIGoal * pGoal);

///////////////////////////////////////////////////////////////////////////////
//
// TEMPLATE: cAISubability
//

template <class OWNERABILITY_T>
class cAISubability : public cAIActor
{
public:
   cAISubability()
    : m_pOwner(NULL)
   {
   }

   virtual void InitSubability(OWNERABILITY_T * pOwner)
   {
      m_pOwner = pOwner;
   }
   
protected:
   // Is the owner ability losing control?
   BOOL IsOwnerLosingControl(const cAIGoal * pPrevious, const cAIGoal * pGoal) const
   {
      return AISubabilityIsOwnerLosingControl(m_pOwner, pPrevious, pGoal);
   }
   
   virtual IAIAbility * GetOwner() const
   {
      return m_pOwner;
   }

   OWNERABILITY_T * m_pOwner;
};

///////////////////////////////////////////////////////////////////////////////

#endif /* !__AISUBABL_H */
