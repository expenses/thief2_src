///////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/ai/RCS/aimanbhv.h 1.1 1999/05/10 10:45:43 TOML Exp $
//
//
//

#ifndef __AIMANBHV_H
#define __AIMANBHV_H

#include <hashpp.h>
#include <hshpptem.h>
#include <aiapibhv.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIBehaviorSets
//

template <class KEY> 
class cHashTableCaselessFunctions
{
public:
   static unsigned Hash(const KEY & k)
   {
      return cCaselessStringHashFuncs::Hash(k);
   }

   static BOOL IsEqual(const KEY& k1, const KEY& k2)
   {
      return cCaselessStringHashFuncs::IsEqual(k1,k2);
   }
};

class cAIBehaviorSets : private cHashTable<const char *, IAIBehaviorSet *, cHashTableCaselessFunctions<const char *> >
{
public:
   void             Add(IAIBehaviorSet * pBehaviorSet);
   IAIBehaviorSet * Get(const char * pszName);
   void             ReleaseAll();
};

///////////////////////////////////////

inline void cAIBehaviorSets::Add(IAIBehaviorSet * pBehaviorSet)
{
   Insert(pBehaviorSet->GetName(), pBehaviorSet);
}

///////////////////////////////////////

inline IAIBehaviorSet * cAIBehaviorSets::Get(const char * pszName)
{
   IAIBehaviorSet * pResult = NULL;
   Lookup(pszName, &pResult);
   return pResult;
}

///////////////////////////////////////

inline void cAIBehaviorSets::ReleaseAll()
{
   cIter iter = Iter();

   while (!iter.Done())
   {
      iter.Value()->Release();
      iter.Next();
   }
}

///////////////////////////////////////////////////////////////////////////////

#endif /* !__AIMANBHV_H */
